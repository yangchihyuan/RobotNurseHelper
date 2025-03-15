#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>

#ifndef ProcessAudioThread_hpp
#define ProcessAudioThread_hpp

using namespace std;

class ProcessAudioThread: public QThread
{
    Q_OBJECT

public:
    ProcessAudioThread();

    bool b_RunProcessAudio;
    mutex mutex_audio_buffer;
    queue<short> AudioBuffer;
    condition_variable cond_var_process_audio;

protected:
    void run();

};

#endif