#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>

#ifndef ThreadProcessAudio_hpp
#define ThreadProcessAudio_hpp

using namespace std;

class ThreadProcessAudio: public QThread
{
    Q_OBJECT

public:
    ThreadProcessAudio();

    bool b_RunProcessAudio;
    mutex mutex_audio_buffer;
    queue<short> AudioBuffer;
    condition_variable cond_var_process_audio;

protected:
    void run();

};

#endif