#ifndef Ollama_hpp
#define Ollama_hpp

#include <QThread>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include "ollama.hpp"

using namespace std;

extern string chosen_action; 
extern int chosen_dance;
extern vector<string> summary;

class ThreadOllama: public QThread
{
    Q_OBJECT

public:
    ThreadOllama();
    ~ThreadOllama();

    int maximum_prompt_wait_time;
    bool b_WhileLoop = true;
    condition_variable cond_var_ollama;
    string strPrompt;
    bool b_new_LLM_response = false;
    string strResponse;
    string str_system_message;
    string str_system_message_list[4];
    string bio_summary_prompt;
    string check_stage_prompt;
    string no_response;
    string ModelName = "gemma3:27b";
    string action_prompt = R"(Here is a list of available robot actions:
    
    "EM_Mad02", "BA_Nodhead", "SP_Swim02", "PE_RotateA", "SP_Karate", "RE_Cheer", "SP_Climb",
    "DA_Hit", "TA_DictateR", "SP_Bowling", "SP_Walk", "SA_Find", "BA_TurnHead", "SA_Toothache",
    "SA_Sick", "SA_Shocked", "SP_Dumbbell", "SA_Discover", "RE_Thanks", "PE_Changing",
    "SP_HorizontalBar", "WO_Traffic", "RE_HiR", "RE_HiL", "DA_Brushteeth", "RE_Encourage",
    "RE_Request", "PE_Brewing", "RE_Change", "PE_Phubbing", "RE_Baoquan", "SP_Cheer", "RE_Ask",
    "PE_Triangel", "PE_Sorcery", "PE_Sneak", "PE_Singing", "LE_Yoyo", "SP_Throw", "SP_RaceWalk",
    "PE_ShakeFart", "PE_RotateC", "PE_RotateB", "EM_Blush", "PE_Puff", "PE_PlayCello", "PE_Pikachu"
    
    Pick the best action for a suitable for the recent conversation context provided. For example, SA_Shocked for shocking responses, RE_Request for requests and so on. If a child patient requests a certain action, choose the most fitting from the above list. If no option seems too fitting for the context, be more creative in choosing from the list.
    
    Reply only with:
    The chosen action)";

protected:
    void run();
    string validate_conversation(ollama::options options, ollama::messages &message_history, string &prompt, bool remove_message, int context_size=0);
    mutex mtx;
};

#endif
