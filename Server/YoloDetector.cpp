#include "YoloDetector.hpp"
#include <fstream>
#include <iostream>

YoloDetector::YoloDetector() {
    // Default session options – enable GPU if available
    sessionOptions.SetIntraOpNumThreads(1);
    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    // Attempt to use CUDA if present
    #ifdef USE_CUDA
    OrtCUDAProviderOptions cuda_options{};
    sessionOptions.AppendExecutionProvider_CUDA(cuda_options);
    #endif
}

YoloDetector::~YoloDetector() {
    if (session) {
        delete session;
        session = nullptr;
    }
}

bool YoloDetector::initialize(const std::string& modelPath) {
    try {
        session = new Ort::Session(env, modelPath.c_str(), sessionOptions);
    } catch (const Ort::Exception& e) {
        std::cerr << "Failed to load YOLO model: " << e.what() << std::endl;
        return false;
    }
    // Load class names – for simplicity we embed a small list here.
    classNames = {"person", "wheelchair", "IV stand", "medicine bottle", "bed", "chair", "walking stick", "medical equipment"};
    // Retrieve input dimensions from the model (assume first input)
    Ort::AllocatorWithDefaultOptions allocator;
    auto inputInfo = session->GetInputTypeInfo(0);
    auto tensorInfo = inputInfo.GetTensorTypeAndShapeInfo();
    auto inputDims = tensorInfo.GetShape();
    if (inputDims.size() == 4) { // NCHW
        inputHeight = static_cast<int>(inputDims[2]);
        inputWidth  = static_cast<int>(inputDims[3]);
    }
    return true;
}

cv::Mat YoloDetector::preprocess(const cv::Mat& img) {
    cv::Mat resized;
    cv::resize(img, resized, cv::Size(inputWidth, inputHeight));
    // Convert BGR to RGB as most YOLO models expect RGB
    cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
    // Convert to float and normalize to [0,1]
    resized.convertTo(resized, CV_32F, 1.0 / 255.0);
    return resized;
}

void YoloDetector::postprocess(const std::vector<float>& output, const cv::Size& originalSize,
                               std::vector<YoloBox>& detections) {
    // YOLO output format varies; here we assume the common format:
    // [batch, num_boxes, 85] where 85 = 4 box + 1 obj_conf + 80 class scores.
    // For simplicity we treat the output as a flat vector and iterate.
    const int numAttributes = 85; // 4 + 1 + 80
    const int numBoxes = static_cast<int>(output.size() / numAttributes);
    const float xScale = static_cast<float>(originalSize.width) / inputWidth;
    const float yScale = static_cast<float>(originalSize.height) / inputHeight;

    // Temporary containers for NMS
    std::vector<cv::Rect> boxes;
    std::vector<float> scores;
    std::vector<int> classIds;

    // Adjustable thresholds – lower confidence improves recall, NMS removes duplicates
    const float confidenceThreshold = 0.2f; // more permissive than previous 0.3
    const float nmsIoUThreshold = 0.4f;

    for (int i = 0; i < numBoxes; ++i) {
        const float* ptr = &output[i * numAttributes];
        float objConf = ptr[4];
        // Find class with max score
        int classId = -1;
        float maxClassConf = 0.0f;
        for (int c = 0; c < 80; ++c) {
            float classConf = ptr[5 + c];
            if (classConf > maxClassConf) {
                maxClassConf = classConf;
                classId = c;
            }
        }
        float confidence = objConf * maxClassConf;
        if (confidence < confidenceThreshold) continue; // filter low confidence

        // Box coordinates are center x, center y, width, height (relative to input size)
        float cx = ptr[0];
        float cy = ptr[1];
        float w  = ptr[2];
        float h  = ptr[3];
        float x1 = (cx - w / 2.0f) * xScale;
        float y1 = (cy - h / 2.0f) * yScale;
        float boxW = w * xScale;
        float boxH = h * yScale;
        cv::Rect box(static_cast<int>(x1), static_cast<int>(y1),
                     static_cast<int>(boxW), static_cast<int>(boxH));
        boxes.push_back(box);
        scores.push_back(confidence);
        classIds.push_back(classId);
    }

    // Apply class‑aware NMS using OpenCV's built‑in function
    std::vector<int> nmsIndices;
    cv::dnn::NMSBoxes(boxes, scores, confidenceThreshold, nmsIoUThreshold, nmsIndices);
    for (int idx : nmsIndices) {
        YoloBox yb;
        int cid = classIds[idx];
        yb.label = (cid >= 0 && cid < static_cast<int>(classNames.size())) ? classNames[cid] : "unknown";
        yb.confidence = scores[idx];
        yb.box = boxes[idx];
        detections.push_back(yb);
    }
}

bool YoloDetector::detect(const cv::Mat& image, std::vector<YoloBox>& detections) {
    if (!session) {
        std::cerr << "YOLO session not initialized" << std::endl;
        return false;
    }
    cv::Mat inputBlob = preprocess(image);
    // Prepare input tensor
    std::array<int64_t, 4> inputShape{1, 3, inputHeight, inputWidth};
    size_t inputTensorSize = 1 * 3 * inputHeight * inputWidth;
    std::vector<float> inputTensorValues(inputTensorSize);
    // OpenCV stores HWC; we need CHW
    std::vector<cv::Mat> channels(3);
    cv::split(inputBlob, channels);
    for (int c = 0; c < 3; ++c) {
        std::memcpy(inputTensorValues.data() + c * inputHeight * inputWidth,
                    channels[c].data, inputHeight * inputWidth * sizeof(float));
    }
    Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memInfo, inputTensorValues.data(),
                                                             inputTensorSize, inputShape.data(), 4);
    // Run inference
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::AllocatedStringPtr inputNameAlloc = session->GetInputNameAllocated(0, allocator);
    Ort::AllocatedStringPtr outputNameAlloc = session->GetOutputNameAllocated(0, allocator);
    std::vector<const char*> inputNames = {inputNameAlloc.get()};
    std::vector<const char*> outputNames = {outputNameAlloc.get()};
    auto outputTensors = session->Run(Ort::RunOptions{nullptr}, inputNames.data(), &inputTensor, 1,
                                      outputNames.data(), 1);
    // Extract output tensor data
    float* floatArray = outputTensors.front().GetTensorMutableData<float>();
    size_t outputSize = outputTensors.front().GetTensorTypeAndShapeInfo().GetElementCount();
    std::vector<float> outputVec(floatArray, floatArray + outputSize);
    postprocess(outputVec, image.size(), detections);
    return true;
}
