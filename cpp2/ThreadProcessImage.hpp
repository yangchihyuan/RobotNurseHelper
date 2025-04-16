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
#include "ThreadSendCommand.hpp"
#include "SocketHandler.hpp"
#ifdef USE_GPU
    #include "libmp_gpu.h"
#else
    #include "libmp.h"
#endif


using namespace std;

class ThreadProcessImage: public QThread
{
    Q_OBJECT

public:
    ThreadProcessImage();

    bool b_HumanPoseEstimation = false;
    bool b_WhileLoop = true;
    bool bSaveTransmittedImage = false;
    string raw_images_directory;
    condition_variable cond_var_process_image;

    ThreadSendCommand *pThreadSendCommand;
    SocketHandler *pSocketHandler;

    void setTask(std::string task);
    void setProcessor(std::string processor);

protected:
    void run();
    void reloadGraph();
    std::string Task;
    std::shared_ptr<mediapipe::LibMP> libmp;
    std::string Processor;
    mutex mtx_Task;
    mutex mtx;
};

#endif