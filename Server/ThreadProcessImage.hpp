#ifndef ThreadProcessImage_hpp
#define ThreadProcessImage_hpp

#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "SocketHandler.hpp"
#include "SendMessageManager.hpp"
#include <opencv2/opencv.hpp>
//EmotiEffLib
#include "emotiefflib/facial_analysis.h"

#ifdef USE_GPU
    #include "libmp_gpu.h"
#else
    #include "libmp.h"
#endif


using namespace std;
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

    void setTask(std::string task);
    void setProcessor(std::string processor);
    bool bNewoutFrame = false;
    int image_save_every_N_frame = 1; //default value is 1, which means every frame will be saved
    void NotifyEvent(string description, chrono::time_point<chrono::system_clock> timestamp, float yaw = 0.0, float pitch = 0.0);

    Mat getOutFrame();
protected:
    void run();
    void reloadGraph();
    std::string Task;
    std::shared_ptr<mediapipe::LibMP> libmp;
    std::string Processor;
    mutex mtx_Task;
    mutex mtx_UpdateOutFrame;
    bool m_bDirectoryCreated = false;
    bool mbWatchPatient = true;
    Mat outFrame;
    Mat CropRegion(Mat inputImage, vector<array<float, 3>> normalized_landmarks);        //Crop the face region from inputImage according to the landmarks
    unique_ptr<EmotiEffLib::EmotiEffLibRecognizer> fer;
};

#endif