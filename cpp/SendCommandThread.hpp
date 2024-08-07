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

#ifndef SendCommandThread_hpp
#define SendCommandThread_hpp

using namespace std;

class SendCommandThread: public QThread
{
    Q_OBJECT

public:
    SendCommandThread();

    bool b_KeepLoop;
    QTcpSocket *pSocket = NULL;
    condition_variable cond_var_report_result;    

public slots:
    void AddMessage(ZenboNurseHelperProtobuf::ReportAndCommand);

protected:
    void run();
    char str_results[4096];
    int str_results_len;
    mutex mutex_message_buffer;
    queue<ZenboNurseHelperProtobuf::ReportAndCommand> mQueue;

};

#endif