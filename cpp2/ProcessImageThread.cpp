#include "ProcessImageThread.hpp"
//#include "human_pose_estimator.hpp"
#include "utility_TimeRecorder.hpp"
#include "utility_directory.hpp"
#include "utility_string.hpp"
#include "utility_csv.hpp"
#include <numeric>      // std::iota
#include "JPEG.hpp"
#include "ServerSend.pb.h"
//#include "Pose.hpp"
#include "utility_directory.hpp"
#include <opencv2/opencv.hpp>

#include "libmp.h"

// Compiled protobuf headers for MediaPipe types used
#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/formats/image_format.pb.h"


//using namespace human_pose_estimation;
using namespace cv;

cv::Mat outFrame;
bool bNewoutFrame;

ProcessImageThread::ProcessImageThread()
{
    frame_buffer1 = std::make_unique<char[]>(buffer_size);
}

typedef pair<float,int> mypair;
bool comparator ( const mypair& l, const mypair& r)
   { return l.first < r.first; }

int ProcessImageThread::get_buffer_size()
{
    return buffer_size;
}

void ProcessImageThread::run()
{
    //2024/12/27 this is hard-coded, incorrect.
    std::string str_home_path(getenv("HOME"));
//    std::string pose_model_path = str_home_path + "/open_model_zoo/models/intel/human-pose-estimation-0001/FP32/human-pose-estimation-0001.xml";
//    HumanPoseEstimator estimator(pose_model_path);


const char* graph = R"(
    # CPU image. (ImageFrame)
input_stream: "input_video"

# My added output
#input_stream: "FLOATS:output_landmark_vector"
#output_stream: "output_landmark_vector"


# CPU image with rendered results. (ImageFrame)
output_stream: "output_video"


# Gets image size.
node {
  calculator: "ImagePropertiesCalculator"
  input_stream: "IMAGE:throttled_input_video"
  output_stream: "SIZE:image_size"
}

# Throttles the images flowing downstream for flow control. It passes through
# the very first incoming image unaltered, and waits for downstream nodes
# (calculators and subgraphs) in the graph to finish their tasks before it
# passes through another image. All images that come in while waiting are
# dropped, limiting the number of in-flight images in most part of the graph to
# 1. This prevents the downstream nodes from queuing up incoming images and data
# excessively, which leads to increased latency and memory usage, unwanted in
# real-time mobile applications. It also eliminates unnecessarily computation,
# e.g., the output produced by a node may get dropped downstream if the
# subsequent nodes are still busy processing previous inputs.
node {
  calculator: "FlowLimiterCalculator"
  input_stream: "input_video"
  input_stream: "FINISHED:output_video"
  input_stream_info: {
    tag_index: "FINISHED"
    back_edge: true
  }
  output_stream: "throttled_input_video"
  node_options: {
    [type.googleapis.com/mediapipe.FlowLimiterCalculatorOptions] {
      max_in_flight: 1
      max_in_queue: 1
      # Timeout is disabled (set to 0) as first frame processing can take more
      # than 1 second.
      in_flight_timeout: 0
    }
  }
}

# Converts pose, hands and face landmarks to a render data vector.
node {
  calculator: "HolisticTrackingToRenderData"
  input_stream: "IMAGE_SIZE:image_size"
  input_stream: "POSE_LANDMARKS:pose_landmarks"
  input_stream: "POSE_ROI:pose_roi"
  input_stream: "LEFT_HAND_LANDMARKS:left_hand_landmarks"
  input_stream: "RIGHT_HAND_LANDMARKS:right_hand_landmarks"
  input_stream: "FACE_LANDMARKS:face_landmarks"
  output_stream: "RENDER_DATA_VECTOR:render_data_vector"
  output_stream: "FLOATS:output_landmark_vector"
}

# Draws annotations and overlays them on top of the input images.
node {
  calculator: "AnnotationOverlayCalculator"
  input_stream: "IMAGE:throttled_input_video"
  input_stream: "VECTOR:render_data_vector"
  output_stream: "IMAGE:output_video"
}

# Tracks and renders pose + hands + face landmarks.
node {
  calculator: "HolisticLandmarkCpu"
  input_stream: "IMAGE:throttled_input_video"
  output_stream: "POSE_LANDMARKS:pose_landmarks"
  output_stream: "POSE_ROI:pose_roi"
  output_stream: "POSE_DETECTION:pose_detection"
  output_stream: "FACE_LANDMARKS:face_landmarks"
  output_stream: "LEFT_HAND_LANDMARKS:left_hand_landmarks"
  output_stream: "RIGHT_HAND_LANDMARKS:right_hand_landmarks"
}
)";

    std::shared_ptr<mediapipe::LibMP> face_mesh(mediapipe::LibMP::Create(graph, "input_video"));
	face_mesh->AddOutputStream("output_video");
	face_mesh->Start();

    mutex m;
    unique_lock<mutex> lock(m);

    while(b_WhileLoop)
    {
//        cond_var_process_image.wait(lock);
        if( b_frame_buffer1_unused )    //here is an infinite loop
        {
            //frame_buffer1 should be protected by mutex here.
            mutex_frame_buffer1.lock();
            char *data_ = frame_buffer1.get();

            string heading(data_);

            //Check the correctness of this frame buffer
            if( heading.length() != 23){
                cout << "heading length incorrect'" << endl;
                b_frame_buffer1_unused = false;
                mutex_frame_buffer1.unlock();
                continue;
            }

            if( heading.substr(0,6) != "Begin:"){
                cout << "Beginning is not 'Begin:'" << endl;
                b_frame_buffer1_unused = false;
                mutex_frame_buffer1.unlock();
                continue;
            }

            string sJPEG_length(data_+heading.length()+1);
            int iJPEG_length = 0;
            try{
                iJPEG_length = stoi(sJPEG_length);
            }
            catch(exception &e){
                b_frame_buffer1_unused = false;
                mutex_frame_buffer1.unlock();
                cout << "Convert sJPEG_length to iJPEG_length fails" << endl;
                continue;
            }

            //check if length correct
            if( iJPEG_length + 41 != frame_buffer1_length){
                b_frame_buffer1_unused = false;
                mutex_frame_buffer1.unlock();
                cout << "Buffer length does not match heading plus JPEG data" << endl;
                cout << "frame_buffer1_length " << frame_buffer1_length << endl;
                cout << "iJPEG_length+41 " << iJPEG_length + 41 << endl;
                continue;
            }

            //check JPEG signature
            if( !(static_cast<int>(static_cast<unsigned char>(data_[30])) == 0xFF &&
                static_cast<int>(static_cast<unsigned char>(data_[31])) == 0xD8 &&
                static_cast<int>(static_cast<unsigned char>(data_[32])) == 0xFF &&
                static_cast<int>(static_cast<unsigned char>(data_[30+iJPEG_length-2])) == 0xFF &&
                static_cast<int>(static_cast<unsigned char>(data_[30+iJPEG_length-1])) == 0xD9 ))
            {
                b_frame_buffer1_unused = false;
                mutex_frame_buffer1.unlock();
                cout << "JPEG signature does not match" << endl;
                continue;
            }

            //2024/6/8 Report result back to Zenbo so it can take actions.
            ZenboNurseHelperProtobuf::ReportAndCommand report_data;
            string header(data_);
            string str_timestamp = header.substr(6,13);
            string str_pitch_degree = header.substr(20,2);
            long timestamp = 0;
            int pitch_degree = 0;
            try{
                timestamp = stol(str_timestamp);                
                pitch_degree = stoi(str_pitch_degree);
            }
            catch(exception &e)
            {
                throw("cannot do stol");
            }
            report_data.set_time_stamp(timestamp);
            report_data.set_pitch_degree(pitch_degree);
            //2025/3/9 Bug note: my previous end argument is wrong: data_+iJPEG_length where "+30" is missing.
            //In OpenCV 4.6, imdecode still works, but in OpenCV 4.11 and 4.12, it fails.
            //That is the reason that in my imshow() output window, the bottom region is always blurred.
            //The reason is that the imdecode() function fails to decode the JPEG image. 
            vector<uchar> JPEG_Data(data_ + 30, data_+30+iJPEG_length);

            bool bCorrectlyDecoded = false;
            Mat inputImage;
            try{
                inputImage = imdecode(JPEG_Data, IMREAD_COLOR);
                if( inputImage.data )
                    bCorrectlyDecoded = true;
                else
                {
                    cout << "imdecode fails." << std::endl;
                    b_frame_buffer1_unused = false;
                    mutex_frame_buffer1.unlock();
                    continue;
                }
            }
            catch(exception &e)
            {
                cout << "Received JPEG frame are corrupt although the signature is correct." << std::endl;
            }

            if( bCorrectlyDecoded)
            {
//                cout << "bCorrectlyDecoded." << std::endl;
                bNewoutFrame = true;
//                inputImage.copyTo(outFrame);

                if(bSaveTransmittedImage)
                {
                    //2025/1/7 How to change the timestamp to a meaningful filename?
                    string filename = raw_images_directory + "/" + str_timestamp + ".jpg";
                    save_image_JPEG(data_ + 30, iJPEG_length , filename);
//                    std::cout << filename << std::endl;
                }

                if( b_HumanPoseEstimation)
                {
//                    vector<HumanPose> poses = estimator.estimate(inputImage );
                    //This function is written in the Pose.cpp
//                    poses = SortPosesByHeight(poses);
                    //Do I need to convert color channels?
//                    cv::Mat frame_rgb;
//                    cv::cvtColor(inputImage, frame_rgb, cv::COLOR_BGR2RGB);

                    if (!face_mesh->Process(inputImage.data, inputImage.cols, inputImage.rows, mediapipe::ImageFormat::SRGB)) {
                //    if (!face_mesh->Process(frame_rgb.data, frame_rgb.cols, frame_rgb.rows, mediapipe::ImageFormat::SRGB)) {
                        std::cerr << "Process() failed!" << std::endl;
                        break;
                    }
//                    face_mesh->WaitUntilIdle();

                    if( face_mesh->WriteOutputImage(outFrame.data, face_mesh->GetOutputPacket("output_video")) )
                    {
//                        cout << "outFrame." << std::endl;
                    }
                    else
                    {
                        cout << "WriteOutputImage fails." << std::endl;
                        b_frame_buffer1_unused = false;
                        mutex_frame_buffer1.unlock();
                        continue;
                    }

//                    for( unsigned int idx = 0; idx < poses.size(); idx++ )
                    for( unsigned int idx = 0; idx < 0; idx++ )
                    {
//                        HumanPose pose = poses[idx];
                        ZenboNurseHelperProtobuf::ReportAndCommand::OpenPosePose *pPose = report_data.add_pose();
                        //This line should be modified.
//                        pPose->set_score(static_cast<long>(pose.score * 2147483647));
/*
                        for( auto keypoint : pose.keypoints)
                        {
                            ZenboNurseHelperProtobuf::ReportAndCommand::OpenPosePose::OpenPoseCoordinate *pCoord = pPose->add_coord();
                            if(keypoint.x == -1 && keypoint.y == -1)
                            {
                                pCoord->set_x(0);
                                pCoord->set_y(0);
                                pCoord->set_valid(0);
                            }
                            else
                            {
                                pCoord->set_x(static_cast<long>(keypoint.x));
                                pCoord->set_y(static_cast<long>(keypoint.y));
                                pCoord->set_valid(1);
                            }
                        }
*/
                    }
                    pSendCommandThread->AddMessage(report_data);

//                    long byteSize = report_data.ByteSizeLong();
//                    if( byteSize <= 4096)
//                    {
//                        pSendCommandThread->str_results_len = byteSize;
//                        report_data.SerializeToArray(pSendCommandThread->str_results,byteSize);
//                    }else
//                        throw( "report_data too large.");

                    b_frame_buffer1_unused = false;
//                    pSendCommandThread->cond_var_report_result.notify_one();
                }
                else
                {
                    inputImage.copyTo(outFrame);
//                    outFrame = inputImage;
                    bNewoutFrame = true;
                    b_frame_buffer1_unused = false;
                }
            }
            mutex_frame_buffer1.unlock();
        }
    }
    cout << "Exit while loop." << std::endl;
}
