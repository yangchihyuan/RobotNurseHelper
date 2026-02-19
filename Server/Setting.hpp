#ifndef SETTING_HPP
#define SETTING_HPP

#include <string>
#include <nlohmann/json.hpp>
#include "utility_json.hpp"

using namespace std;

struct Setting
{
    string StateControlFile;
    bool bServerPlaysRobotReceivedAudio = false;
    bool bVideoWindowFullScreen = true;
    bool bShowPreviewWindow = false;
    int iImageSaveIntervalMillisecond = 1000;   //default value is 1000, which means the interval between two save images is 1000 milliseconds. This is used to control how many images will be saved when bSaveTransmittedImage is true. The smaller this value, the more images will be saved.
    bool bFacialExpressionRecognition = false;
    bool bHumanPoseEstimation = false;
    string PoseEstimationModel = "Yolo11n_Pose";  //Yolo11n_Pose, MediaPipe_Pose
    bool bHandLandmarkDetection = false;
    bool bUseDlibForFaceRecognition = false;
    bool bFaceDetection = false;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Setting, StateControlFile, bServerPlaysRobotReceivedAudio, bVideoWindowFullScreen, bShowPreviewWindow, iImageSaveIntervalMillisecond, bFacialExpressionRecognition, bHumanPoseEstimation, bHandLandmarkDetection, bUseDlibForFaceRecognition, bFaceDetection, PoseEstimationModel
)

#endif // SETTING_HPP