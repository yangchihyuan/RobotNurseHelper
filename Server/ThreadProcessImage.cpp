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
}


void ThreadProcessImage::run()
{
    auto previous_time = std::chrono::high_resolution_clock::now();

    std::vector<std::vector<std::array<float, 3>>> last_landmarks;
    bool bLastLandmarksEffective = false;
    int iFrameCount = 0;
    int iNoPersonFrameCount = 0;  //If cannot find a person for 30 frames, move the head to up right frontal
    Mat inputImage;                 //BGR (Blue, Green, Red)

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
            std::vector<uchar> JPEG_Data;  //2025/12/27 Debug: I declare the JPEG_Data twice. I copy the RTSmessage.jpegdata to the second one in the if section. The first one before the if section is still empty.
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
                    cv::cvtColor(inputImage, inputImage, cv::COLOR_RGB2BGR);
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
                    inputImage.copyTo(tempFrame);  //To let tempFrame has buffer
                }

                if(bSaveTransmittedImage)
                {
                    if(iFrameCount % image_save_every_N_frame == 0 )
                    {
                        string str_now = GetCurrentTimeString(true);

                        string filename = ImageSaveDirectory + "/" + str_now + ".jpg";
                        if(! m_bDirectoryCreated )
                        {
                            if( !CheckDirectoryExist(ImageSaveDirectory))
                            {
                                CreateDirectory(ImageSaveDirectory);
                                m_bDirectoryCreated = true;
                            }
                        }
//                        save_image_JPEG(data_ + shift_length, iJPEG_length , filename);
                        save_image_JPEG(JPEG_Data, filename);
                        iFrameCount = 0; //reset the frame count
                    }
                    else
                    {
                        iFrameCount++;
                    }
                }

                if( b_HumanPoseEstimation)
                {
//                    start = std::chrono::high_resolution_clock::now();

                    mtx_Task.lock();
                    //try CPU first
                    std::vector<std::vector<std::array<float, 3>>> NL_pose;   //normalized_landmarks;
                    bool use_Yolo11n_Pose = true;
                    //2025/11/18 ToDo: Yolo11n-pose is computational expensive, and it cause cv::imshow() to frozen on Hinton. I don't know why.

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
                    
                    //limbp_face always uses CPU
                    if( !libmp_face->Process2(inputImage) )
                    {
                        std::cerr << "libmp_face Process() failed!" << std::endl;
                        break;
                    }

                    //limbp_hand always uses CPU
                    if( !libmp_hand->Process2(inputImage) )
                    {
                        std::cerr << "libmp_hand Process() failed!" << std::endl;
                        break;
                    }


                    //Draw Pose landmarks
                    mtx_UpdateOutFrame.lock();
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

                        //only draw points
                        /*
                        for (int pose_num = 0; pose_num < num_poses; pose_num++) {
                            for (const std::array<float, 3>& norm_xyz : NL_pose[pose_num]) {
                                int x = static_cast<int>(norm_xyz[0] * inputImage.cols);
                                int y = static_cast<int>(norm_xyz[1] * inputImage.rows);
                                cv::circle(outFrame, cv::Point(x, y), 3, cv::Scalar(255, 255, 0), -1);
                            }
                        }
                        */
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
                    std::vector<std::vector<std::array<float, 3>>> NL_faces;   //normalized_landmarks;
                    NL_faces = get_landmarks_face(libmp_face);
                    bool bDrawImageByOurOwn = true;
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

                    //Draw hand landmarks
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

                    //Crop the face regions and do facial expression recognition
                    if( m_bRecognizeFacialExpression && !NL_faces.empty() )
                    {
                        size_t num_faces = NL_faces.size();
                        for (int face_num = 0; face_num < num_faces; face_num++) {
                            //crop the face region.
                            //Why is the cropped region too small?
                            Mat face = CropRegion(inputImage, NL_faces[face_num]);
                            //Debug 2025 Nov 5: Here is the reason that Hinton frozens.
                            //I sitll don't know why. But if I disable this imshow(), Hinton works fine.
//                            cv::imshow("Cropped face", face);

                            auto res = fer->predictEmotions(face, false);       //false will return the softmax scores
                            Rect roi = GetBoundingBoxFromLandmarks(NL_faces[face_num], inputImage.cols, inputImage.rows);
                            //not very accurate, the cropped face is too small, around 135x156.
//                            cout << "Cropped size: " << face.cols << " " << face.rows << endl;
                            cv::putText(outFrame, res.labels[0] + std::format("{:.3f}", res.scores[0]) , Point(roi.x, roi.y) , cv::FONT_HERSHEY_SIMPLEX, 1. , cv::Scalar(0,255,0), 2);



                            //Get face recognition features
                            //Although there is only one face, the dlib face recognition model needs a vector of faces as input.
                            std::vector<dlib::matrix<dlib::rgb_pixel>> faces;
                            dlib::matrix<dlib::rgb_pixel> dlib_face;
                            dlib::assign_image(dlib_face, dlib::cv_image<dlib::bgr_pixel>(face));
                            faces.push_back(dlib_face);
                            
                            //Here is a restriction. The input size needs to be 150x150
                            //So we need to resize the face image first.
                            // Temporary workaround: skip this step if the face size is not correct.
                            /*
                            std::vector<matrix<float,0,1>> face_descriptors = net(faces);
                            //It is a vector of 128D
                            //print it out
                            cout << "face descriptor for one face: " << dlib::trans(face_descriptors[0]) << endl;
                            */
                            //how to create a cluster of the face descriptors for face recognition?
                            //no idea now.
                        }
                    }

                    mtx_UpdateOutFrame.unlock();

                    //This variable is used to prevent the robot from sending new commands while the previous command is being executed.
                    if( mbWatchPatient )
                    {
                        if( !NL_pose.empty())      //If there is no person detected, the following code will not be executed.
                        {
                            iNoPersonFrameCount = 0;

                            bLastLandmarksEffective = true;
                            //use time control first, wait for 1 seconds
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
                    }
                    mtx_Task.unlock();    
                }
                else
                {
                    mtx_UpdateOutFrame.lock();
                    inputImage.copyTo(outFrame);
                    bNewoutFrame = true;
                    mtx_UpdateOutFrame.unlock();
                }
            }    //if bCorrectlyDecoded

            //debug code, to messure the processing time
            bool bShowTransmittedImage = false;
            if( bShowTransmittedImage )
            {
                auto stop = std::chrono::high_resolution_clock::now();
                auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
                std::cout << "Elapsed time: " << duration_ms.count() << " milliseconds" << std::endl;
            }

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