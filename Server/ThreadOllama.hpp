#ifndef Ollama_hpp
#define Ollama_hpp

#include <QThread>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include "ollama.hpp"

using namespace std;

extern string chosen_action, summary;

class ThreadOllama: public QThread
{
    Q_OBJECT

public:
    ThreadOllama();
    ~ThreadOllama();

    bool b_WhileLoop = true;
    condition_variable cond_var_ollama;
    string strPrompt;
    bool b_new_LLM_response = false;
    string strResponse;
    string str_system_message;
    string str_system_message_list[4];
    string ModelName = "gemma3:4b";

protected:
    void run();
    string validate_conversation(ollama::options options, ollama::messages &message_history, string &prompt, bool remove_message, int context_size=0);
    mutex mtx;
};

#endif
