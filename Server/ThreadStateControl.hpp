#ifndef __THREAD_STAGE_CONTROL_hpp__
#define __THREAD_STAGE_CONTROL_hpp__

#include <chrono>
#include <string>
#include <QThread>
#include "ollama.hpp"
#include "SendMessageManager.hpp"
#include "ThreadWhisper.hpp"
#include "ThreadOllama.hpp" 

using namespace std;

struct State
{
    string m_strStateName;
    chrono::seconds m_secDurationLimit;
    string m_strSummary;
    chrono::time_point<std::chrono::system_clock> m_Start_time;
    string m_strSystemMessage;
    string m_strFirstSentence;
    ollama::messages message_history;
    bool bInitial = true;
    bool bWaitForTTSComplete = true;
    int iNextStateIndex = -1;  //bug proofing
    bool bEndState = false;
    int iStage = 0;
    string sFace;
    string sMotion;
};

class ThreadOllama; //Because ThreadOllama.hpp and ThreadStateControl.hpp include each other, I need to use forward declaration


class ThreadStateControl: public QThread
{
    Q_OBJECT

public:
    ThreadStateControl();
    ~ThreadStateControl();

    bool b_WhileLoop = true;
    void InitializeStates();
    void NextState();
    SendMessageManager *m_pSendMessageManager;
    ThreadWhisper *mpThreadWhisper;
    ThreadOllama *mpThreadOllama;
    void NotifyEvent(string description, chrono::time_point<chrono::system_clock> timestamp, string sLLMResult = "");
    condition_variable cond_var_state_control;

protected:
    void run();
    vector<State> mStates;
    int m_iNumberOfStates = 10;
    int m_iStateIndex = 0;

    bool mbTTSComplete = false;
    bool mbWaitForTTSComplete = false;
    chrono::time_point<chrono::system_clock> mtimestamp_TTSComplete;

    bool mbLLMResult = false;
    bool mbWaitForLLMResult = false;
    chrono::time_point<chrono::system_clock> mtimestamp_LLMResult;
    string msLLMResult;
    int chosen_dance = 0;
};

#endif