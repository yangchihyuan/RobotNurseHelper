#ifndef Ollama_hpp
#define Ollama_hpp

#include <QThread>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include "ollama.hpp"

using namespace std;

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

protected:
    void run();
    mutex mtx;
};

#endif
