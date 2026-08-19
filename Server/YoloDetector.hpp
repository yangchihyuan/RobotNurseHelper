#pragma once

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

// Simple struct to hold a detection result from YOLO
struct YoloBox {
    std::string label;   // class name
    float confidence;    // confidence score [0,1]
    cv::Rect box;        // bounding box in pixel coordinates
};

/**
 * YoloDetector wraps an ONNX Runtime session for a YOLOv11/v12 model.
 * It loads the model once and provides a thread‑safe detect() method.
 * The class is deliberately lightweight so it can be instantiated inside
 * ThreadProcessImage without affecting existing functionality.
 */
class YoloDetector {
public:
    YoloDetector();
    ~YoloDetector();

    /**
     * Initialize the detector with the path to an ONNX model.
     * Returns true on success.
     */
    bool initialize(const std::string& modelPath);

    /**
     * Run inference on a BGR cv::Mat image and fill the output vector with detections.
     * The image is resized to the model's expected input size internally.
     */
    bool detect(const cv::Mat& image, std::vector<YoloBox>& detections);

private:
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "YoloDetector"};
    Ort::Session* session = nullptr;
    Ort::SessionOptions sessionOptions;
    std::vector<std::string> classNames; // e.g., person, wheelchair, ...
    int inputWidth = 640;   // default YOLO input size – can be overridden after init
    int inputHeight = 640;

    void loadClassNames();
    cv::Mat preprocess(const cv::Mat& img);
    void postprocess(const std::vector<float>& output, const cv::Size& originalSize,
                    std::vector<YoloBox>& detections);
};
