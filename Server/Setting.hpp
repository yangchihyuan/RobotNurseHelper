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
    bool bHideCursor = true;
    string Machine = "PC";                      //AGXOrin, PC
    int iTextFontSize = 48;

    //Proactive vision wake-up: when true, ThreadProcessImage runs a YOLO person
    //detector while the robot is idle (state 0) and wakes it up on sight of a
    //person instead of waiting for a voice command. Requires a YOLO detection
    //(not pose) ONNX model at PersonDetectionModel; if that file is missing the
    //feature disables itself at startup with a warning rather than failing.
    bool bProactiveGreeting = true;
    string PersonDetectionModel = "yolo11n.onnx";

};

//WITH_DEFAULT: any field missing from an existing settings JSON file (e.g. one
//that predates a newly added field) falls back to the in-class default above
//instead of throwing json::out_of_range and aborting LoadJSONFile for every
//other field too.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Setting, StateControlFile, bServerPlaysRobotReceivedAudio, bVideoWindowFullScreen, bShowPreviewWindow, bSaveImages,iImageSaveIntervalMillisecond, bFacialExpressionRecognition, bHumanPoseEstimation, bHandLandmarkDetection, bUseDlibForFaceRecognition, bFaceDetection, PoseEstimationModel, FaceDetectionModel, bUseVisualCompass, WhisperModel, ImageSaveDirectory, LanguageModel, Language, AnythingLLM_API_key, AnythingLLM_workspace_slug, bHideCursor, Machine, bProactiveGreeting, PersonDetectionModel
)

#endif // SETTING_HPP