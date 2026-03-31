#ifndef __THREAD_OLLAMA_hpp__
#define __THREAD_OLLAMA_hpp__

#include <QThread>
#include <iostream>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <chrono>
#include "ThreadStateControl.hpp"
#include "Setting.hpp"
#include "AnythingLLM.hpp"

using namespace std;

class ThreadStateControl;       //Because ThreadLLM.hpp and ThreadStateControl.hpp include each other, I need to use forward declaration

struct LLMTask
{
    string str_message;
    chrono::time_point<std::chrono::system_clock> timestamp;
    bool bNotify = true;        //Notify the ThreadStateControl
};

class ThreadLLM: public QThread
{
    Q_OBJECT

public:
    ThreadLLM();
    ~ThreadLLM();

    bool b_WhileLoop = true;
    bool b_new_LLM_response = false;
    
    condition_variable cond_var_thread_LLM;
    string strResponse;
    

    void AddQueue(LLMTask task);
    ThreadStateControl *mpThreadStateControl;
    void SetSettingFile(const QString &filePath);

protected:
    void run();
    queue<LLMTask> mqueue;
    Setting msetting;
};

#endif
