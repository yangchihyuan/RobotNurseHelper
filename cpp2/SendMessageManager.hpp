#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include "ServerSend.pb.h"

#ifndef __SendMessageManager_hpp__
#define __SendMessageManager_hpp__

using namespace std;

class SendMessageManager
{
public:
    QTcpSocket *pSocket = NULL;
    void AddMessage(ZenboNurseHelperProtobuf::ReportAndCommand);
    void Send();

protected:
    char str_results[4096];
    int str_results_len;
    mutex mutex_message_buffer;
    queue<ZenboNurseHelperProtobuf::ReportAndCommand> mQueue;
};

#endif