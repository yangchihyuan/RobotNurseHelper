#include "MediaPipeDetector.h"

#include <iostream>

using namespace mediapipe;
using namespace mediapipe::tasks::vision::pose_landmarker;
using namespace mediapipe::tasks::vision::object_detector;
using namespace mediapipe::tasks::core;

MediaPipeDetector::MediaPipeDetector() {}

MediaPipeDetector::~MediaPipeDetector() {}

std::unique_ptr<Image> MediaPipeDetector::ConvertMatToMediaPipeImage(const cv::Mat& frame) {
    cv::Mat rgb;
    if (frame.channels() == 3) {
        cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    } else {
        // Fallback: assume already RGB
        rgb = frame;
    }

    auto image_frame = std::make_unique<ImageFrame>(
        ImageFormat::SRGB,
        rgb.cols,
        rgb.rows,
        ImageFrame::kDefaultAlignmentBoundary);

    if (rgb.isContinuous()) {
        std::memcpy(image_frame->MutablePixelData(), rgb.data,
                    static_cast<size_t>(rgb.total() * rgb.elemSize()));
    } else {
        cv::Mat tmp = rgb.clone();
        std::memcpy(image_frame->MutablePixelData(), tmp.data,
                    static_cast<size_t>(tmp.total() * tmp.elemSize()));
    }

    return std::make_unique<Image>(std::move(image_frame));
}

bool MediaPipeDetector::InitializePose(const std::string& taskPath) {
    auto options = std::make_unique<PoseLandmarkerOptions>();
    options->base_options.model_asset_path = taskPath;

    // Force MediaPipe Tasks to use CPU delegate (prevents GpuBuffer/ImageFrame mismatch)
    options->base_options.delegate = Delegate::CPU;

    options->running_mode = RunningMode::IMAGE;


    auto status_or_landmarker = PoseLandmarker::Create(std::move(options));
    if (!status_or_landmarker.ok()) {
        std::cerr << "Failed to create PoseLandmarker: "
                  << status_or_landmarker.status().message() << std::endl;
        return false;
    }
    pose_landmarker_ = std::move(status_or_landmarker.value());
    return true;
}

bool MediaPipeDetector::InitializeObjectDetector(const std::string& taskPath) {
    auto options = std::make_unique<ObjectDetectorOptions>();
    options->base_options.model_asset_path = taskPath;

    // Force MediaPipe Tasks to use CPU delegate (prevents GpuBuffer/ImageFrame mismatch)
    options->base_options.delegate = Delegate::CPU;

    options->running_mode = RunningMode::IMAGE;
    options->score_threshold = 0.5f;


    auto status_or_detector = ObjectDetector::Create(std::move(options));
    if (!status_or_detector.ok()) {
        std::cerr << "Failed to create ObjectDetector: "
                  << status_or_detector.status().message() << std::endl;
        return false;
    }
    object_detector_ = std::move(status_or_detector.value());
    return true;
}

void MediaPipeDetector::DetectPose(const cv::Mat& frame, cv::Mat& outFrame) {
    if (!pose_landmarker_) return;
    outFrame = frame.clone();

    auto mp_image = ConvertMatToMediaPipeImage(frame);
    auto result_or = pose_landmarker_->Detect(*mp_image);

    if (!result_or.ok()) return;

    const auto& pose_result = result_or.value();
    for (const auto& pose_landmarks : pose_result.pose_landmarks) {
        for (const auto& landmark : pose_landmarks) {
            int x = static_cast<int>(landmark.x() * frame.cols);
            int y = static_cast<int>(landmark.y() * frame.rows);
            if (x >= 0 && x < frame.cols && y >= 0 && y < frame.rows) {
                cv::circle(outFrame, cv::Point(x, y), 5, cv::Scalar(0, 255, 0), -1);
            }
        }
    }
}

void MediaPipeDetector::DetectObjects(const cv::Mat& frame, cv::Mat& outFrame) {
    if (!object_detector_) return;
    outFrame = frame.clone();

    auto mp_image = ConvertMatToMediaPipeImage(frame);
    auto result_or = object_detector_->Detect(*mp_image);

    if (!result_or.ok()) return;

    const auto& detection_result = result_or.value();
    for (const auto& detection : detection_result.detections) {
        if (!detection.bounding_box() ) {
            continue;
        }
        const auto& bbox = detection.bounding_box();

        // MediaPipe tasks bbox uses normalized coordinates.
        int x = static_cast<int>(bbox.origin_x() * frame.cols);
        int y = static_cast<int>(bbox.origin_y() * frame.rows);
        int w = static_cast<int>(bbox.width() * frame.cols);
        int h = static_cast<int>(bbox.height() * frame.rows);

        cv::Rect rect(x, y, w, h);
        rect &= cv::Rect(0, 0, frame.cols, frame.rows);
        if (rect.width <= 0 || rect.height <= 0) continue;

        cv::rectangle(outFrame, rect, cv::Scalar(255, 0, 0), 2);

        std::string label;
        if (!detection.categories().empty()) {
            label = detection.categories()[0].category_name();
        }
        if (label.empty()) {
            label = "object";
        }

        cv::putText(outFrame, label, cv::Point(rect.x, std::max(0, rect.y - 10)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 0, 0), 2);
    }
}

