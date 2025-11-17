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
#include "SocketHandler.hpp"
#include "SendMessageManager.hpp"
#include "ActionOption.hpp"
#include <opencv2/opencv.hpp>


//EmotiEffLib
#include "emotiefflib/facial_analysis.h"

#ifdef USE_GPU
    #include "libmp_gpu.h"
#else
    #include "libmp.h"
#endif
#include <dlib/dnn.h>
#include <dlib/gui_widgets.h>
//#include <dlib/clustering.h>
#include <dlib/string.h>
#include <dlib/image_io.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing.h>
#include <dlib/opencv.h> // Essential for cv_image

//Yolo11pse
#include "Yolo11Pose.hpp"   

//using namespace dlib;
using dlib::add_prev1;
using dlib::add_prev2;
using dlib::avg_pool;
using dlib::con;
using dlib::fc_no_bias;
using dlib::input_rgb_image_sized;
using dlib::loss_metric;
using dlib::max_pool;
using dlib::relu;
using dlib::tag1;
using dlib::tag2;
using dlib::affine;
using dlib::avg_pool_everything;
using dlib::loss_multiclass_log_per_pixel;
using dlib::matrix;
using dlib::deserialize;
using dlib::shape_predictor;
using dlib::rectangle;
using dlib::full_object_detection;
using dlib::skip1;
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
template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual = add_prev1<block<N,BN,1,tag1<SUBNET>>>;

template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual_down = add_prev2<avg_pool<2,2,2,2,skip1<tag2<block<N,BN,2,tag1<SUBNET>>>>>>;

template <int N, template <typename> class BN, int stride, typename SUBNET> 
using block  = BN<con<N,3,3,1,1,relu<BN<con<N,3,3,stride,stride,SUBNET>>>>>;

template <int N, typename SUBNET> using ares      = relu<residual<block,N,affine,SUBNET>>;
template <int N, typename SUBNET> using ares_down = relu<residual_down<block,N,affine,SUBNET>>;

template <typename SUBNET> using alevel0 = ares_down<256,SUBNET>;
template <typename SUBNET> using alevel1 = ares<256,ares<256,ares_down<256,SUBNET>>>;
template <typename SUBNET> using alevel2 = ares<128,ares<128,ares_down<128,SUBNET>>>;
template <typename SUBNET> using alevel3 = ares<64,ares<64,ares<64,ares_down<64,SUBNET>>>>;
template <typename SUBNET> using alevel4 = ares<32,ares<32,ares<32,SUBNET>>>;

using anet_type = loss_metric<fc_no_bias<128,avg_pool_everything<
                            alevel0<
                            alevel1<
                            alevel2<
                            alevel3<
                            alevel4<
                            max_pool<3,3,2,2,relu<affine<con<32,7,7,2,2,
                            input_rgb_image_sized<150>
                            >>>>>>>>>>>>;

// ----------------------------------------------------------------------------------------



//using namespace std;
using std::vector;
using std::array;
using namespace cv;
extern int is_dancing;

class ThreadProcessImage: public QThread
{
    Q_OBJECT

public:
    ThreadProcessImage();

    bool b_HumanPoseEstimation = false;
    bool b_WhileLoop = true;
    bool bSaveTransmittedImage = false;
    bool m_bRecognizeFacialExpression = true;
    string ImageSaveDirectory = ""; //default value is empty, which means not saving images
    condition_variable cond_var_process_image;

    SendMessageManager *pSendMessageManager;
    SocketHandler *pSocketHandler;

    bool bNewoutFrame = false;
    int image_save_every_N_frame = 1; //default value is 1, which means every frame will be saved
    void NotifyEvent(string description, chrono::time_point<chrono::system_clock> timestamp, float yaw = 0.0, float pitch = 0.0);

    ActionOption action_option;
    Mat getOutFrame();
protected:
    void run();
    std::shared_ptr<mediapipe::LibMP> libmp_face;
    std::shared_ptr<mediapipe::LibMP> libmp_hand;
    std::shared_ptr<mediapipe::LibMP> libmp_pose;

    std::mutex mtx_Task;                        //dlib::mutex has the same class mutex, so I add std::
    std::mutex mtx_UpdateOutFrame;
    bool m_bDirectoryCreated = false;
    bool mbWatchPatient = true;
    Mat outFrame, tempFrame;
    cv::Rect GetBoundingBoxFromLandmarks(const std::vector<std::array<float, 3>>& normalized_landmarks, int img_width, int img_height);    
    Mat CropRegion(Mat inputImage, std::vector<std::array<float, 3>> normalized_landmarks);   //dlib has the vector and array, too.       //Crop the face region from inputImage according to the landmarks
    unique_ptr<EmotiEffLib::EmotiEffLibRecognizer> fer;

    //I need to create a dlib::face_recognition_model_v1 object to extract face recognition features
    anet_type net;

    Yolo11Pose yolo11pose;
};

#endif