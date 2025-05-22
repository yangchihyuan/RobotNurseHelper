#include <QThread>
#include <iostream>
#include <mutex>
#include <queue>

#ifndef ThreadProcessAudio_hpp
#define ThreadProcessAudio_hpp

using namespace std;

class ThreadProcessAudio: public QThread
{
    Q_OBJECT

public:
    ThreadProcessAudio();

    queue<short> AudioBuffer;

protected:
    void run();

};

#endif