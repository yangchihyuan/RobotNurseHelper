#ifndef Yolo11Pose_hpp
#define Yolo11Pose_hpp

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

class Yolo11Pose
{
public:
    Yolo11Pose();
    std::vector<std::vector<std::array<float, 3>>> Process(cv::Mat& frame);

    // COCO 17 skeleton pairs
    std::vector<std::pair<int, int>> skeleton = {
    {0,1},{0,2},
    {1,3},{2,4},
    {0,5},{0,6},
    {5,7},{7,9},
    {6,8},{8,10},
    {5,11},{6,12},
    {11,13},{13,15},
    {12,14},{14,16}
    };

    cv::Scalar COLOR_HEAD, COLOR_ARMS, COLOR_BODY, COLOR_LEGS;
    cv::Scalar pair_color(int a, int b)
    {
        if (a <= 4 || b <= 4) return COLOR_HEAD;
        if ((a >= 5 && a <= 10) || (b >= 5 && b <= 10)) return COLOR_ARMS;
        if ((a == 5 || a == 6 || a == 11 || a == 12) || (b == 5 || b == 6 || b == 12)) return COLOR_BODY;
        return COLOR_LEGS;
    };

protected:
    const std::string MODEL_PATH = "yolo11n-pose.onnx";
    const int INPUT_SIZE = 640;
    const float CONF_THRES = 0.4f;
    const float NMS_IOU = 0.45f;
    const float KP_CONF_THRES = 0.4f;



    Ort::Env env;
    Ort::SessionOptions session_options;
    Ort::Session session;
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::AllocatedStringPtr input_name_ptr;
    char* input_name;
    size_t out_count;
    std::vector<const char*> output_names;
    std::vector<Ort::AllocatedStringPtr> out_name_ptrs;

};

#endif /* Yolo11Pose_hpp */
