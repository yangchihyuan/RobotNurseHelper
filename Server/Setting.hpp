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
    bool bSaveImages = false;
    int iImageSaveIntervalMillisecond = 1000;   //default value is 1000, which means the interval between two save images is 1000 milliseconds. This is used to control how many images will be saved when bSaveTransmittedImage is true. The smaller this value, the more images will be saved.
    bool bFacialExpressionRecognition = false;
    bool bHumanPoseEstimation = false;
    string PoseEstimationModel = "Yolo11n_Pose";  //Yolo11n_Pose, MediaPipe_Pose
    bool bHandLandmarkDetection = false;
    bool bUseDlibForFaceRecognition = false;
    bool bFaceDetection = false;
    string FaceDetectionModel = "InspireFace";    //MediaPipe_Face, InspireFace
    bool bUseVisualCompass = false;
    string WhisperModel = "$HOME/RobotNurseHelper_build/whisper.cpp/models/ggml-large-v3-turbo.bin";
    string ImageSaveDirectory = "$HOME/Downloads/raw_images";
    string LanguageModel = "gemma3:1b";
    string Language = "Chinese";
    string AnythingLLM_API_key;
    string AnythingLLM_workspace_slug;
    int AnythingLLM_Port = 3001;
    bool bHideCursor = true;
    string Machine = "PC";                      //AGXOrin, PC
    int iTextFontSize = 48;
    
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Setting, StateControlFile, bServerPlaysRobotReceivedAudio, bVideoWindowFullScreen, bShowPreviewWindow, bSaveImages,iImageSaveIntervalMillisecond, bFacialExpressionRecognition, bHumanPoseEstimation, bHandLandmarkDetection, bUseDlibForFaceRecognition, bFaceDetection, PoseEstimationModel, FaceDetectionModel, bUseVisualCompass, WhisperModel, ImageSaveDirectory, LanguageModel, Language, AnythingLLM_API_key, AnythingLLM_workspace_slug, AnythingLLM_Port, bHideCursor, Machine
)

#endif // SETTING_HPP