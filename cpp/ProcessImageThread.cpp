#include "ProcessImageThread.hpp"
#include "human_pose_estimator.hpp"
#include "utility_TimeRecorder.hpp"
#include "utility_directory.hpp"
#include "utility_string.hpp"
#include "utility_csv.hpp"
#include <numeric>      // std::iota
#include "JPEG.hpp"
#include "ServerSend.pb.h"
#include "Pose.hpp"


using namespace human_pose_estimation;
using namespace cv;

extern cv::Mat outFrame;
extern bool bNewoutFrame;

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
    std::string pose_model_path("/home/chihyuan/open_model_zoo/models/intel/human-pose-estimation-0001/FP32/human-pose-estimation-0001.xml");
    bool bSaveTransmittedImage = false;
    std::string save_to_directory("/home/chihyuan/Downloads");
    HumanPoseEstimator estimator(pose_model_path);
    string raw_images_directory = save_to_directory + "/raw_images";
    if(bSaveTransmittedImage)
        CreateDirectory(raw_images_directory);

    mutex m;
    unique_lock<mutex> lock(m);

    while(b_WhileLoop)
    {
        cond_var_process_image.wait(lock);
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
            vector<char> JPEG_Data(data_ + 30, data_+iJPEG_length);
            bool bCorrectlyDecoded = false;
            Mat inputImage;
            try{
                //Chih-Yuan Yang: imdecode is an OpenCV function. Because I use using namespace cv, the linker
                //find the imdecode() function and the IMREAD_COLOR tag.
                
                inputImage = imdecode(JPEG_Data, IMREAD_COLOR); //check this result. The image may be corrupt.
                if( inputImage.data )
                    bCorrectlyDecoded = true;
            }
            catch(exception &e)
            {
                cout << "Received JPEG frame are corrupt although the signature is correct." << std::endl;
            }

            if( bCorrectlyDecoded)
            {
                //This inputImage has little delay.
//                cv::imshow("latest image",inputImage);
                if(bSaveTransmittedImage)
                {
                    string filename = raw_images_directory + "/" + str_timestamp + ".jpg";
                    save_image_JPEG(data_ + 30, iJPEG_length , filename);
                    std::cout << filename << std::endl;
                }

                //Where is the displayImage used?
//                Mat displayImage = inputImage.clone();

                //There is a problem: The OpenVINO human pose estimator is not a fast-response
                //program. It uses a pipeline to incrase the throughput, but renders the human
                //skeleton on an old frame.
                if( b_HumanPoseEstimation)
                {
                    vector<HumanPose> poses = estimator.estimate(inputImage );
                    //This function is written in the Pose.cpp
                    poses = SortPosesByHeight(poses);
                    
                    for( unsigned int idx = 0; idx < poses.size(); idx++ )
                    {
                        HumanPose pose = poses[idx];
                        ZenboNurseHelperProtobuf::ReportAndCommand::OpenPosePose *pPose = report_data.add_pose();
                        //This line should be modified.
                        pPose->set_score(static_cast<long>(pose.score * 2147483647));
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
                    outFrame = inputImage;
                    bNewoutFrame = true;
                }
            }
            mutex_frame_buffer1.unlock();
        }
    }
}
