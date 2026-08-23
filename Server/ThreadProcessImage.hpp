#ifndef ThreadProcessImage_hpp
#define ThreadProcessImage_hpp

#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <iostream>
#include <filesystem>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include "ThreadSafeQueue.hpp"
#include "SendMessageManager.hpp"
#include "ActionOption.hpp"
#include <opencv2/opencv.hpp>

#include "Setting.hpp"

struct DataFrame;

// EmotiEffLib
#include "emotiefflib/facial_analysis.h"

#ifdef USE_GPU
#include "libmp_gpu.h"
#else
#include "libmp.h"
#endif

#ifdef USE_dlib
#include <dlib/dnn.h>
#include <dlib/gui_widgets.h>
// #include <dlib/clustering.h>
#include <dlib/string.h>
#include <dlib/image_io.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing.h>
#include <dlib/opencv.h> // Essential for cv_image
#endif

// Yolo11pse
#include "Yolo11Pose.hpp"

// InspireFace
#include "inspireface.h"
#include "herror.h" // 為了使用 HSUCCEED

// using namespace dlib;
#ifdef USE_dlib
using dlib::add_prev1;
using dlib::add_prev2;
using dlib::affine;
using dlib::avg_pool;
using dlib::avg_pool_everything;
using dlib::con;
using dlib::deserialize;
using dlib::fc_no_bias;
using dlib::full_object_detection;
using dlib::input_rgb_image_sized;
using dlib::loss_metric;
using dlib::loss_multiclass_log_per_pixel;
using dlib::matrix;
using dlib::max_pool;
using dlib::rectangle;
using dlib::relu;
using dlib::shape_predictor;
using dlib::skip1;
using dlib::tag1;
using dlib::tag2;
// ----------------------------------------------------------------------------------------

// The next bit of code defines a ResNet network.  It's basically copied
// and pasted from the dnn_imagenet_ex.cpp example, except we replaced the loss
// layer with loss_metric and made the network somewhat smaller.  Go read the introductory
// dlib DNN examples to learn what all this stuff means.
//
// Also, the dnn_metric_learning_on_images_ex.cpp example shows how to train this network.
// The dlib_face_recognition_resnet_model_v1 model used by this example was trained using
// essentially the code shown in dnn_metric_learning_on_images_ex.cpp except the
// mini-batches were made larger (35x15 instead of 5x5), the iterations without progress
// was set to 10000, and the training dataset consisted of about 3 million images instead of
// 55.  Also, the input layer was locked to images of size 150.
template <template <int, template <typename> class, int, typename> class block, int N, template <typename> class BN, typename SUBNET>
using residual = add_prev1<block<N, BN, 1, tag1<SUBNET>>>;

template <template <int, template <typename> class, int, typename> class block, int N, template <typename> class BN, typename SUBNET>
using residual_down = add_prev2<avg_pool<2, 2, 2, 2, skip1<tag2<block<N, BN, 2, tag1<SUBNET>>>>>>;

template <int N, template <typename> class BN, int stride, typename SUBNET>
using block = BN<con<N, 3, 3, 1, 1, relu<BN<con<N, 3, 3, stride, stride, SUBNET>>>>>;

template <int N, typename SUBNET>
using ares = relu<residual<block, N, affine, SUBNET>>;
template <int N, typename SUBNET>
using ares_down = relu<residual_down<block, N, affine, SUBNET>>;

template <typename SUBNET>
using alevel0 = ares_down<256, SUBNET>;
template <typename SUBNET>
using alevel1 = ares<256, ares<256, ares_down<256, SUBNET>>>;
template <typename SUBNET>
using alevel2 = ares<128, ares<128, ares_down<128, SUBNET>>>;
template <typename SUBNET>
using alevel3 = ares<64, ares<64, ares<64, ares_down<64, SUBNET>>>>;
template <typename SUBNET>
using alevel4 = ares<32, ares<32, ares<32, SUBNET>>>;

using anet_type = loss_metric<fc_no_bias<128, avg_pool_everything<
                                                  alevel0<
                                                      alevel1<
                                                          alevel2<
                                                              alevel3<
                                                                  alevel4<
                                                                      max_pool<3, 3, 2, 2, relu<affine<con<32, 7, 7, 2, 2, input_rgb_image_sized<150>>>>>>>>>>>>>;

// ----------------------------------------------------------------------------------------
#endif

// using namespace std;
using std::array;
using std::vector;
using namespace cv;
extern int is_dancing;

class ThreadProcessImage : public QThread
{
    Q_OBJECT

public:
    ThreadProcessImage();
    ~ThreadProcessImage();

    bool b_HumanPoseEstimation = false;
    bool b_WhileLoop = true;
    bool bSaveTransmittedImage = false;
    std::atomic<bool> bSaveRequestedPhoto{false};
    std::string requestedPhotoPrefix;
    bool m_bRecognizeFacialExpression = true;
    string ImageSaveDirectory = ""; // default value is empty, which means not saving images

    SendMessageManager *pSendMessageManager;
    ThreadSafeQueue<DataFrame> DataFrames_queue;

    bool bNewoutFrame = false;
    void NotifyEvent(string description, chrono::time_point<chrono::system_clock> timestamp, float yaw = 0.0, float pitch = 0.0);

    ActionOption action_option;
    Mat getOutFrame();
    Mat getLatestFrame();

    string GetPatientGender();
    int GetPatientAge();
    void SetSettingFile(const QString &filePath);

protected:
    void run();
    std::shared_ptr<mediapipe::LibMP> libmp_face;
    std::shared_ptr<mediapipe::LibMP> libmp_hand;
    std::shared_ptr<mediapipe::LibMP> libmp_pose;

    std::mutex mtx_Task; // dlib::mutex has the same class mutex, so I add std::
    std::mutex mtx_UpdateOutFrame;
    bool m_bDirectoryCreated = false;
    bool m_bDirectoryCreated_VisualCompass = false;
    bool mbWatchPatient = true;
    Mat outFrame, tempFrame;
    cv::Rect GetBoundingBoxFromLandmarks(const std::vector<std::array<float, 3>> &normalized_landmarks, int img_width, int img_height);
    Mat CropRegion(Mat inputImage, std::vector<std::array<float, 3>> normalized_landmarks); // dlib has the vector and array, too.       //Crop the face region from inputImage according to the landmarks
    unique_ptr<EmotiEffLib::EmotiEffLibRecognizer> fer;

#ifdef USE_dlib
    // I need to create a dlib::face_recognition_model_v1 object to extract face recognition features
    anet_type net;
#endif

    Yolo11Pose yolo11pose;
    std::string mstr_captured_timestamp;

    Setting msetting;

    // InspireFace
    HFSession session = {0};

    string msPatientGender = "Unknown"; // default
    int miPatientAge = -1;              // default -1 means unknown age

    std::chrono::time_point<std::chrono::system_clock> m_LastPersonDetectedTime;
    std::chrono::time_point<std::chrono::system_clock> m_LastCompassCheckTime;
    bool m_bTurningToZero = false;
    bool m_bBodyAtZero = false;
    string s_RobotModel = "Unknown"; // default value is Unknown. It will be set to "Kebbi" or "Zenbo" when the robot sends the first status message.
};

#endif