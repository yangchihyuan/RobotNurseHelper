#include "ThreadProcessImage.hpp"
#include "utility_TimeRecorder.hpp"
#include "utility_directory.hpp"
#include "utility_string.hpp"
#include "utility_time.hpp"
#include <numeric>      // std::iota
#include "JPEG.hpp"
#ifdef USE_KEBBI
    #include "Kebbi/RobotCommand.pb.h"
#elif USE_ZENBO
    #include "Zenbo/RobotCommand.pb.h"
#endif
#include "utility_directory.hpp"

#include "RobotStatus.hpp"

// Compiled protobuf headers for MediaPipe types used
//The two files are at /home/chihyuan/mediapipe/bazel-bin/mediapipe/framework/formats
//They are included by target_include_directories(MP_FORMATS PUBLIC "${MEDIAPIPE_DIR}/bazel-bin") in the CMakeLists.txt
//However, they seem uncessary here because I move them to the GetLandmarks.hpp
//#include "mediapipe/framework/formats/landmark.pb.h"
//#include "mediapipe/framework/formats/image_format.pb.h"

#include <google/protobuf/text_format.h>

#include "GetLandmarks.hpp"
#include "LandmarkToRobotAction.hpp"

//xt::argmax
#include "xtensor/containers/xarray.hpp"
#include "xtensor/io/xio.hpp"
#include "xtensor/core/xmath.hpp"
#include "xtensor/misc/xsort.hpp"

RobotStatus robot_status;

int is_dancing = 0;

//for Yolo11n-pose
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "SocketBufferParser.hpp"   //DataFrame is defined in this hpp file
#include "ThreadSafeQueue.hpp"

ThreadProcessImage::ThreadProcessImage()
{
    LoadJSONFile(msetting, "json/Setting.json");

    //Initialize the EmotiEffLib
    string backend = "onnx";
    string modelName = EmotiEffLib::getSupportedModels(backend)[0];
    string ext = ".onnx";
    string Homepath(getenv("HOME"));
    string emotiEffLibRootDir = Homepath + "/RobotNurseHelper_build/EmotiEffLib";
    string modelPath = emotiEffLibRootDir + "/models/affectnet_emotions/onnx/" + modelName + ext;
    fer = EmotiEffLib::EmotiEffLibRecognizer::createInstance(backend, modelPath);

    deserialize("dlib_face_recognition_resnet_model_v1.dat") >> net;

    string filepath = std::filesystem::current_path() / "mediapipe_addition/graph_strings/face_cpu.txt";
    string graph_string = LoadFileToString(filepath);
    libmp_face.reset(mediapipe::LibMP::Create(graph_string.c_str(), "input_video"));
    libmp_face->AddOutputStream("multi_face_landmarks");
    libmp_face->AddOutputStream("output_video");
    libmp_face->Start();

    filepath = std::filesystem::current_path() / "mediapipe_addition/graph_strings/hand_cpu.txt";
    graph_string = LoadFileToString(filepath);
    libmp_hand.reset(mediapipe::LibMP::Create(graph_string.c_str(), "input_video"));
    libmp_hand->AddOutputStream("landmarks");
    libmp_hand->AddOutputStream("output_video");
    libmp_hand->Start();

    filepath = std::filesystem::current_path() / "mediapipe_addition/graph_strings/pose_cpu.txt";
    graph_string = LoadFileToString(filepath);
    libmp_pose.reset(mediapipe::LibMP::Create(graph_string.c_str(), "input_video"));
    libmp_pose->AddOutputStream("pose_landmarks");
    libmp_pose->AddOutputStream("output_video");
    libmp_pose->Start();

    const char* InspireFaceModelPath = "/home/chihyuan/Downloads/InspireFace/test_res/pack/Pikachu"; // 確保此路徑下有模型文件
    if (HFLaunchInspireFace(InspireFaceModelPath) != HSUCCEED) {
        std::cerr << "InspireFace 初始化失敗！" << std::endl;
    }    

    // 1. 初始化 Session 參數結構體
    HFSessionCustomParameter customParameter = {0}; // 全部初始化為 0    
    // 2. 依照需求開啟功能 (1 為開啟, 0 為關閉)
    customParameter.enable_recognition = 1;  // 開啟人臉識別 (特徵提取)
    customParameter.enable_liveness = 0;         // 關閉 RGB 活體檢測
    customParameter.enable_face_attribute = 1;   // 開啟人臉屬性分析 (如性別、年齡)
    customParameter.enable_ir_liveness = 0;      // 關閉紅外線活體檢測

    HFDetectMode detectMode = HF_DETECT_MODE_ALWAYS_DETECT; //HF_DETECT_MODE_LIGHT_TRACK; //HF_DETECT_MODE_TRACK_BY_DETECTION;  // 使用偵測驅動的追蹤模式，適合高解析度視頻流

    HInt32 maxDetectFaceNum = 5;  // 設定最大偵測人臉數量

    HInt32 detectPixelLevel = 320;  // 修改偵測器的輸入解析度等級，數值越大越好，默認值 -1 為 320，建議輸入 160、320、640 等 160 的倍數

    HInt32 trackByDetectModeFPS = -1;  // 如果使用 MODE_TRACK_BY_DETECTION 模式，設定當前輸入視頻流的 fps 幀率，默認值 -1 為 30fps
    HResult sessionRet = HFCreateInspireFaceSession(customParameter, detectMode, maxDetectFaceNum, detectPixelLevel, trackByDetectModeFPS, &session);
    if (sessionRet == HSUCCEED) {
        std::cout << "Session 建立成功！" << std::endl;
    } else {
        std::cout << "Session 建立失敗，錯誤碼: " << sessionRet << std::endl;
    }
}

ThreadProcessImage::~ThreadProcessImage()
{
    HFReleaseInspireFaceSession(session);

}

void ThreadProcessImage::run()
{
    auto previous_time = std::chrono::high_resolution_clock::now();

    std::vector<std::vector<std::array<float, 3>>> last_landmarks;
    bool bDrawImageByOurOwn = true;
    int iFrameCount = 0;
    int iNoPersonFrameCount = 0;  //If cannot find a person for 30 frames, move the head to up right frontal
    //try CPU first
    std::vector<std::vector<std::array<float, 3>>> NL_pose;   //normalized_landmarks;
    Mat inputImage;                 //BGR (Blue, Green, Red), this is OpenCV's default color channel order. I will convert it to RGB order before sending it to MediaPipe because MediaPipe uses RGB order.
    chrono::time_point<chrono::system_clock> previous_image_save_time;
    while(b_WhileLoop)
    {
        if( DataFrames_queue.size() > 0 )
        {
            auto start = std::chrono::high_resolution_clock::now();
            DataFrame dataframe;
            //It takes 68 to 138 ms to process a frame, so I need to clear the queue to reduce latency.
            //Maybe it takes less time if I use a faster PC.
            while (DataFrames_queue.size() > 0)
            {
                DataFrames_queue.pop(dataframe);
            }
            char *data_ = dataframe.data.get();
            size_t data_length = dataframe.length;
            
            bool bCorrectlyDecoded = false;

            RobotCommandProtobuf::RobotToServerMessage RTSmessage;
            //2025/12/27 Debug, to parse socket buffer data, I should use ParseFromArray instead of ParseFromString.
            bool parseSuccess = RTSmessage.ParseFromArray(data_, static_cast<int>(data_length));
            if (!parseSuccess) {
                cout << "Failed to parse protobuf message" << endl;
                continue;
            }
            std::vector<uchar> JPEG_Data;
            if( RTSmessage.has_jpegdata() && RTSmessage.has_jpegdatalength())
            {
                string strJPEG_Data = RTSmessage.jpegdata();
                if (strJPEG_Data.length() == 0) {
                    cout << "Warning: jpegdata is empty" << endl;
                }
                JPEG_Data.assign(strJPEG_Data.begin(), strJPEG_Data.end());
            }
            else
            {
                cout << "No jpegdata or jpegdatalength in the protobuf message" << endl;
                continue;
            }
            
            try{
                if (JPEG_Data.empty()) {
                    cout << "JPEG_Data is empty, skipping imdecode." << endl;
                    continue;
                }
                inputImage = imdecode(JPEG_Data, IMREAD_COLOR);
                if( inputImage.data )
                {
                    bCorrectlyDecoded = true;
                }
                else
                {
                    cout << "imdecode fails." << endl;
                    continue;
                }
            }
            catch(exception &e)
            {
                cout << "imdecode try catch exception: " << e.what() << endl;
            }

            if( bCorrectlyDecoded)
            {
                if( iFrameCount == 0 )
                { 
                    inputImage.copyTo(outFrame);  //To let outFrame has buffer
                    //I draw the outFrame by myself, so I don't need to use the output_video of MediaPipe. But MediaPipe requires an output buffer to write the output video. So I create outFrame for this purpose. I will draw the output video by myself, and I will not use the content of outFrame outside this function.

                    inputImage.copyTo(tempFrame);  //To let tempFrame has buffer
                    //tempFrame is used to receive the output video from MediaPipe. I don't need it, but MediaPipe requires an output buffer to write the output video. So I create tempFrame for this purpose. I will not use the content of tempFrame outside this function.
                }

                bool bSaveProcessResult = false;     //default false, this variable is controlled by the iFrameCount
                if(bSaveTransmittedImage)
                {
                    chrono::time_point<chrono::system_clock> protobufTimestamp = protobufTimestampToTimePoint(RTSmessage.event_time());
                    if(iFrameCount == 0 || protobufTimestamp - previous_image_save_time > chrono::milliseconds(msetting.iImageSaveIntervalMillisecond) )
                    {
                        const bool bMillisecond = true;
                        mstr_captured_timestamp = ConvertProtobufTimestampToString( RTSmessage.event_time(), bMillisecond);     //use event_time as the timestamp for saving images. It is more accurate than using the current time.
                        string filename = ImageSaveDirectory + "/" + mstr_captured_timestamp + ".jpg";
                        previous_image_save_time = protobufTimestamp;
                        if(! m_bDirectoryCreated )
                        {
                            if( !CheckDirectoryExist(ImageSaveDirectory))
                            {
                                CreateDirectory(ImageSaveDirectory);
                                m_bDirectoryCreated = true;
                            }
                        }
                        save_image_JPEG(JPEG_Data, filename);           //wrong color.
                        bSaveProcessResult = true;
                    }
                }


                //Draw Pose landmarks
                mtx_UpdateOutFrame.lock();
                //ToDo: remove this variable.
                //if( b_HumanPoseEstimation)
                if( msetting.bHumanPoseEstimation )
                {
                    mtx_Task.lock();
                    bool use_Yolo11n_Pose = true;
                    if( msetting.PoseEstimationModel == "Yolo11n_Pose")
                    {
                        use_Yolo11n_Pose = true;
                    }
                    else if( msetting.PoseEstimationModel == "MediaPipe_Pose")
                    {
                        use_Yolo11n_Pose = false;
                    }
                    else
                    {
                        cout << "Unknown PoseEstimationModel: " << msetting.PoseEstimationModel << ". Use Yolo11n_Pose by default." << endl;
                        use_Yolo11n_Pose = true;
                    }

                    if( use_Yolo11n_Pose)
                    {
                        NL_pose = yolo11pose.Process(inputImage);    //process the inputImage and draw the pose on inputImage
                    }
                    else
                    {
                        if( !libmp_pose->Process2(inputImage) )
                        {
                            std::cerr << "Libmp_pose Proces() failed!" << std::endl;
                            break;
                        }
                    }
                    
                    //2025 Nov 5. Debug: MediaPipe cannot run GPU and CPU at the same time.
                    if( use_Yolo11n_Pose)
                    {
                        int num_kps = 17;    //for pose
                        const float KP_CONF_THRES = 0.4f;
                        inputImage.copyTo(outFrame);
                        size_t num_poses = NL_pose.size();
                        for (int pose_num = 0; pose_num < num_poses; pose_num++) {
                            //draw skeleton
                            for (auto& pr : yolo11pose.skeleton) {
                                int a = pr.first;
                                int b2 = pr.second;
                                float kp_conf_a = NL_pose[pose_num][a][2];
                                float kp_conf_b2 = NL_pose[pose_num][b2][2];
                                if (a < num_kps && b2 < num_kps &&
                                    kp_conf_a > KP_CONF_THRES && kp_conf_b2 > KP_CONF_THRES) {
                                    cv::Scalar col = yolo11pose.pair_color(a, b2);
                                    int a_x = static_cast<int>(NL_pose[pose_num][a][0] * inputImage.cols);
                                    int a_y = static_cast<int>(NL_pose[pose_num][a][1] * inputImage.rows);
                                    int b2_x = static_cast<int>(NL_pose[pose_num][b2][0] * inputImage.cols);
                                    int b2_y = static_cast<int>(NL_pose[pose_num][b2][1] * inputImage.rows);
                                    cv::line(outFrame, cv::Point(a_x, a_y), cv::Point(b2_x, b2_y), col, 3, cv::LINE_AA);
                                }
                            }
                        }
                        bNewoutFrame = true;
                    }
                    else
                    {
                        if( libmp_pose->WriteOutputImage(outFrame.data, libmp_pose->GetOutputPacket("output_video")) )
                        {
                            bNewoutFrame = true;
                        }
                        else
                        {
                            cout << "WriteOutputImage fails." << std::endl;
                        }

                        NL_pose = get_landmarks_pose(libmp_pose);      //I use this to guide robot's movement
                    }
                }

                //Draw hand landmarks
                if( msetting.bHandLandmarkDetection )
                {
                    if( !libmp_hand->Process2(inputImage) )
                    {
                        std::cerr << "Libmp_hand Proces() failed!" << std::endl;
                        break;
                    }

                    //I don't need the frame because I only need the landmarks.
                    if( libmp_hand->WriteOutputImage(tempFrame.data, libmp_hand->GetOutputPacket("output_video") ) )
                    {
                        bNewoutFrame = true;
                    }
                    else
                    {
                        cout << "libmp_hand WriteOutputImage fails." << std::endl;
                    }

                    std::vector<std::vector<std::array<float, 3>>> NL_hands;   //normalized_landmarks;
                    NL_hands = get_landmarks_hand(libmp_hand);
                    if( bDrawImageByOurOwn )
                    {
                        size_t num_hands = NL_hands.size();
                        for (int hand_num = 0; hand_num < num_hands; hand_num++) {
                            for (const std::array<float, 3>& norm_xyz : NL_hands[hand_num]) {
                                int x = static_cast<int>(norm_xyz[0] * inputImage.cols);
                                int y = static_cast<int>(norm_xyz[1] * inputImage.rows);
                                cv::circle(outFrame, cv::Point(x, y), 5, cv::Scalar(0, 0, 255), -1);
                            }
                        }
                    }
                }

                std::vector<std::vector<std::array<float, 3>>> NL_faces;   //normalized_landmarks;
                if( msetting.bFaceDetection )
                {
                    if( msetting.FaceDetectionModel == "InspireFace")
                    {
                        //Use InspireFace to detect faces.
                        HFImageBitmapData imageBitmapData;
                        imageBitmapData.data = inputImage.data;
                        imageBitmapData.width = inputImage.cols;
                        imageBitmapData.height = inputImage.rows;
                        imageBitmapData.channels = inputImage.channels();

                        HResult result;
                        HFImageBitmap image;
                        result = HFCreateImageBitmap(&imageBitmapData, &image);
                        if (result != HSUCCEED) {
                            std::cerr << "Failed to create image bitmap." << std::endl;
                            break;
                        }

                        HFImageStream imageHandle = {0};
                        result = HFCreateImageStreamFromImageBitmap(image, HF_CAMERA_ROTATION_0, &imageHandle);
                        if (result != HSUCCEED) {
                            std::cerr << "Failed to set image format." << std::endl;
                            break;
                        }

                        result = HFSessionSetFaceDetectThreshold(session, 0.5);  // 可選：設定人臉偵測的信心閾值，範圍為 0.0 到 1.0
                        if (result != HSUCCEED) {
                            std::cerr << "Failed to set face detection threshold." << std::endl;
                            break;
                        }

                        HFMultipleFaceData MultipleFaceData = {0};
                        result = HFExecuteFaceTrack(session, imageHandle, &MultipleFaceData);
                        if (result != HSUCCEED) {
                            std::cerr << "Failed to execute face tracking." << std::endl;
                            break;
                        }
                        
                        for (int i = 0; i < MultipleFaceData.detectedNum; i++) {
                            HFaceRect faceRect = MultipleFaceData.rects[i];
                            cv::rectangle(outFrame, cv::Point(faceRect.x, faceRect.y), cv::Point(faceRect.x+faceRect.width-1, faceRect.y+faceRect.height-1), cv::Scalar(0, 255, 0), 2);
                            //cout << "Face " << i << ": x=" << faceRect.x << ", y=" << faceRect.y << ", width=" << faceRect.width << ", height=" << faceRect.height << endl;                        
                        }
                        HFReleaseImageStream(imageHandle);
                    }                    
                    else if( msetting.FaceDetectionModel == "MediaPipe_Face")
                    {
                        //Use MediaPipe to detect faces.
                        if( !libmp_face->Process2(inputImage) )
                        {
                            std::cerr << "libmp_face Process() failed!" << std::endl;
                            break;
                        }

                        //Draw face
                        //Do I need the output_video of libmp_face? I only need the landmarks.
                        //I draw the MediaPipe output to tempFrame, which is not used outside this function.
                        if( libmp_face->WriteOutputImage(tempFrame.data, libmp_face->GetOutputPacket("output_video") ) )
                        {
                            bNewoutFrame = true;
                        }
                        else
                        {
                            cout << "WriteOutputImage fails." << std::endl;
                        }
                        NL_faces = get_landmarks_face(libmp_face);
                        if( bDrawImageByOurOwn )
                        {
                            size_t num_faces = NL_faces.size();
                            for (int face_num = 0; face_num < num_faces; face_num++) {
                                for (const std::array<float, 3>& norm_xyz : NL_faces[face_num]) {
                                    int x = static_cast<int>(norm_xyz[0] * inputImage.cols);
                                    int y = static_cast<int>(norm_xyz[1] * inputImage.rows);
                                    cv::circle(outFrame, cv::Point(x, y), 1, cv::Scalar(0, 255, 0), -1);
                                }
                            }
                        }


    //                    if( m_bRecognizeFacialExpression && !NL_faces.empty() )
                        if( !NL_faces.empty() )
                        {
                            size_t num_faces = NL_faces.size();
                            for (int face_num = 0; face_num < num_faces; face_num++) {
                                //crop the face region.
                                //Why is the cropped region too small?
                                Mat face = CropRegion(inputImage, NL_faces[face_num]);
                                //Debug 2025 Nov 5: Here is the reason that Hinton frozens.
                                //I sitll don't know why. But if I disable this imshow(), Hinton works fine.
    //                            cv::imshow("Cropped face", face);
                                if( msetting.bFacialExpressionRecognition )
                                {
                                    auto res = fer->predictEmotions(face, false);       //false will return the softmax scores
                                    Rect roi = GetBoundingBoxFromLandmarks(NL_faces[face_num], inputImage.cols, inputImage.rows);
                                    //not very accurate, the cropped face is too small, around 135x156.
        //                            cout << "Cropped size: " << face.cols << " " << face.rows << endl;
                                    //std::format requires C++20, but my project uses C++17. So I use sprintf instead.
        //                            cv::putText(outFrame, res.labels[0] + std::format("{:.3f}", res.scores[0]) , Point(roi.x, roi.y) , cv::FONT_HERSHEY_SIMPLEX, 1. , cv::Scalar(0,255,0), 2);
                                    char text[256];
                                    sprintf(text, "%s%.3f", res.labels[0].c_str(), res.scores[0]);
                                    cv::putText(outFrame, text, Point(roi.x, roi.y), cv::FONT_HERSHEY_SIMPLEX, 1., cv::Scalar(0,255,0), 2);
                                }


                                //Get face recognition features
                                //Although there is only one face, the dlib face recognition model needs a vector of faces as input.
                                if( msetting.bUseDlibForFaceRecognition )
                                {
                                    std::vector<dlib::matrix<dlib::rgb_pixel>> faces;
                                    dlib::matrix<dlib::rgb_pixel> dlib_face;
                                    //The face size has to be 150x150, which is the input size of the dlib face recognition model. So I need to resize the cropped face to 150x150 before sending it to the dlib model. But resizing may cause distortion, so I will skip this step if the face size is not correct.
                                    if( face.cols != 150 || face.rows != 150)
                                    {
                                        //Use OpenCV to resize the face to 150x150. But it may cause distortion. I will skip this step if the face size is not correct.
                                        Mat resized_face;
                                        cv::resize(face, resized_face, Size(150, 150));
                                        face = resized_face;
                                    }
                                    dlib::assign_image(dlib_face, dlib::cv_image<dlib::bgr_pixel>(face));
                                    faces.push_back(dlib_face);
                                    //dlib use GPU already, but it is still slow.
                                    //std::cout << "CUDA Device Count: " << dlib::cuda::get_num_devices() << std::endl;                            
                                    std::vector<matrix<float,0,1>> face_descriptors = net(faces);
                                    //It is a vector of 128D
                                    //print it out
                                    cout << "face descriptor for one face: " << dlib::trans(face_descriptors[0]) << endl;
                                }                            
                                //how to create a cluster of the face descriptors for face recognition?
                                //no idea now.
                            }
                        }
                    

                    }
                    else
                    {
                        cout << "Unknown FaceDetectionModel: " << msetting.FaceDetectionModel << ". Use InspireFace by default." << endl;
                    }
                
                    //If I turn this on, the FaceTrack no longer works. Why?
                    //Is it unit session rather than a single frame?
                    /*
                    if( MultipleFaceData.detectedNum > 0 )
                    {
                        HFFaceAttributeResult faceAttributeResult;
                        HResult result = HFGetFaceAttributeResult(session, &faceAttributeResult);
                        if( result == HSUCCEED )
                        {
                            cout << "Successfully get face attribute result." << endl;
                            if( faceAttributeResult.num > 0 )
                            {
                                if( faceAttributeResult.gender == 0 )
                                {
                                    cout << "Gender: Female" << endl;
                                }
                                else
                                {
                                    cout << "Gender: Male" << endl;
                                }
                                cout << "Age bracket: " << faceAttributeResult.ageBracket << endl;
                            }
                        }
                        else
                        {
                            cout << "Failed to get face attribute result, error code: " << result << endl;
                        }
                    }
                    */
                }




                //Dump outFrame for debugging
                if(bSaveProcessResult)
                {
                    string filename = ImageSaveDirectory + "/" + mstr_captured_timestamp + ".outFrame.jpg";
                    cv::imwrite(filename, outFrame);

                    size_t num_faces = NL_faces.size();
                    for (int face_num = 0; face_num < num_faces; face_num++) 
                    {
                        Mat face = CropRegion(inputImage, NL_faces[face_num]);
                        filename = ImageSaveDirectory + "/" + mstr_captured_timestamp + ".face" + to_string(face_num) + ".jpg";
                        cv::imwrite(filename, face);
                    }
                }

                mtx_UpdateOutFrame.unlock();

                //This variable is used to prevent the robot from sending new commands while the previous command is being executed.
                if( mbWatchPatient )
                {
                    if( !NL_pose.empty())      //If there is no person detected, the following code will not be executed.
                    {
                        iNoPersonFrameCount = 0;

                        //use time control
                        auto current_time = chrono::high_resolution_clock::now();
                        auto duration = chrono::duration_cast<chrono::seconds>(current_time - previous_time);
                        //to prevent too many messages being sent to the robot, I set a time interval between two messages.
                        if (duration.count() >= 1) {
                            if( action_option.move_mode != action_option.MOVE_MANUAL)
                            {
                                RobotCommandProtobuf::RobotCommand command;
                                PoseLandmarks_to_RobotAction_yolo(NL_pose, robot_status, action_option, command);
                                previous_time = current_time;
                                pSendMessageManager->AddMessage(command);       //The command is filled in the PoseLandmarks_to_RobotAction function
                            }
                        }
                    }
                    else
                    {
                        iNoPersonFrameCount++;
                        if( iNoPersonFrameCount > 30)
                        {
                            RobotCommandProtobuf::RobotCommand command;
                            command.set_yaw(0);
                            command.set_pitch(0);
                            pSendMessageManager->AddMessage(command);
                            //debug
                            iNoPersonFrameCount = 0;
                        }
                    }
                }    //if( mbWatchPatient )
                mtx_Task.unlock();    
            }    //if( bCorrectlyDecoded )

            //debug code, to messure the processing time
            bool bShowTransmittedImage = false;
            if( bShowTransmittedImage )
            {
                auto stop = std::chrono::high_resolution_clock::now();
                auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
                std::cout << "Elapsed time: " << duration_ms.count() << " milliseconds" << std::endl;
            }

            iFrameCount++;
        } //if( pSocketBufferParser->get_queue_length() > 0 )
        msleep(1);   //to prevent CPU usage too high
    } //while(b_WhileLoop)
    cout << "Exit ThreadProcessImage loop." << endl;
}

void ThreadProcessImage::NotifyEvent(string description, chrono::time_point<chrono::system_clock> timestamp, float yaw, float pitch )
{
    if( description == "onCompleteOfMotionPlay")
    {
//        cout << "(A) onCompleteOfMotionPlay yaw " << yaw << " pitch " << pitch << endl;
        robot_status.yaw_degree = (int)yaw;
        robot_status.pitch_degree = (int)pitch;
        mbWatchPatient = true;
    }
    else if(description == "KebbiMoveHeadDuringMotion")
    {
//        cout << "(B) KebbiMoveHeadDuringMotion " << endl;
        mbWatchPatient = false;
    }
}

Mat ThreadProcessImage::getOutFrame()
{
    Mat frame;
    mtx_UpdateOutFrame.lock();
    if( bNewoutFrame )
    {
        outFrame.copyTo(frame);
    }
    mtx_UpdateOutFrame.unlock();
    return frame;
}

Mat ThreadProcessImage::CropRegion(Mat inputImage, std::vector<std::array<float, 3>> normalized_landmarks)
{
    Rect roi = GetBoundingBoxFromLandmarks(normalized_landmarks, inputImage.cols, inputImage.rows);
    Mat cropped_face = inputImage(roi).clone(); //clone to ensure a deep copy
    return cropped_face;
}

cv::Rect ThreadProcessImage::GetBoundingBoxFromLandmarks(const std::vector<std::array<float, 3>>& normalized_landmarks, int img_width, int img_height)
{
    //Find the bounding box of the landmarks
    float x_min = 1.0, x_max = 0.0, y_min = 1.0, y_max = 0.0;
    for( const auto& norm_xyz : normalized_landmarks)
    {
        if( norm_xyz[0] < x_min ) x_min = norm_xyz[0];
        if( norm_xyz[0] > x_max ) x_max = norm_xyz[0];
        if( norm_xyz[1] < y_min ) y_min = norm_xyz[1];
        if( norm_xyz[1] > y_max ) y_max = norm_xyz[1];
    }

    //Convert to pixel coordinates
    int x1 = static_cast<int>( x_min * img_width );
    int y1 = static_cast<int>( y_min * img_height );
    int x2 = static_cast<int>( x_max * img_width );
    int y2 = static_cast<int>( y_max * img_height );

    //Ensure the bounding box is within image boundaries
    if( x1 < 0 ) x1 = 0;
    if( y1 < 0 ) y1 = 0;
    if( x2 > img_width ) x2 = img_width;
    if( y2 > img_height ) y2 = img_height;

    //Return the bounding box
    return Rect(x1, y1, x2 - x1, y2 - y1);
}