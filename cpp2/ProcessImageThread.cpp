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
#include "RobotStatus.hpp"
#include "ActionOption.hpp"


#include "libmp.h"

// Compiled protobuf headers for MediaPipe types used
#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/formats/image_format.pb.h"
#include <google/protobuf/text_format.h>

#include "GetLandmarks.hpp"
#include "FaceLandmarkToZenboAction.hpp"

//using namespace human_pose_estimation;
using namespace cv;

RobotStatus robot_status;
ActionOption action_option;

cv::Mat outFrame;
bool bNewoutFrame;

ProcessImageThread::ProcessImageThread()
{
}

void ProcessImageThread::run()
{
    std::string str_home_path(getenv("HOME"));

const char* graph = R"(
		# MediaPipe graph that performs face mesh with TensorFlow Lite on CPU.

		# Input image. (ImageFrame)
		input_stream: "input_video"

		# Output image with rendered results. (ImageFrame)
		output_stream: "output_video"
		# Collection of detected/processed faces, each represented as a list of
		# landmarks. (std::vector<NormalizedLandmarkList>)
		output_stream: "multi_face_landmarks"

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
		}

		# Defines side packets for further use in the graph.
		node {
			calculator: "ConstantSidePacketCalculator"
			output_side_packet: "PACKET:0:num_faces"
			output_side_packet: "PACKET:1:use_prev_landmarks"
			output_side_packet: "PACKET:2:with_attention"
			node_options: {
				[type.googleapis.com/mediapipe.ConstantSidePacketCalculatorOptions]: {
#					packet { int_value: 1 }
					packet { int_value: 3 }
					packet { bool_value: true }
					packet { bool_value: true }
				}
			}
		}
		# Subgraph that detects faces and corresponding landmarks.
		node {
			calculator: "FaceLandmarkFrontCpu"
			input_stream: "IMAGE:throttled_input_video"
			input_side_packet: "NUM_FACES:num_faces"
			input_side_packet: "USE_PREV_LANDMARKS:use_prev_landmarks"
			input_side_packet: "WITH_ATTENTION:with_attention"
			output_stream: "LANDMARKS:multi_face_landmarks"
			output_stream: "ROIS_FROM_LANDMARKS:face_rects_from_landmarks"
			output_stream: "DETECTIONS:face_detections"
			output_stream: "ROIS_FROM_DETECTIONS:face_rects_from_detections"
		}
		# Subgraph that renders face-landmark annotation onto the input image.
		node {
			calculator: "FaceRendererCpu"
			input_stream: "IMAGE:throttled_input_video"
			input_stream: "LANDMARKS:multi_face_landmarks"
			input_stream: "NORM_RECTS:face_rects_from_landmarks"
			input_stream: "DETECTIONS:face_detections"
			output_stream: "IMAGE:output_video"
		}

)";

    std::shared_ptr<mediapipe::LibMP> libmp(mediapipe::LibMP::Create(graph, "input_video"));
	libmp->AddOutputStream("output_video");
	libmp->AddOutputStream("multi_face_landmarks");
	libmp->Start();

    auto previous_time = std::chrono::high_resolution_clock::now();

    while(b_WhileLoop)
    {
        if( pSocketHandler->get_queue_length() > 0 )    //here is an infinite loop
        {

            //Get message from the queue
            Message message = pSocketHandler->get_head();
            pSocketHandler->pop_head();
            char *data_ = message.data.get();
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

//            cout << "iJPEG_length " << iJPEG_length << endl;
//            cout << "JPEG length check " << message.length - 24 << endl;

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

            //2024/6/8 Report result back to Zenbo so it can take actions.
//            ZenboNurseHelperProtobuf::ReportAndCommand report_data;
            string header(data_);
            string str_timestamp = header.substr(0,13);
            string str_pitch_degree = header.substr(14,3);
//            cout << "str_timestamp " << str_timestamp << endl;
//            cout << "str_pitch_degree " << str_pitch_degree << endl;

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
//            report_data.set_time_stamp(timestamp);
//            report_data.set_pitch_degree(pitch_degree);
            //2025/3/9 Bug note: my previous end argument is wrong: data_+iJPEG_length where "+30" is missing.
            //In OpenCV 4.6, imdecode still works, but in OpenCV 4.11 and 4.12, it fails.
            //That is the reason that in my imshow() output window, the bottom region is always blurred.
            //The reason is that the imdecode() function fails to decode the JPEG image. 
            vector<uchar> JPEG_Data(data_ + shift_length, data_+shift_length+iJPEG_length);

            bool bCorrectlyDecoded = false;
            Mat inputImage;
            try{
                inputImage = imdecode(JPEG_Data, IMREAD_COLOR);
                if( inputImage.data )
                    bCorrectlyDecoded = true;
                else
                {
                    cout << "imdecode fails." << std::endl;
                    continue;
                }
            }
            catch(exception &e)
            {
                cout << "Received JPEG frame are corrupt although the signature is correct." << std::endl;
            }

            if( bCorrectlyDecoded)
            {
                bNewoutFrame = true;

                if(bSaveTransmittedImage)
                {
                    //2025/1/7 How to change the timestamp to a meaningful filename?
                    string filename = raw_images_directory + "/" + str_timestamp + ".jpg";
                    save_image_JPEG(data_ + shift_length, iJPEG_length , filename);
//                    std::cout << filename << std::endl;
                }

                if( b_HumanPoseEstimation)
                {
                    if (!libmp->Process(inputImage.data, inputImage.cols, inputImage.rows, mediapipe::ImageFormat::SRGB)) {
                        std::cerr << "Process() failed!" << std::endl;
                        break;
                    }
                    libmp->WaitUntilIdle();

                    if( libmp->WriteOutputImage(outFrame.data, libmp->GetOutputPacket("output_video")) )
                    {
                    }
                    else
                    {
                        cout << "WriteOutputImage fails." << std::endl;
                        continue;
                    }

                    std::vector<std::vector<std::array<float, 3>>> normalized_landmarks = get_landmarks(libmp);
                    if (normalized_landmarks.empty()) {
//                        std::cerr << "No landmarks detected!" << std::endl;
//                        continue;
                    }
                    else
                    {
//                        std::cout << "Detected " << normalized_landmarks.size() << " faces." << std::endl;

                        //2025/3/29 I need a component to generate Zenbo's action from the landmarks.
                        //use time control first, wait for 5 seconds
                        auto current_time = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::seconds>(current_time - previous_time);
                        if (duration.count() >= 5) {
                            ZenboNurseHelperProtobuf::ReportAndCommand message;
                            FaceLandmarks_to_ZenboAction(normalized_landmarks, robot_status, action_option, message);
                            previous_time = current_time;
                            pSendCommandThread->AddMessage(message);
                            pSendCommandThread->cond_var_report_result.notify_one();
                            std::string str_out;
                            google::protobuf::TextFormat::PrintToString(message, &str_out);
                            std::cout << str_out << std::endl;
                            std::cout << "Yaw degree " << robot_status.yaw_degree << std::endl;
                            std::cout << "Pitch degree " << robot_status.pitch_degree << std::endl;
                        }
                    }


                    for( unsigned int idx = 0; idx < 0; idx++ )
                    {
//                        HumanPose pose = poses[idx];
//                        ZenboNurseHelperProtobuf::ReportAndCommand::OpenPosePose *pPose = report_data.add_pose();
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
                }
                else
                {
                    inputImage.copyTo(outFrame);
                }
            }
        }
        else
        {
            //wait until being notified
            //std::unique_lock<std::mutex> lk(mtx);
            //cond_var_process_image.wait(lk);
        }
    }
    cout << "Exit while loop." << std::endl;
}
