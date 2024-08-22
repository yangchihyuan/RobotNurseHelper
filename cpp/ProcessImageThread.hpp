#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "SendCommandThread.hpp"

#ifndef ProcessImageThread_hpp
#define ProcessImageThread_hpp

using namespace std;

class ProcessImageThread: public QThread
{
    Q_OBJECT

public:
    ProcessImageThread();

    bool b_HumanPoseEstimation = false;
    bool b_WhileLoop = true;
    mutex mutex_frame_buffer1;
    unique_ptr<char[]> frame_buffer1;    //frame_buffer1 is used for human pose estimation
    int get_buffer_size();
    bool b_frame_buffer1_unused = false;    //Indicate whether frame_buffer1 is unused.
    int frame_buffer1_length = 0;
    condition_variable cond_var_process_image;

    SendCommandThread *pSendCommandThread;

protected:
    void run();

private:
    int buffer_size = 500000;


};

#endif