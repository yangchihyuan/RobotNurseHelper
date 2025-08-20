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
#include "SocketHandler.hpp"
#include "ThreadStateControl.hpp"

#ifdef USE_KEBBI
    #include "Kebbi/RobotCommand.pb.h"
#elif USE_ZENBO
    #include "Zenbo/RobotCommand.pb.h"
#endif

using namespace std;

class ThreadReceiveMessages: public QThread
{
    Q_OBJECT

public:
    bool b_WhileLoop = true;
    condition_variable cond_var_receive_messages;

    SendMessageManager *pSendMessageManager;
    SocketHandler *pSocketHandler;
    ThreadStateControl *mpThreadStateControl;

protected:
    void run();

private:
    mutex mtx;
};

#endif