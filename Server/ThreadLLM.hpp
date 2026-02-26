#ifndef __THREAD_OLLAMA_hpp__
#define __THREAD_OLLAMA_hpp__

#include <QThread>
#include <iostream>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "ollama.hpp"
#include <chrono>
#include "ThreadStateControl.hpp"
#include "Setting.hpp"
#include "anythingllm.hpp"

using namespace std;

class ThreadStateControl;       //Because ThreadLLM.hpp and ThreadStateControl.hpp include each other, I need to use forward declaration

struct OllamaTask
{
    ollama::messages message_history;
    chrono::time_point<std::chrono::system_clock> timestamp;
    bool bNotify = true;        //Notify the ThreadStateControl
};

void DumpOllamaMessages(ollama::messages messages);

class ThreadLLM: public QThread
{
    Q_OBJECT

public:
    ThreadLLM();
    ~ThreadLLM();

    bool b_WhileLoop = true;
    bool b_new_LLM_response = false;
    
    condition_variable cond_var_ollama;
    string strPrompt;                       //The strPrompt is the user's input sentence, genrated by Whisper.
    string strResponse;
    string str_system_message;               
    
    string ModelName = "gemma3:12b";

    string generateResponse(ollama::messages message_history);
    void AddQueue(OllamaTask task);
    ThreadStateControl *mpThreadStateControl;

protected:
    void run();
    queue<OllamaTask> mqueue;
    Setting msetting;
};

#endif
