#pragma once

#include <string>
#include <opencv2/opencv.hpp>
#include <models/results.h>

//#include "human_pose.hpp"

//#include "tensorflow/core/framework/tensor.h"
//#include "PoseSensitiveEmbedding.hpp"

using namespace std;
//using namespace human_pose_estimation;   //for HumanPose struct

struct PoseRegion
{
    cv::Mat mat;
    HumanPose pose;             //Chih-Yuan Yang: The HumanPose is defined in results.h
    unsigned int index_in_poses;
};

int ConvertImageToPoseMaps_npy(const string& input_dir, const vector<string>& file_list, const string& output_dir, const string& pose_model);
int ConvertImageToPoses(const cv::Mat& input_image, const string& pose_model, vector<HumanPose>& poses );
//int ConvertPoseToTensor(const HumanPose& pose, tensorflow::Tensor &Tensor);
//Chih-Yuan Yang, I do not use this function
//vector<PoseRegion> CropRegionsFromPoses(const cv::Mat& inputImage, const vector<HumanPose>& poese);
cv::Rect GetPoseRegion(const HumanPose& pose);
vector<int> SortPosesByNeckToNose(const vector<HumanPose>& poses);

vector<HumanPose> SortPosesByHeight(const vector<HumanPose>& poses);

/*vector<array<float,1536>> ConvertPoseRegionsToReIDFeatures(
    const vector<PoseRegion>& pairs, 
    PSE& id_feature_generator,
    unique_ptr<tensorflow::Session>& tf_session);
*/
