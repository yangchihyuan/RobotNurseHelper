#ifndef ThreadWhipser_hpp
#define ThreadWhipser_hpp

#include <QThread>
#include <QBuffer>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <whisper.h>


using namespace std;

class ThreadWhisper: public QThread
{
    Q_OBJECT

public:
    ThreadWhisper();

    bool b_WhileLoop = true;
    condition_variable cond_var_whisper;
    QBuffer buffer;
    string result;
    bool b_new_result = false;
    QString model_file_path;

protected:
    void run();
    whisper_context* ctx = nullptr;


private:
    mutex mtx;
};

#endif