#ifndef __THREAD_STAGE_CONTROL_hpp__
#define __THREAD_STAGE_CONTROL_hpp__

#include <chrono>
#include <string>
#include <QThread>
#include "ollama.hpp"
#include "SendMessageManager.hpp"

using namespace std;

struct State
{
    string m_strStateName;
    chrono::seconds m_secDurationLimit;
    string m_strSummary;
    chrono::time_point<std::chrono::high_resolution_clock> m_Start_time;
    string m_strSystemMessage;
    string m_strFirstSentence;
    ollama::messages message_history;
    bool bInitial = true;
};

class ThreadStateControl: public QThread
{
    Q_OBJECT

public:
    ThreadStateControl();
    ~ThreadStateControl();

    bool b_WhileLoop = true;
    void InitializeStates();
    void NextState();
    condition_variable cond_var_state_control;
    SendMessageManager *m_pSendMessageManager;

protected:
    void run();
    vector<State> mStates;
    int m_iNumberOfStates = 9;
    int m_iStateIndex = 0;
};

#endif