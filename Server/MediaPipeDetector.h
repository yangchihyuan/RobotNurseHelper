#pragma once

#include <opencv2/opencv.hpp>
#include <memory>
#include <string>

// MediaPipe Tasks Headers
#include "mediapipe/tasks/cc/vision/pose_landmarker/pose_landmarker.h"
#include "mediapipe/tasks/cc/vision/object_detector/object_detector.h"

class MediaPipeDetector {
public:
    MediaPipeDetector();
    ~MediaPipeDetector();

    bool InitializePose(const std::string& taskPath);
    bool InitializeObjectDetector(const std::string& taskPath);

    // Inference methods (draw overlays onto outFrame)
    void DetectPose(const cv::Mat& frame, cv::Mat& outFrame);
    void DetectObjects(const cv::Mat& frame, cv::Mat& outFrame);

private:
    std::unique_ptr<mediapipe::tasks::vision::pose_landmarker::PoseLandmarker> pose_landmarker_;
    std::unique_ptr<mediapipe::tasks::vision::object_detector::ObjectDetector> object_detector_;

    // Helper to convert cv::Mat to MediaPipe Image format (SRGB)
    std::unique_ptr<mediapipe::Image> ConvertMatToMediaPipeImage(const cv::Mat& frame);
};

