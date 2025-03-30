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

#ifndef ThreadProcessImage_hpp
#define ThreadProcessImage_hpp

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

protected:
    void run();

private:
    mutex mtx;
};

#endif