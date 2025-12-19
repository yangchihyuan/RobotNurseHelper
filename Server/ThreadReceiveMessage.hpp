#ifndef __THREAD_RECEIVE_MESSAGES_hpp__
#define __THREAD_RECEIVE_MESSAGES_hpp__

#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "SendMessageManager.hpp"
#include "SocketBufferParser.hpp"
#include "ThreadStateControl.hpp"
#include "ThreadProcessImage.hpp"
#include "ThreadSafeQueue.hpp"

#ifdef USE_KEBBI
    #include "Kebbi/RobotCommand.pb.h"
#elif USE_ZENBO
    #include "Zenbo/RobotCommand.pb.h"
#endif

using namespace std;

class ThreadReceiveMessage: public QThread
{
    Q_OBJECT

public:
    bool b_WhileLoop = true;
    condition_variable cond_var_receive_message;

    SendMessageManager *pSendMessageManager;
    //Because there are two clients connecting to this port: Tablet and Robot app, te DataFrames_queue should be inside the ThreadReceiveMessage class.
    ThreadSafeQueue<DataFrame> DataFrames_queue;
    ThreadStateControl *mpThreadStateControl;
    ThreadProcessImage *mpThreadProcessImage;
protected:
    void run();

private:
    mutex mtx;
};

#endif