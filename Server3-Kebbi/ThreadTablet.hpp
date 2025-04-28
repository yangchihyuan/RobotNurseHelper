#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "SendMessageManager.hpp"
#include "SocketHandler.hpp"

#ifndef ThreadTablet_hpp
#define ThreadTablet_hpp

using namespace std;

class ThreadTablet: public QThread
{
    Q_OBJECT

public:
    bool b_HumanPoseEstimation = false;
    bool b_WhileLoop = true;
    bool bSaveTransmittedImage = false;
    string raw_images_directory;
    condition_variable cond_var_process_image;

    SendMessageManager *pSendMessageManager;
    SocketHandler *pSocketHandler;

protected:
    void run();

private:
    mutex mtx;
};

#endif