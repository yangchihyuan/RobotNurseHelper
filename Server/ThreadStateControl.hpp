#ifndef __THREAD_STATE_CONTROL_hpp__
#define __THREAD_STATE_CONTROL_hpp__

#include <chrono>
#include <string>
#include <map>
#include <QThread>
#include "SendMessageManager.hpp"
#include "ThreadWhisper.hpp"
#include "ThreadLLM.hpp"
#include "ThreadProcessImage.hpp"
#include "utility_KebbiMotion.hpp"
#include <nlohmann/json.hpp>
#include "VideoWindow.hpp"
#include "Setting.hpp"

using namespace std;

struct State
{
    // StateSetting
    int iStateIndex;
    string m_strStateName;
    int iDurationLimitSeconds; // JSON supports int but not chrono::seconds
    string m_strFirstSentence; // The first sentence to speak when enter this state
    string sFace;
    string sMotion;
    string sStateType;
    vector<string> vSmallMotion;
    int iNextStateIndex = -1; // bug proofing
    vector<string> v_str_KeyWordMoveToNextState;
    vector<string> v_str_Action;

    // Dynarmic data
    chrono::time_point<std::chrono::system_clock> m_Start_time;
    bool bInitial = true;
    bool bWaitForTTSComplete = true;
    bool bEndState = false;
    std::vector<std::string> message_history;

    chrono::seconds m_secDurationLimit; // Converted from iDurationLimitSeconds

    // Special variables for some states
    int iStage = 0; // Only wok for the Ask Dance state. Stage 0 is conversation, Stage 1 is dance performance.
    string sImageFileName;
    string sDisplayString;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(State, iStateIndex, m_strStateName, iDurationLimitSeconds,
                                   m_strFirstSentence, iNextStateIndex, sFace, sMotion, vSmallMotion, v_str_KeyWordMoveToNextState,
                                   v_str_Action, sStateType, sImageFileName, sDisplayString)
class ThreadLLM; // Because ThreadLLM.hpp and ThreadStateControl.hpp include each other, I need to use forward declaration

class ThreadStateControl : public QThread
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
    ThreadLLM *mpThreadLLM;
    ThreadProcessImage *mpThreadProcessImage;
    void NotifyEvent(string description, chrono::time_point<chrono::system_clock> timestamp, string sLLMResult = "");
    condition_variable cond_var_state_control;
    void SetIntialStateIndex(int index);

    VideoWindow *pVideoWindow = nullptr;
    Setting *mpsetting = nullptr;
    void Restart();

signals:
    void playVideoRequest(const QString &videoPath);
    void playImageRequest(const QString &imagePath);
    void playTextRequest(const QString &text);

protected:
    void run();
    string ConvertMessageHistoryToString(vector<string> message_history);
    void DumpHistoryMessages(vector<string> messages);
    string ReplaceVariables(string sentence);

    vector<State> mStates;
    int m_iNumberOfStates = 10;
    int m_iStateIndex = 0;

    bool mbTTSComplete = false;
    bool mbWaitForTTSComplete = false;
    chrono::time_point<chrono::system_clock> mtimestamp_TTSComplete;
    chrono::time_point<chrono::system_clock> mtimestamp_VideoComplete;

    bool mbLLMResult = false;
    bool mbWaitForLLMResult = false;
    chrono::time_point<chrono::system_clock> mtimestamp_LLMResult;
    string msLLMResult;
    int chosen_dance = 0;

    bool mbActivity_mbtx_Complete = false;
    chrono::time_point<chrono::system_clock> mtimestamp_Activity_mbtx_Complete;

    bool mbKeepSilent = false;
    bool mbReadyToChangeState = false; // This variable is used to decide whether to chat with LLM in a loop.
    bool mbOldStateComplete = false;   // This varaible is used to check whether the time is up, or some keyword is detected.
    Setting msetting;

    string msPatientName = "";  // default name is "patient"
    string msPatientTitle = ""; // e.g. "Mr." or "Miss". Default is an empty string; it is derived from the patient's age/gender via m_mapPatientTitles in ReplaceVariables().
    string GetPatientName(string input_sentence);
    std::map<std::string, std::string> m_mapPatientTitles;
    void LoadPatientTitles();
};

#endif