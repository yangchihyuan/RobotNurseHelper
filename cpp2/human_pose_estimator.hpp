#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

/*
#include <openvino/openvino.hpp>

#include <models/hpe_model_associative_embedding.h>
#include <models/hpe_model_openpose.h>
#include <models/input_data.h>
#include <models/model_base.h>
#include <models/results.h>
#include <monitors/presenter.h>
#include <pipelines/async_pipeline.h>
#include <pipelines/metadata.h>
#include <utils/common.hpp>
#include <utils/config_factory.h>
#include <utils/default_flags.hpp>
#include <utils/image_utils.h>
#include <utils/images_capture.h>
#include <utils/ocv_common.hpp>
#include <utils/performance_metrics.hpp>
#include <utils/slog.hpp>
*/

namespace human_pose_estimation {

class HumanPoseEstimator {
public:
    HumanPoseEstimator(std::string model_path);
    //2025/3/8 The HumanPose is defined in Open_Model_Zoo
    std::vector<HumanPose> estimate(const cv::Mat& image);
    ~HumanPoseEstimator();
private:
    std::unique_ptr<AsyncPipeline> pipeline;
    std::unique_ptr<ResultBase> result;
};

}