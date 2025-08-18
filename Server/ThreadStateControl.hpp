#ifndef __THREAD_STAGE_CONTROL_hpp__
#define __THREAD_STAGE_CONTROL_hpp__

#include <chrono>
#include <string>
#include <QThread>

using namespace std;

struct Stage
{
    string m_strStageName;
    chrono::seconds m_secDurationLimit;
    string m_strSummary;
    chrono::time_point<std::chrono::high_resolution_clock> m_Start_time;
    string m_strSystemMessage;
    string m_strFirstSentence;
};

class ThreadStageControl: public QThread
{
    Q_OBJECT

public:
    ThreadStageControl();
    ~ThreadStageControl();

protected:
    vector<Stage> mStages;
    int m_iNumberOfStages = 9;
    void InitializeStage();
};

#endif