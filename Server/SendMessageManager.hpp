#ifndef __SendMessageManager_hpp__
#define __SendMessageManager_hpp__

#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#ifdef USE_KEBBI
    #include "Kebbi/RobotCommand.pb.h"
#elif USE_ZENBO
    #include "Zenbo/RobotCommand.pb.h"
#endif

using namespace std;

class SendMessageManager
{
public:
    QTcpSocket *pSocket = NULL;
    void AddMessage(RobotCommandProtobuf::RobotCommand);
    void Send();

protected:
    char str_results[4096];
    int str_results_len;
    mutex mutex_message_buffer;
    queue<RobotCommandProtobuf::RobotCommand> actionQueue;
    queue<RobotCommandProtobuf::RobotCommand> messageQueue;
    queue<RobotCommandProtobuf::RobotCommand> mQueue;
};

#endif