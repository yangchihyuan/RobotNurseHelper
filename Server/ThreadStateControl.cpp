#include "ThreadStateControl.hpp"
#include <fstream>
#include "utility_time.hpp"

#include <algorithm> // For transform()
#include <cstdlib>   // For rand() and srand()
#include <ctime>     // For time()
#include "utility_json.hpp"
#include "ollama.hpp"

ThreadStateControl::ThreadStateControl()
{
    msPatientName = ""; 
}

ThreadStateControl::~ThreadStateControl()
{
}

void ThreadStateControl::InitializeStates()
{
    LoadJSONFile(mStates, mpsetting->StateControlFile);
    for (size_t i = 0; i < mStates.size(); i++)
    {
        mStates[i].m_secDurationLimit = chrono::seconds(mStates[i].iDurationLimitSeconds);
    }
    LoadPatientTitles();
}

void ThreadStateControl::NextState()
{
    m_iStateIndex++;
}

void ThreadStateControl::run()
{
    chrono::time_point<chrono::system_clock> current_time;

    auto faceExpressionForState = [this]()
    {
        string faceExpression = mStates[m_iStateIndex].sFace;
        if ((m_iStateIndex >= 1 && m_iStateIndex <= 3) ||
            (m_iStateIndex >= 11 && m_iStateIndex <= 14) ||
            (m_iStateIndex >= 17 && m_iStateIndex <= 22))
        {
            faceExpression = m_iStateIndex >= 1 && m_iStateIndex <= 3 ? "serious" : "happy";
        }
        return faceExpression.empty() ? "default" : faceExpression;
    };

    // wait until robot is connected.
    mutex mtx;
    unique_lock<mutex> lk(mtx);
    cond_var_state_control.wait(lk);
    chrono::milliseconds tolerance_duration(1000); 

    // initialize the random seed.
    srand(time(0));
    bool bAccumulateConversation = false;
    string str_inserted_assistant_message;
    string str_assistant_message;
    string str_voice_source = "Patient"; 

    while (b_WhileLoop)
    {
        current_time = chrono::system_clock::now();

        if (mStates[m_iStateIndex].bInitial)
        {
            cout << "Enter state " << m_iStateIndex << endl;

            // Replace variables for both display and speech
            string sReplacedDisplayString = "";
            if (mStates[m_iStateIndex].sDisplayString != "")
            {
                sReplacedDisplayString = ReplaceVariables(mStates[m_iStateIndex].sDisplayString);
                emit playTextRequest(sReplacedDisplayString.c_str());
            }
            else if (mStates[m_iStateIndex].sImageFileName != "")
            {
                emit playImageRequest(mStates[m_iStateIndex].sImageFileName.c_str());
            }

            for (string s : mStates[m_iStateIndex].v_str_Action)
            {
                if (s.find("PlayVideo:") != string::npos)
                {
                    size_t pos = s.find(":");
                    emit playVideoRequest(s.substr(pos + 1).c_str());
                    str_voice_source = "Video";
                }

                if (s == "KeepSilent")
                {
                    mbKeepSilent = true;
                }

                if (s == "KeepSilentOff")
                {
                    mbKeepSilent = false;
                }

                if (s == "AccumulateConversation")
                {
                    mStates[m_iStateIndex].message_history = mStates[m_iStateIndex - 1].message_history;
                    bAccumulateConversation = true;
                }

                if (s.find("InsertAssitantMessage:") != string::npos)
                {
                    size_t pos = s.find(":");
                    str_inserted_assistant_message = s.substr(pos + 1);
                }
            }

            mStates[m_iStateIndex].bInitial = false;
            mStates[m_iStateIndex].m_Start_time = chrono::system_clock::now();
            
            if (mStates[m_iStateIndex].m_strFirstSentence != "")
            {
                string sReplacedFirstSentence = ReplaceVariables(mStates[m_iStateIndex].m_strFirstSentence);
                str_assistant_message = sReplacedFirstSentence + str_inserted_assistant_message;
                str_inserted_assistant_message = ""; 
                mStates[m_iStateIndex].message_history.push_back("Assistant: " + str_assistant_message);
                
                RobotCommandProtobuf::RobotCommand command;
                command.set_speak_sentence(sReplacedFirstSentence); // Spoken sentence now includes the extracted name!

                // Realistic dynamic expression mapping based on state index or purpose
                command.set_sface(faceExpressionForState());
                
                if (mStates[m_iStateIndex].sMotion != "")
                {
                    command.set_smotion(mStates[m_iStateIndex].sMotion);
                }
                
                m_pSendMessageManager->AddMessage(command);
                mbTTSComplete = false;

                // Display on UI
                mpThreadLLM->strResponse = sReplacedFirstSentence;
                mpThreadLLM->b_new_LLM_response = true;
            }
            mbWaitForTTSComplete = mStates[m_iStateIndex].bWaitForTTSComplete;
            mbOldStateComplete = false;                                        
            mbReadyToChangeState = false;

        } // if(mStates[m_iStateIndex].bInitial)

        if (mStates[m_iStateIndex].sStateType == "SaySomething" || mStates[m_iStateIndex].sStateType == "PlayVideo")
        {
            if (current_time - mStates[m_iStateIndex].m_Start_time > mStates[m_iStateIndex].m_secDurationLimit)
            {
                mbReadyToChangeState = true;
                mbOldStateComplete = true;
            }
        }
        else if (mStates[m_iStateIndex].sStateType == "Conversation" || mStates[m_iStateIndex].sStateType == "RobotAskPatientAnswer")
        {
            if (mbWaitForTTSComplete)
            {
                if (mbTTSComplete)
                {
                    WhisperData WhisperResult = mpThreadWhisper->getLatestResult();
                    
                    if (WhisperResult.tSpeechStart > mtimestamp_TTSComplete - tolerance_duration ||
                        (WhisperResult.tSpeechEnd > mtimestamp_TTSComplete && current_time - WhisperResult.tSpeechEnd > 3s))
                    {
                        cout << "Patient's response: " << WhisperResult.sOutput << endl;

                        if (mStates[m_iStateIndex].m_strStateName == "Greeting 1 and ask name")
                        {
                            if (!WhisperResult.sOutput.empty())
                            {
                                msPatientName = GetPatientName(WhisperResult.sOutput);
                            }
                            cout << "Extracted patient name: " << msPatientName << endl;
                        }

                        if (mStates[m_iStateIndex].v_str_KeyWordMoveToNextState.size() > 0)
                        {
                            for (size_t k = 0; k < mStates[m_iStateIndex].v_str_KeyWordMoveToNextState.size(); k++)
                            {
                                if (WhisperResult.sOutput.find(mStates[m_iStateIndex].v_str_KeyWordMoveToNextState.at(k)) != string::npos)
                                {
                                    mbReadyToChangeState = true;
                                    mbOldStateComplete = true;
                                    break;
                                }
                            }
                        }

                        mStates[m_iStateIndex].message_history.push_back("User: " + WhisperResult.sOutput);

                        if (mStates[m_iStateIndex].sStateType == "RobotAskPatientAnswer")
                        {
                            mbReadyToChangeState = true; 
                        }

                        if (mbReadyToChangeState)
                        {
                            mbOldStateComplete = true;
                        }
                        else if (mStates[m_iStateIndex].sStateType == "Conversation")
                        {
                            DumpHistoryMessages(mStates[m_iStateIndex].message_history);
                            mbWaitForTTSComplete = false;
                            mbWaitForLLMResult = true;
                            LLMTask task;

                            vector<string> truncated_history = mStates[m_iStateIndex].message_history;
                            if (truncated_history.size() > 6)
                            {
                                truncated_history.erase(truncated_history.begin(), truncated_history.end() - 6);
                            }

                            task.str_message = ConvertMessageHistoryToString(truncated_history);
                            task.timestamp = chrono::system_clock::now();
                            task.bNotify = true;
                            mpThreadLLM->AddQueue(task);
                            mpThreadLLM->cond_var_thread_LLM.notify_one();
                        }
                    }
                }
            }

            if (mbWaitForLLMResult)
            {
                if (mbLLMResult)
                {
                    mStates[m_iStateIndex].message_history.push_back(msLLMResult);

                    if (!mbKeepSilent)
                    {
                        RobotCommandProtobuf::RobotCommand command;
                        size_t pos = msLLMResult.find("Assistant: ");
                        string str_assistant_message;
                        if (pos != string::npos)
                        {
                            str_assistant_message = msLLMResult.substr(pos + string("Assistant: ").length());
                        }
                        else
                        {
                            str_assistant_message = msLLMResult;
                        }
                        str_assistant_message = ReplaceVariables(str_assistant_message);
                        command.set_speak_sentence(str_assistant_message);
                        command.set_sface(faceExpressionForState());

                        if (mStates[m_iStateIndex].vSmallMotion.size() > 0)
                        {
                            int randomNumber = (rand() % mStates[m_iStateIndex].vSmallMotion.size());
                            command.set_smotion(mStates[m_iStateIndex].vSmallMotion.at(randomNumber));
                        }

                        m_pSendMessageManager->AddMessage(command);
                        mpThreadLLM->strResponse = str_assistant_message;
                        mpThreadLLM->b_new_LLM_response = true;
                        mbTTSComplete = false;
                        mbWaitForTTSComplete = true;
                        mbWaitForLLMResult = false;
                        mbLLMResult = false;
                    }
                }
            }

            if (current_time - mStates[m_iStateIndex].m_Start_time > mStates[m_iStateIndex].m_secDurationLimit)
            {
                mbReadyToChangeState = true;
            }
        }
        else
        {
            cout << "Unknown state type: " << mStates[m_iStateIndex].sStateType << endl;
        }

        if (mbReadyToChangeState && mbOldStateComplete)
        {
            if (mStates[m_iStateIndex].bEndState)
            {
                b_WhileLoop = false;
            }
            else
            {
                m_iStateIndex = mStates[m_iStateIndex].iNextStateIndex;
                mbReadyToChangeState = false;
            }
        }

        this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void ThreadStateControl::NotifyEvent(string description, chrono::time_point<chrono::system_clock> timestamp, string sLLMResult)
{
    if (description == "onTTSComplete")
    {
        mbTTSComplete = true;
        mtimestamp_TTSComplete = timestamp;
    }
    else if (description == "onLLMResult")
    {
        mbLLMResult = true;
        mtimestamp_LLMResult = timestamp;
        msLLMResult = sLLMResult;
    }
    else if (description == "onActivityRestart")
    {
        mbActivity_mbtx_Complete = true;
        mtimestamp_Activity_mbtx_Complete = timestamp;
    }
    else if (description == "onVideoComplete")
    {
        cout << "NotifyEvent onVideoComplete" << endl;
        mbReadyToChangeState = true;
        mbOldStateComplete = true;
        mtimestamp_VideoComplete = timestamp;
    }
}

void ThreadStateControl::SetIntialStateIndex(int N)
{
    if (N < 0 || N >= m_iNumberOfStates)
        throw "Invalid stage number: " + std::to_string(N);
    m_iStateIndex = N;
}

string ThreadStateControl::GetPatientName(string input_sentence)
{
    ollama::options options;
    options["seed"] = rand();
    options["temperature"] = 0.1;
    options["num_ctx"] = 131072;

    string prompt = "You are a precise name extractor. Look at the speaker's sentence and extract only their name or what they want to be called. "
                     "Rules: 1. Return ONLY the name (e.g., if they say 'My name is KK', return 'KK'). 2. Do not include punctuation, quotes, or explanation. "
                     "Sentence: \"" +
                     input_sentence + "\"";
    string ModelName = "gemma3:1b";
    string name = ollama::generate(ModelName, prompt, options);

    name.erase(0, name.find_first_not_of(" \n\r\t"));
    name.erase(name.find_last_not_of(" \n\r\t") + 1);

    if (name.empty())
    {
        name = input_sentence;
    }

    cout << "Extracted patient name by LLM: " << name << endl;

    return name;
}

string ThreadStateControl::ConvertMessageHistoryToString(vector<string> message_history)
{
    string result = "";
    for (const auto &message : message_history)
    {
        result += message + "\n";
    }
    return result;
}

void ThreadStateControl::DumpHistoryMessages(vector<string> messages)
{
    cout << "---- Dumping message history ----" << endl;
    for (const auto &message : messages)
    {
        cout << message << endl;
    }
    cout << "---- End of message history ----" << endl;
}

string ThreadStateControl::ReplaceVariables(string sentence)
{
    string result = sentence;

    const string namePlaceholder = "{PatientName}";
    const string nameToUse = msPatientName.empty() ? "Friend" : msPatientName;
    size_t namePosition = 0;
    while ((namePosition = result.find(namePlaceholder, namePosition)) != string::npos)
    {
        result.replace(namePosition, namePlaceholder.length(), nameToUse);
        namePosition += nameToUse.length();
    }

    const string titlePlaceholder = "{PatientTitle}";
    size_t titlePosition = 0;
    while ((titlePosition = result.find(titlePlaceholder, titlePosition)) != string::npos)
    {
        result.replace(titlePosition, titlePlaceholder.length(), msPatientTitle);
        titlePosition += msPatientTitle.length();
    }
    
    return result;
}

void ThreadStateControl::Restart()
{
    m_iStateIndex = 0;
    for (auto &state : mStates)
    {
        state.bInitial = true;
        state.bWaitForTTSComplete = true;
        state.bEndState = false;
    }
}

void ThreadStateControl::LoadPatientTitles()
{
    m_mapPatientTitles.clear();
}