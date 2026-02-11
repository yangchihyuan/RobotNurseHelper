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

using namespace std;

class ThreadStateControl;       //Because ThreadOllama.hpp and ThreadStateControl.hpp include each other, I need to use forward declaration

struct OllamaTask
{
    ollama::messages message_history;
    chrono::time_point<std::chrono::system_clock> timestamp;
    bool bNotify = true;        //Notify the ThreadStateControl
};

void DumpOllamaMessages(ollama::messages messages);

class ThreadOllama: public QThread
{
    Q_OBJECT

public:
    ThreadOllama();
    ~ThreadOllama();

    //no longer use it. It has been moved to the ThreadStateControl class.
//    int stage_index = 0;
//    int mNumberOfStages = 9;
//    chrono::time_point<std::chrono::high_resolution_clock> stage_start_time[9];


    bool b_WhileLoop = true;
    bool b_new_LLM_response = false;
    
    condition_variable cond_var_ollama;
    string strPrompt;                       //The strPrompt is the user's input sentence, genrated by Whisper.
    string strResponse;
    string str_system_message;               
    chrono::seconds mStageDurationLimit[9];
    
    string bio_summary_prompt;              
    string check_stage_prompt;              //added by Mohamed
    string no_response, dance_complete;     //added by Mohamed
    string ModelName = "gemma3:12b";

    string generateResponse(ollama::messages message_history);
    void AddQueue(OllamaTask task);
    ThreadStateControl *mpThreadStateControl;

protected:
    void run();
    queue<OllamaTask> mqueue;
};

#endif
