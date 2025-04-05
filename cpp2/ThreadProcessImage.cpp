#include "ThreadProcessImage.hpp"
#include "utility_TimeRecorder.hpp"
#include "utility_directory.hpp"
#include "utility_string.hpp"
#include "utility_csv.hpp"
#include <numeric>      // std::iota
#include "JPEG.hpp"
#include "ServerSend.pb.h"
#include "utility_directory.hpp"
#include <opencv2/opencv.hpp>
#include "RobotStatus.hpp"
#include "ActionOption.hpp"

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

ThreadProcessImage::ThreadProcessImage()
{
    Processor = "CPU";
    Task = "None";
}

void ThreadProcessImage::setProcessor(std::string processor)
{
    bool ChangeProcessor = false;
    if( processor == "CPU" )
    {
        if( Processor != "CPU" )
        {
            ChangeProcessor = true;
        }
        Processor = "CPU";
    }
    else if( processor == "GPU" )
    {
        if( Processor != "GPU" )
        {
            ChangeProcessor = true;
        }
        Processor = "GPU";
    }
    else
    {
        cout << "Processor is not supported." << endl;
    }

    if( ChangeProcessor )
    {
        reloadGraph();
    }
    else
    {
        cout << "Processor is not changed." << endl;
    }
}

void ThreadProcessImage::setTask(std::string task)
{
    bool ChangeTask = false;
    //2025/4/5 Debug info: the setTask() function is called by the main thread. Thus, the Task member variable can change immeidately even when
    //the ThreadProcessImage thread is running. As a result, the get_landmarks() and get_landmarks_pose() may be wrongly called and lead
    //to an out-of-range exception from unordered_map.at()
    mtx_Task.lock();
    if( task == "Pose" )
    {
        if( Task != "Pose" )
        {
            ChangeTask = true;
        }
        Task = "Pose";
    }
    else if(task == "Face")
    {
        if( Task != "Face" )
        {
            ChangeTask = true;
        }
        Task = "Face";
    }
     else if(task == "Holistic")
    {
        if( Task != "Holistic" )
        {
            ChangeTask = true;
        }
        Task = "Holistic";
    }
    else if(task == "None")
    {
        if( Task != "None" )
        {
            ChangeTask = true;
        }
        Task = "None";
    }
    else
    {
        cout << "Task is not supported." << endl;
    }

    if( ChangeTask )
    {
        reloadGraph();
    }
    else
    {
        cout << "Task is not changed." << endl;
    }
    mtx_Task.unlock();
}

void ThreadProcessImage::reloadGraph()
{
    if( Processor == "CPU" )
    {
        if( Task == "Pose" )
        {
            graph_string = R"(
                # MediaPipe graph that performs pose tracking with TensorFlow Lite on CPU.

                # CPU buffer. (ImageFrame)
                input_stream: "input_video"
                
                # Output image with rendered results. (ImageFrame)
                output_stream: "output_video"
                # Pose landmarks. (NormalizedLandmarkList)
                output_stream: "pose_landmarks"
                
                # Generates side packet to enable segmentation.
                node {
                    calculator: "ConstantSidePacketCalculator"
                    output_side_packet: "PACKET:enable_segmentation"
                    node_options: {
                        [type.googleapis.com/mediapipe.ConstantSidePacketCalculatorOptions]: {
    #                    packet { bool_value: true }
                        packet { bool_value: false }
                        }
                    }
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
                }
                
                # Subgraph that detects poses and corresponding landmarks.
                node {
                    calculator: "PoseLandmarkCpu"
                    input_side_packet: "ENABLE_SEGMENTATION:enable_segmentation"
                    input_stream: "IMAGE:throttled_input_video"
                    output_stream: "LANDMARKS:pose_landmarks"
                    output_stream: "SEGMENTATION_MASK:segmentation_mask"
                    output_stream: "DETECTION:pose_detection"
                    output_stream: "ROI_FROM_LANDMARKS:roi_from_landmarks"
                }
                
                # Subgraph that renders pose-landmark annotation onto the input image.
                node {
                    calculator: "PoseRendererCpu"
                    input_stream: "IMAGE:throttled_input_video"
                    input_stream: "LANDMARKS:pose_landmarks"
                    input_stream: "SEGMENTATION_MASK:segmentation_mask"
                    input_stream: "DETECTION:pose_detection"
                    input_stream: "ROI:roi_from_landmarks"
                    output_stream: "IMAGE:output_video"
                }
            )";
        }
        else if( Task == "Face")
        {
            graph_string = R"(
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
        #                   packet { int_value: 1 }
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
        }
        else if (Task == "Holistic")
        {
            graph_string = R"(
                # Tracks and renders pose + hands + face landmarks.

                # CPU image. (ImageFrame)
                input_stream: "input_video"

                # CPU image with rendered results. (ImageFrame)
                output_stream: "output_video"

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

                # Gets image size.
                node {
                calculator: "ImagePropertiesCalculator"
                input_stream: "IMAGE:throttled_input_video"
                output_stream: "SIZE:image_size"
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
                }

                # Draws annotations and overlays them on top of the input images.
                node {
                calculator: "AnnotationOverlayCalculator"
                input_stream: "IMAGE:throttled_input_video"
                input_stream: "VECTOR:render_data_vector"
                output_stream: "IMAGE:output_video"
                }
            )";
        }
        else
        {
            cout << "Task is not supported." << endl;
        }
    }
    else if(Processor == "GPU")
    {
        if( Task == "Pose" )
        {
            graph_string = R"(
                # MediaPipe graph that performs pose tracking with TensorFlow Lite on GPU.

                # GPU buffer. (GpuBuffer)
                input_stream: "input_video"

                # Output image with rendered results. (GpuBuffer)
                output_stream: "output_video"
                # Pose landmarks. (NormalizedLandmarkList)
                output_stream: "pose_landmarks"

                # Generates side packet to enable segmentation.
                node {
                calculator: "ConstantSidePacketCalculator"
                output_side_packet: "PACKET:enable_segmentation"
                node_options: {
                    [type.googleapis.com/mediapipe.ConstantSidePacketCalculatorOptions]: {
#                    packet { bool_value: true }
                    packet { bool_value: false }
                    }
                }
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
                }

                # Subgraph that detects poses and corresponding landmarks.
                node {
                calculator: "PoseLandmarkGpu"
                input_side_packet: "ENABLE_SEGMENTATION:enable_segmentation"
                input_stream: "IMAGE:throttled_input_video"
                output_stream: "LANDMARKS:pose_landmarks"
                output_stream: "SEGMENTATION_MASK:segmentation_mask"
                output_stream: "DETECTION:pose_detection"
                output_stream: "ROI_FROM_LANDMARKS:roi_from_landmarks"
                }

                # Subgraph that renders pose-landmark annotation onto the input image.
                node {
                calculator: "PoseRendererGpu"
                input_stream: "IMAGE:throttled_input_video"
                input_stream: "LANDMARKS:pose_landmarks"
                input_stream: "SEGMENTATION_MASK:segmentation_mask"
                input_stream: "DETECTION:pose_detection"
                input_stream: "ROI:roi_from_landmarks"
                output_stream: "IMAGE:output_video"
                }
            )";
        }
    }
    libmp.reset(mediapipe::LibMP::Create(graph_string.c_str(), "input_video"));
	libmp->AddOutputStream("output_video");
    if(Task == "Face")
        libmp->AddOutputStream("multi_face_landmarks");    //Maybe this function is not a real-time function
    else if(Task == "Pose")
        libmp->AddOutputStream("pose_landmarks");
    libmp->Start();
}

void ThreadProcessImage::run()
{
    std::string str_home_path(getenv("HOME"));

    auto previous_time = std::chrono::high_resolution_clock::now();

    while(b_WhileLoop)
    {
        if( pSocketHandler->get_queue_length() > 0 )    //here is an infinite loop
        {

            auto start = std::chrono::high_resolution_clock::now();
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

                auto stop = std::chrono::high_resolution_clock::now();
                auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
                std::cout << "Elapsed time: " << duration_ms.count() << " milliseconds" << std::endl;

                if( b_HumanPoseEstimation)
                {
                    start = std::chrono::high_resolution_clock::now();

                    mtx_Task.lock();
                    //This Process function only works for the CPU mode because the GPU mode uses the GpuBuffer.
                    if( Processor == "CPU" )
                    {
                        if( !libmp->Process(inputImage.data, inputImage.cols, inputImage.rows, mediapipe::ImageFormat::SRGB) )
                        {
                            std::cerr << "Process() failed!" << std::endl;
                            break;
                        }
                    }
                    else if( Processor == "GPU" )
                    {
                        //2025/1/7 Bug note: the GPU mode uses GpuBuffer, so the Process function is not needed.
                        /*
                        if( !libmp->Process_GPU(inputImage.data, inputImage.cols, inputImage.rows, mediapipe::ImageFormat::SRGB) )
                        {
                            std::cerr << "Process_GPU() failed!" << std::endl;
                            break;
                        }
                        */
                    }
                    else
                    {
                        cout << "Processor is not supported." << endl;
                    }

                    if( Processor == "CPU" )
                    {
                        if( libmp->WriteOutputImage(outFrame.data, libmp->GetOutputPacket("output_video")) )
                        {
                        }
                        else
                        {
                            cout << "WriteOutputImage fails." << std::endl;
                        }
                    }
                    else if( Processor == "GPU" )
                    {
                        /*
                        if( libmp->WriteOutputImage_GPU(outFrame.data, libmp->GetOutputPacket("output_video")) )
                        {
                        }
                        else
                        {
                            cout << "WriteOutputImage fails." << std::endl;
                        }
                        */
                    }

                    stop = std::chrono::high_resolution_clock::now();                    
                    duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
                    std::cout << "Process time: " << duration_ms.count() << " milliseconds" << std::endl;
                        
                    std::vector<std::vector<std::array<float, 3>>> normalized_landmarks;
                    if( Task == "Face" ) 
                    {
                        normalized_landmarks = get_landmarks(libmp);
                    }
                    else if( Task == "Pose" )
                    {
                        normalized_landmarks = get_landmarks_pose(libmp);
                    }
                    else if( Task == "Holistic" )
                    {
                        //normalized_landmarks = get_landmarks_holistic(libmp);
                    }
                    else
                    {
                        cout << "Task is not supported." << endl;
//                        continue;
                    }

                    if (normalized_landmarks.empty()) {
//                        std::cerr << "No landmarks detected!" << std::endl;
//                        continue;
                    }
                    else
                    {
                        //use time control first, wait for 5 seconds
                        auto current_time = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::seconds>(current_time - previous_time);
                        if (duration.count() >= 5) {
                            ZenboNurseHelperProtobuf::ReportAndCommand message;
                            if( Task == "Face" )
                                FaceLandmarks_to_ZenboAction(normalized_landmarks, robot_status, action_option, message);
                            else if( Task == "Pose" )
                            {
                                PoseLandmarks_to_ZenboAction(normalized_landmarks, robot_status, action_option, message);
                            }
                            else if( Task == "Holistic" )
                            {
                                //HolisticLandmarks_to_ZenboAction(normalized_landmarks, robot_status, action_option, message);
                            }
                            else
                            {
                                cout << "Task is not supported." << endl;
//                                continue;
                            }
//                            FaceLandmarks_to_ZenboAction(normalized_landmarks, robot_status, action_option, message);
                            previous_time = current_time;
                            pThreadSendCommand->AddMessage(message);
                            pThreadSendCommand->cond_var_report_result.notify_one();
                        }
                    }
                    mtx_Task.unlock();    
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
            std::unique_lock<std::mutex> lk(mtx);
            cond_var_process_image.wait(lk);
        }
    }
    cout << "Exit while loop." << std::endl;
}
