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
    bool b_WhileLoop = true;
    condition_variable cond_var_tablet;

    SendMessageManager *pSendMessageManager;
    SocketHandler *pSocketHandler;

protected:
    void run();

private:
    mutex mtx;
};

#endif