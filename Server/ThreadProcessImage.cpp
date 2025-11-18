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
#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/formats/image_format.pb.h"

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
        if( pSocketHandler->get_queue_length() > 0 )    //here is an infinite loop
        {
            auto start = std::chrono::high_resolution_clock::now();
            //Get message from the queue
            //Only need the latest message
            DataFrame dataframe;
            while (pSocketHandler->get_queue_length() > 0)
            {
                dataframe = pSocketHandler->get_head();
                pSocketHandler->pop_head();    
            }
            char *data_ = dataframe.data.get();
            
            //Here, I need to parse the protobuf object
            //I don't know why it does not work.
//            RobotCommandProtobuf::RobotToServerMessage RTSmessage;
//            RTSmessage.ParseFromString(data_);
            bool bCorrectlyDecoded = false;
//            if( RTSmessage.has_jpegdata() && RTSmessage.has_jpegdatalength())
//            {
//                google::protobuf::Timestamp timestamp = RTSmessage.event_time();
//                cout << "Receive an Image at " << timestamp.seconds() << " " << timestamp.nanos() << endl;
//                string strJPEG_Data = RTSmessage.jpegdata();
//                vector<uchar> JPEG_Data(strJPEG_Data.begin(), strJPEG_Data.end());
//                int iJPEG_length = RTSmessage.jpegdatalength();


            string heading(data_);

            //Check the correctness of this frame buffer
            if( heading.length() != 17){
                cout << "heading length incorrect'" << endl;
                continue;
            }


            string sJPEG_length(data_+heading.length()+1);
            int iJPEG_length = 0;
            try{
                iJPEG_length = stoi(sJPEG_length);
            }
            catch(exception &e){
                cout << "Convert sJPEG_length to iJPEG_length fails" << endl;
                continue;
            }

            //check JPEG signature
            int shift_length = 13 + 1 + 3 + 1 + sJPEG_length.length() + 1;
            if( !(static_cast<int>(static_cast<unsigned char>(data_[shift_length])) == 0xFF &&
                static_cast<int>(static_cast<unsigned char>(data_[shift_length+1])) == 0xD8 &&
                static_cast<int>(static_cast<unsigned char>(data_[shift_length+2])) == 0xFF 
                && static_cast<int>(static_cast<unsigned char>(data_[shift_length+iJPEG_length-2])) == 0xFF
               && static_cast<int>(static_cast<unsigned char>(data_[shift_length+iJPEG_length-1])) == 0xD9 
           ))
            {
                cout << static_cast<int>(static_cast<unsigned char>(data_[shift_length])) << endl;
                cout << static_cast<int>(static_cast<unsigned char>(data_[shift_length+1])) << endl;
                cout << static_cast<int>(static_cast<unsigned char>(data_[shift_length+2])) << endl;
                cout << static_cast<int>(static_cast<unsigned char>(data_[shift_length+iJPEG_length-2])) << endl;
                cout << static_cast<int>(static_cast<unsigned char>(data_[shift_length+iJPEG_length-1])) << endl;
                cout << "JPEG signature does not match" << endl;
                continue;
            }

            string header(data_);
            string str_timestamp = header.substr(0,13);
            string str_is_dancing = header.substr(14,3);

            long timestamp = 0;
            is_dancing = 0;             //Here is a logical problem. While Kebbi is dancing, I won't receive image frames, and don't know if it is still dancing.
            try{
                timestamp = stol(str_timestamp);                
                is_dancing = stoi(str_is_dancing);   //2025 Aug 5: Mohamed wants the server-side program to know that the robot is dancing.
            }
            catch(exception &e)
            {
                throw("cannot do stol");
            }
            //2025/3/9 Bug note: my previous end argument is wrong: data_+iJPEG_length where "+30" is missing.
            //In OpenCV 4.6, imdecode still works, but in OpenCV 4.11 and 4.12, it fails.
            //That is the reason that in my imshow() output window, the bottom region is always blurred.
            //The reason is that the imdecode() function fails to decode the JPEG image. 
            std::vector<uchar> JPEG_Data(data_ + shift_length, data_+shift_length+iJPEG_length);

            try{
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
                cout << "imdecode try catch exception." << endl;
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
                        save_image_JPEG(data_ + shift_length, iJPEG_length , filename);
                        save_image_JPEG(JPEG_Data, filename);
                        iFrameCount = 0; //reset the frame count
                    }
                    else
                    {
                        iFrameCount++;
                    }
                }

                //debug code
                /*
                bool bShowTransmittedImage = false;
                if( bShowTransmittedImage )
                {
                    auto stop = std::chrono::high_resolution_clock::now();
                    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
                    std::cout << "Elapsed time: " << duration_ms.count() << " milliseconds" << std::endl;
                }
                */

                if( b_HumanPoseEstimation)
                {
                    start = std::chrono::high_resolution_clock::now();

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
                        inputImage.copyTo(outFrame);
                        size_t num_poses = NL_pose.size();
                        for (int pose_num = 0; pose_num < num_poses; pose_num++) {
                            for (const std::array<float, 3>& norm_xyz : NL_pose[pose_num]) {
                                int x = static_cast<int>(norm_xyz[0] * inputImage.cols);
                                int y = static_cast<int>(norm_xyz[1] * inputImage.rows);
                                cv::circle(outFrame, cv::Point(x, y), 3, cv::Scalar(255, 255, 0), -1);
                            }
                        }
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

                            cv::imshow("Cropped face", face);

                            auto res = fer->predictEmotions(face, false);       //false will return the softmax scores
                            Rect roi = GetBoundingBoxFromLandmarks(NL_faces[face_num], inputImage.cols, inputImage.rows);
                            //not very accurate, the cropped face is too small, around 135x156.
                            cout << "Cropped size: " << face.cols << " " << face.rows << endl;
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
        }
        else
        {
            //wait until being notified
            mutex mtx;
            unique_lock<mutex> lk(mtx);
            cond_var_process_image.wait(lk);
        }
    }
    cout << "Exit ThreadProcessImage loop." << endl;
}

void ThreadProcessImage::NotifyEvent(string description, chrono::time_point<chrono::system_clock> timestamp, float yaw, float pitch )
{
    if( description == "onCompleteOfMotionPlay")
    {
        cout << "(A) onCompleteOfMotionPlay yaw " << yaw << " pitch " << pitch << endl;
        robot_status.yaw_degree = (int)yaw;
        robot_status.pitch_degree = (int)pitch;
        mbWatchPatient = true;
    }
    else if(description == "KebbiMoveHeadDuringMotion")
    {
        cout << "(B) KebbiMoveHeadDuringMotion " << endl;
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