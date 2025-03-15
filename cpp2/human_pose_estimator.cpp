#include "human_pose_estimator.hpp"
bool bNewoutFrame = false;
cv::Mat outFrame;
namespace human_pose_estimation {

cv::Mat renderHumanPose(HumanPoseResult& result, OutputTransform& outputTransform, cv::Mat outputImg) {
    if (!result.metaData) {
        throw std::invalid_argument("Renderer: metadata is null");
    }

    //if I use this img, I will get an old image in the pipeline.
//    auto outputImg = result.metaData->asRef<ImageMetaData>().img;

//    if (outputImg.empty()) {
//       throw std::invalid_argument("Renderer: image provided in metadata is empty");
//    }
    outputTransform.resize(outputImg);
    static const cv::Scalar colors[HPEOpenPose::keypointsNumber] = {cv::Scalar(255, 0, 0),
                                                                    cv::Scalar(255, 85, 0),
                                                                    cv::Scalar(255, 170, 0),
                                                                    cv::Scalar(255, 255, 0),
                                                                    cv::Scalar(170, 255, 0),
                                                                    cv::Scalar(85, 255, 0),
                                                                    cv::Scalar(0, 255, 0),
                                                                    cv::Scalar(0, 255, 85),
                                                                    cv::Scalar(0, 255, 170),
                                                                    cv::Scalar(0, 255, 255),
                                                                    cv::Scalar(0, 170, 255),
                                                                    cv::Scalar(0, 85, 255),
                                                                    cv::Scalar(0, 0, 255),
                                                                    cv::Scalar(85, 0, 255),
                                                                    cv::Scalar(170, 0, 255),
                                                                    cv::Scalar(255, 0, 255),
                                                                    cv::Scalar(255, 0, 170),
                                                                    cv::Scalar(255, 0, 85)};
    //This is the connection between keypoints for OpenPose format.
    static const std::pair<int, int> keypointsOP[] = {{1, 2},
                                                      {1, 5},
                                                      {2, 3},
                                                      {3, 4},
                                                      {5, 6},
                                                      {6, 7},
                                                      {1, 8},
                                                      {8, 9},
                                                      {9, 10},
                                                      {1, 11},
                                                      {11, 12},
                                                      {12, 13},
                                                      {1, 0},
                                                      {0, 14},
                                                      {14, 16},
                                                      {0, 15},
                                                      {15, 17}};
    //This is for another format. AE: Associative Embedding
    static const std::pair<int, int> keypointsAE[] = {{15, 13},
                                                      {13, 11},
                                                      {16, 14},
                                                      {14, 12},
                                                      {11, 12},
                                                      {5, 11},
                                                      {6, 12},
                                                      {5, 6},
                                                      {5, 7},
                                                      {6, 8},
                                                      {7, 9},
                                                      {8, 10},
                                                      {1, 2},
                                                      {0, 1},
                                                      {0, 2},
                                                      {1, 3},
                                                      {2, 4},
                                                      {3, 5},
                                                      {4, 6}};
    const int stickWidth = 4;
    const cv::Point2f absentKeypoint(-1.0f, -1.0f);
    for (auto& pose : result.poses) {
        for (size_t keypointIdx = 0; keypointIdx < pose.keypoints.size(); keypointIdx++) {
            if (pose.keypoints[keypointIdx] != absentKeypoint) {
                outputTransform.scaleCoord(pose.keypoints[keypointIdx]);
                cv::circle(outputImg, pose.keypoints[keypointIdx], 4, colors[keypointIdx], -1);
            }
        }
    }
    std::vector<std::pair<int, int>> limbKeypointsIds;
    if (!result.poses.empty()) {
        if (result.poses[0].keypoints.size() == HPEOpenPose::keypointsNumber) {
            limbKeypointsIds.insert(limbKeypointsIds.begin(), std::begin(keypointsOP), std::end(keypointsOP));
        } else {
            limbKeypointsIds.insert(limbKeypointsIds.begin(), std::begin(keypointsAE), std::end(keypointsAE));
        }
    }
    cv::Mat pane = outputImg.clone();
    for (auto pose : result.poses) {
        for (const auto& limbKeypointsId : limbKeypointsIds) {
            std::pair<cv::Point2f, cv::Point2f> limbKeypoints(pose.keypoints[limbKeypointsId.first],
                                                              pose.keypoints[limbKeypointsId.second]);
            if (limbKeypoints.first == absentKeypoint || limbKeypoints.second == absentKeypoint) {
                continue;
            }

            float meanX = (limbKeypoints.first.x + limbKeypoints.second.x) / 2;
            float meanY = (limbKeypoints.first.y + limbKeypoints.second.y) / 2;
            cv::Point difference = limbKeypoints.first - limbKeypoints.second;
            double length = std::sqrt(difference.x * difference.x + difference.y * difference.y);
            int angle = static_cast<int>(std::atan2(difference.y, difference.x) * 180 / CV_PI);
            std::vector<cv::Point> polygon;
            cv::ellipse2Poly(cv::Point2d(meanX, meanY), cv::Size2d(length / 2, stickWidth), angle, 0, 360, 1, polygon);
            cv::fillConvexPoly(pane, polygon, colors[limbKeypointsId.second]);
        }
    }
    cv::addWeighted(outputImg, 0.4, pane, 0.6, 0, outputImg);
    return outputImg;
}


HumanPoseEstimator::HumanPoseEstimator(std::string model_path)
{
    std::unique_ptr<ModelBase> model;
    //Chih-Yuan Yang: Zenbo's image resolution is VGA
    double aspectRatio = 640.0 / 480.0;
    uint32_t tsize = 0;
    float threshold = 0.1;
    std::string layout = "";
    model.reset(new HPEOpenPose(model_path, aspectRatio, tsize, threshold, layout));
    std::string d = "CPU";
    uint32_t nireq = 0;
    std::string nstreams = "";
    //std::string nstreams = "CPU:6"; //No difference.
    uint32_t nthreads = 0;
    //uint32_t nthreads = 12;    //No difference.
    ov::Core core;    //What is this?

    pipeline = std::unique_ptr<AsyncPipeline>( new AsyncPipeline(std::move(model),
                            ConfigFactory::getUserConfig(d, nireq, nstreams, nthreads),
                            core));
}

std::vector<HumanPose> HumanPoseEstimator::estimate(const cv::Mat& curr_frame)
{
    auto startTime = std::chrono::steady_clock::now();
    pipeline->submitData(ImageInputData(curr_frame),
                                    std::make_shared<ImageMetaData>(curr_frame, startTime));
    //--- Waiting for free input slot or output data available. Function will return immediately if any of them
    // are available.
    //What will happen if I do not use waitForDta()? no difference.
    pipeline->waitForData();
    result = pipeline->getResult();
    //Chih-Yuan Yang: Here is the code to render images with poses.
    //The renderingStart is not used.
//    auto renderingStart = std::chrono::steady_clock::now();
    //What is the outputTransform?
    OutputTransform outputTransform = OutputTransform();


    std::vector<HumanPose> poses;
    try{
        HumanPoseResult temp = result->asRef<HumanPoseResult>();  //Error: here is an exception std::bad_cast
        poses = temp.poses;
        outFrame = renderHumanPose(result->asRef<HumanPoseResult>(), outputTransform, curr_frame);
        bNewoutFrame = true;
//        cv::Mat outFrame = renderHumanPose(result->asRef<HumanPoseResult>(), outputTransform, curr_frame);

//        cv::Mat outFrame = renderHumanPose(result->asRef<HumanPoseResult>(), outputTransform);
//        outFrame = renderHumanPose(result->asRef<HumanPoseResult>(), outputTransform);
        //the string u is used to decide which monitor to display. I don't need it.
//        std::string u = "";
//        Presenter presenter(u);  
//        presenter.drawGraphs(outFrame);  
//        cv::imshow("Human Pose Estimation Results", outFrame);    //This command in unstable.
        //--- Processing keyboard events
//        int key = cv::waitKey(25);
    }
    catch(std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
    return poses;
}

HumanPoseEstimator::~HumanPoseEstimator() {
}

}  // namespace human_pose_estimation
