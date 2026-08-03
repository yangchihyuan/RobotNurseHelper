#include "ThreadStateControl.hpp"
#include <fstream>
#include "utility_time.hpp"

#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()
#include "utility_json.hpp"
#include "ollama.hpp"

ThreadStateControl::ThreadStateControl()
{

}

ThreadStateControl::~ThreadStateControl()
{
    
}

void ThreadStateControl::SetSettingFile(const QString &filePath)
{
    LoadJSONFile(msetting, filePath.toStdString());
    //After loading the setting, I can set the initial state of the state control thread.
    InitializeStates();
}

void ThreadStateControl::InitializeStates()
{
    LoadJSONFile(mStates, msetting.StateControlFile);
    for( size_t i = 0; i < mStates.size(); i++)
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

    //wait until Kebbi is connected.
    mutex mtx;
    unique_lock<mutex> lk(mtx);
    cond_var_state_control.wait(lk);
    chrono::milliseconds tolerance_duration(1000);      //tolerance for onTTSComplete and patient's tSpeechStart
    chrono::seconds dance_wait_duration;
    chrono::time_point<chrono::system_clock> dance_start_time;

    //initialize the random seed.
    srand(time(0));
    bool bAccumulateConversation = false;
    string str_inserted_assistant_message;
    string str_assistant_message;
    string str_voice_source = "Patient"; //or "Video"
    while(b_WhileLoop)
    {
        current_time = chrono::system_clock::now();

        if(mStates[m_iStateIndex].bInitial)
        {
            cout << "Enter state " << m_iStateIndex << endl;
            
            if(mStates[m_iStateIndex].sDisplayString != "")
            {
                emit playTextRequest(mStates[m_iStateIndex].sDisplayString.c_str());
            }
            else if( mStates[m_iStateIndex].sImageFileName != "" )
            {
                emit playImageRequest(mStates[m_iStateIndex].sImageFileName.c_str());
            }

            for( string s : mStates[m_iStateIndex].v_str_Action)
            {
                if( s.find("PlayVideo:") != string::npos )
                {
                    size_t pos = s.find(":");
                    emit playVideoRequest(s.substr(pos + 1).c_str());
                    str_voice_source = "Video";
                }

                if( s == "KeepSilent" )
                {
                    //Do not send any message for a period of time
                    mbKeepSilent = true;
                }

                if( s == "KeepSilentOff" )
                {
                    //Do not send any message for a period of time
                    mbKeepSilent = false;
                }

                if( s == "AccumulateConversation" )
                {
                    //Do not clear the message history when entering this state.
                    mStates[m_iStateIndex].message_history = mStates[m_iStateIndex-1].message_history;
                    bAccumulateConversation = true;
                }

                if( s.find("InsertAssitantMessage:") != string::npos )
                {
                    size_t pos = s.find(":");
                    str_inserted_assistant_message = s.substr(pos + 1);
                }
            }
            
            mStates[m_iStateIndex].bInitial = false;
            mStates[m_iStateIndex].m_Start_time = chrono::system_clock::now();
            if( mStates[m_iStateIndex].m_strFirstSentence != "")
            {
                string sReplacedFirstSentence = ReplaceVariables(mStates[m_iStateIndex].m_strFirstSentence);
                str_assistant_message = sReplacedFirstSentence + str_inserted_assistant_message;
                str_inserted_assistant_message = "";   //clear the string
                mStates[m_iStateIndex].message_history.push_back("Assistant: "+str_assistant_message);
                RobotCommandProtobuf::RobotCommand command;
                command.set_speak_sentence(sReplacedFirstSentence);
                command.set_sface(mStates[m_iStateIndex].sFace);
                if(mStates[m_iStateIndex].sMotion != "")
                {
                    command.set_smotion(mStates[m_iStateIndex].sMotion);
                    if( KebbiMoveHeadDuringMotion(command.smotion()) )
                    {
                        mpThreadProcessImage->NotifyEvent("KebbiMoveHeadDuringMotion", chrono::system_clock::now()); //pause watching patient
                    }
                }
                m_pSendMessageManager->AddMessage(command);
                mbTTSComplete = false;

                //just for display on UI
                mpThreadLLM->strResponse = sReplacedFirstSentence;
                mpThreadLLM->b_new_LLM_response = true;
            }
            mbWaitForTTSComplete = mStates[m_iStateIndex].bWaitForTTSComplete;      //now it is always true
            mbOldStateComplete = false;              //In the initial state, the old state is not complete.
            mbReadyToChangeState = false;

        }  //if(mStates[m_iStateIndex].bInitial)

        if( mStates[m_iStateIndex].sStateType == "SaySomething" || mStates[m_iStateIndex].sStateType == "PlayVideo")   //Robot does not wait for patient's response
        {
            if( current_time - mStates[m_iStateIndex].m_Start_time > mStates[m_iStateIndex].m_secDurationLimit)
            {
                mbReadyToChangeState = true;
                mbOldStateComplete = true;
            }
        }
        else if( mStates[m_iStateIndex].sStateType == "Conversation" || mStates[m_iStateIndex].sStateType == "RobotAskPatientAnswer")
        {
            if( mbWaitForTTSComplete)
            {
                if( mbTTSComplete)
                {
                    WhisperData WhisperResult = mpThreadWhisper->getLatestResult();
                    if( mStates[m_iStateIndex].m_strStateName == "Ask dance")
                    {
                        if( mStates[m_iStateIndex].iStage == 0 )  //Conversation
                        {
                            if( current_time - mStates[m_iStateIndex].m_Start_time > mStates[m_iStateIndex].m_secDurationLimit)
                            {
                                cout << "Time out, choose Egyptian dance." << endl;
                                chosen_dance = 1;
                                dance_wait_duration = chrono::seconds(73);
                                cout << "CHOSEN_DANCE: " << chosen_dance << "\n"; 
                            }
                            else        //string comparison
                            {
                                //ToDo: I need to clear this piece of code. This is Mohamed's requirement.
                                if(WhisperResult.sOutput.find("及") != string::npos || WhisperResult.sOutput.find("吉") != string::npos || WhisperResult.sOutput.find("極") != string::npos || WhisperResult.sOutput.find("級") != string::npos)
                                {
                                    chosen_dance = 1;
                                    dance_wait_duration = chrono::seconds(73);
                                }
                                else if (WhisperResult.sOutput.find("牛") != string::npos || WhisperResult.sOutput.find("仔") != string::npos )
                                {
                                    chosen_dance = 2;
                                    dance_wait_duration = chrono::seconds(81);
                                }
                            }

                            if( chosen_dance != 0 )
                            {
                                //debug
                                //cout << "(J) chosen_dance " << chosen_dance << endl;
                                RobotCommandProtobuf::RobotCommand dance_command;
                                dance_command.set_dancetype(chosen_dance);
                                m_pSendMessageManager->AddMessage(dance_command);
                                mStates[m_iStateIndex].iStage = 1; //Waiting for Dance Complete
                                dance_start_time = chrono::system_clock::now();
                            }
                        }
                        else if( mStates[m_iStateIndex].iStage == 1 )  //Wait for dance completion
                        {
                            //Use signal to control the state flow
                            //debug 
                            //cout << "(K) wait for mbActivity_mbtx_Complete as true" << endl;
                            if( mbActivity_mbtx_Complete )
                            {
                                mbOldStateComplete = true;
                                mbReadyToChangeState = true;

                                //turn of the face because the dance completes.
                                RobotCommandProtobuf::RobotCommand command;
                                command.set_hideface(0);
                                m_pSendMessageManager->AddMessage(command);
                            }
                        }
                    }
                    //There are two cases I accept a patient's response
                    //1. The patient's tSpeechStart is within the tolerance_duration before the mtimestamp_TTSComplete 
                    //2. The patient's tSpeechEnd is after the mtimestamp_TTSComplete, and current_time is a few seconds after tSpeechEnd
                    //2026/1/7 New case: the voice is from video playback rather than patient's response.
                    //That is the case I need to distinguish the voice source.
                    //Can I use any visual cue to help me determine whether the voice is from patient or video? It is very hard.
                    else if( WhisperResult.tSpeechStart > mtimestamp_TTSComplete - tolerance_duration || 
                            (WhisperResult.tSpeechEnd > mtimestamp_TTSComplete && current_time - WhisperResult.tSpeechEnd > 3s) )
                    {
                        //debug
                        cout << "Patient's response: " << WhisperResult.sOutput << endl;
                        
                        //get patient's name
                        if( mStates[m_iStateIndex].m_strStateName == "Greeting 1 and ask name" )
                        {
                            msPatientName = GetPatientName(WhisperResult.sOutput);
                            cout << "Extracted patient name: " << msPatientName << endl;
                        }


                        if( mStates[m_iStateIndex].v_str_KeyWordMoveToNextState.size() > 0)
                        {
                            for( size_t k = 0; k < mStates[m_iStateIndex].v_str_KeyWordMoveToNextState.size(); k++)
                            {
                                if( WhisperResult.sOutput.find( mStates[m_iStateIndex].v_str_KeyWordMoveToNextState.at(k) ) != string::npos)
                                {
                                    mbReadyToChangeState = true;
                                    mbOldStateComplete = true;
                                    break;
                                }
                            }
                        }

                        mStates[m_iStateIndex].message_history.push_back("User: "+WhisperResult.sOutput);

                        if (mStates[m_iStateIndex].sStateType == "RobotAskPatientAnswer")
                        {
                            mbReadyToChangeState = true;   //After the patient answers the question, the robot can move to the next state without waiting for LLM response.
                        }

                        if(mbReadyToChangeState )
                        {
                            mbOldStateComplete = true;
                        }
                        else if (mStates[m_iStateIndex].sStateType == "Conversation")
                        {
                            //generate LLM response
                            //debug
                            DumpHistoryMessages(mStates[m_iStateIndex].message_history);
                            //generate LLM result;
                            mbWaitForTTSComplete = false;
                            mbWaitForLLMResult = true;
                            LLMTask task;
                            task.str_message = ConvertMessageHistoryToString( mStates[m_iStateIndex].message_history );
                            task.timestamp = chrono::system_clock::now();
                            task.bNotify = true;
                            mpThreadLLM->AddQueue(task);
                            mpThreadLLM->cond_var_thread_LLM.notify_one();
                        }
                    }
                }
            }   //if( mbWaitForTTSComplete)

            if( mbWaitForLLMResult)
            {
                if( mbLLMResult)
                {
                    //Ollama.hpp's format is different from AnythingLLM.
                    //Ollama.hpp does not have the "Assistant: " prefix in the response, while AnythingLLM has.
//                    mStates[m_iStateIndex].message_history.push_back("Assistant: "+msLLMResult);
                    mStates[m_iStateIndex].message_history.push_back(msLLMResult);
                    
                    //Here is the command to let Kebbi speak the LLM result
                    if( !mbKeepSilent)
                    {
                        RobotCommandProtobuf::RobotCommand command;
                        size_t pos = msLLMResult.find("Assistant: ");
                        string str_assistant_message;
                        if( pos != string::npos )
                        {
                            str_assistant_message = msLLMResult.substr(pos + string("Assistant: ").length());
                        }
                        else
                        {
                            str_assistant_message = msLLMResult;
                        }
                        command.set_speak_sentence(str_assistant_message);
                        //randomly choose a motion
                        if( mStates[m_iStateIndex].vSmallMotion.size() > 0)
                        {
                            int randomNumber = (rand() % mStates[m_iStateIndex].vSmallMotion.size());
                            command.set_smotion(mStates[m_iStateIndex].vSmallMotion.at(randomNumber));
                        }

                        m_pSendMessageManager->AddMessage(command);
                        mbTTSComplete = false;
                        mbWaitForTTSComplete = true;
                        mbWaitForLLMResult = false;
                        mbLLMResult = false;
                    }
                }
            }

            //Check if the time exceed the state limit
            if(current_time - mStates[m_iStateIndex].m_Start_time > mStates[m_iStateIndex].m_secDurationLimit)
            {
                mbReadyToChangeState = true;
            }
        }
        else //neither StateAndListen nor Conversation
        {
            //unknown state type
            cout << "Unknown state type: " << mStates[m_iStateIndex].sStateType << endl;
        }

        if( mbReadyToChangeState && mbOldStateComplete)
        {
            if(mStates[m_iStateIndex].bEndState)
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
    if( description == "onTTSComplete")
    {
        mbTTSComplete = true;
        mtimestamp_TTSComplete = timestamp;
        //debug
        //cout << "(E)" << endl;
        //cout << "NotifyEvent mtimestamp_TTSComplete " << ConvertTimeToString(mtimestamp_TTSComplete) << endl;
    }
    else if( description == "onLLMResult")
    {
        mbLLMResult = true;
        mtimestamp_LLMResult = timestamp;
        msLLMResult = sLLMResult;
        //debug
        //cout << "(H)" << endl;
        //cout << "NotifyEvent onLLMResult" << endl;
    }
    else if( description == "onActivityRestart")
    {
        mbActivity_mbtx_Complete = true;
        mtimestamp_Activity_mbtx_Complete = timestamp;
    }
    else if( description == "onVideoComplete")
    {
        //debug
        cout << "NotifyEvent onVideoComplete" << endl;
        mbReadyToChangeState = true;
        mbOldStateComplete = true;
        mtimestamp_VideoComplete = timestamp;     //In fact, it is not TTSComplete, but I reuse this variable to store the time when video is complete.
    }
}

//N is the initial state index
void ThreadStateControl::SetIntialStateIndex(int N) { 
    if( N < 0 || N >= m_iNumberOfStates)
    throw "Invalid stage number: " + std::to_string(N);
    m_iStateIndex = N; 
}


string ThreadStateControl::GetPatientName(string input_sentence){
    ollama::options options;
    //options["seed"] = 1;      
    options["seed"] = rand();
    options["temperature"] = 0.3;
    options["num_ctx"] = 131072; //number of context tokens, which is the maximum number of tokens the model can handle in a single request

    //Todo: I need to repleace the Chinese prompt to English prompt.
    // 建立更嚴謹的 Prompt，要求模型只輸出 JSON 或純名字
    string prompt = "你是一個機器人助手。請從以下句子中提取說話者的姓名。"
                    "規則：1.只回傳姓名 2.不要有任何標點或解釋。"
                    "句子：\"" + input_sentence + "\"";    
    // 呼叫 Ollama
    string ModelName = "gemma3:1b";
    string name = ollama::generate(ModelName, prompt, options);

    // 去除 LLM 可能誤加的空白或換行
    name.erase(0, name.find_first_not_of(" \n\r\t"));
    name.erase(name.find_last_not_of(" \n\r\t") + 1);

    cout << "Extracted patient name by LLM: " << name << endl;
    
    return name;
}

string ThreadStateControl::ConvertMessageHistoryToString(vector<string> message_history)
{
    string result = "";
    for( const auto& message : message_history)
    {
        result += message + "\n";
    }
    return result;
}

void ThreadStateControl::DumpHistoryMessages(vector<string> messages)
{
    cout << "---- Dumping message history ----" << endl;
    for( const auto& message : messages)
    {
        cout << message << endl;
    }
    cout << "---- End of message history ----" << endl;
}

string ThreadStateControl::ReplaceVariables(string sentence)
{
    //This function is used to replace the variables in the sentence with the real values. For example, replace {PatientName} with the real patient name.
    string result = sentence;
    if( result.find("{PatientName}") != string::npos)
    {
        result.replace(result.find("{PatientName}"), string("{PatientName}").length(), msPatientName);
    }

    if( result.find("{PatientTitle}") != string::npos)
    {
        string sPatientGender = mpThreadProcessImage->GetPatientGender(); //update the patient
        int iPatientAge = mpThreadProcessImage->GetPatientAge();
        if( iPatientAge != -1 )
        {
            if( iPatientAge < 10 )
            {
                msPatientTitle = m_mapPatientTitles["Child"];
            }
            else if( iPatientAge < 20 )
            {
                msPatientTitle = m_mapPatientTitles["Youth"];
            }
            else
            {
                if( sPatientGender == "Male" )
                {
                    msPatientTitle = m_mapPatientTitles["MaleAdult"];
                }
                else if ( sPatientGender == "Female" )
                {
                    if( iPatientAge < 40 )
                    {
                        msPatientTitle = m_mapPatientTitles["FemaleYoungAdult"];
                    }
                    else
                    {
                        msPatientTitle = m_mapPatientTitles["FemaleOlderAdult"];
                    }
                }
                else
                {
                    msPatientTitle = "";
                }
            }
        }

        result.replace(result.find("{PatientTitle}"), string("{PatientTitle}").length(), msPatientTitle);
    }
    return result;
}

void ThreadStateControl::Restart()
{
    m_iStateIndex = 0;
    for( auto& state : mStates)
    {
        state.bInitial = true;
        state.bWaitForTTSComplete = true;
        state.bEndState = false;
    }
}

void ThreadStateControl::LoadPatientTitles()
{
    m_mapPatientTitles.clear();

    // Set fallback defaults based on language
    if (msetting.Language == "English") {
        m_mapPatientTitles["Child"] = "Kid";
        m_mapPatientTitles["Youth"] = "Student";
        m_mapPatientTitles["MaleAdult"] = "Mr.";
        m_mapPatientTitles["FemaleYoungAdult"] = "Miss";
        m_mapPatientTitles["FemaleOlderAdult"] = "Ms.";
    } else {
        // Default to Chinese
        m_mapPatientTitles["Child"] = "小朋友";
        m_mapPatientTitles["Youth"] = "同學";
        m_mapPatientTitles["MaleAdult"] = "先生";
        m_mapPatientTitles["FemaleYoungAdult"] = "小姐";
        m_mapPatientTitles["FemaleOlderAdult"] = "女士";
    }

    // Try to load from dynamic text file
    string fileName = "PatientTitle_" + msetting.Language + ".txt";
    ifstream file(fileName);
    if (!file.is_open()) {
        if (msetting.Language == "Arabic") {
            fileName = "PatientTitle_English.txt";
            file.open(fileName);
        }
    }

    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (line.empty() || line[0] == '#') continue;

            size_t sep = line.find(':');
            if (sep != string::npos) {
                string key = line.substr(0, sep);
                string value = line.substr(sep + 1);

                // Trim key and value
                key.erase(0, key.find_first_not_of(" \t\r\n"));
                key.erase(key.find_last_not_of(" \t\r\n") + 1);
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);

                m_mapPatientTitles[key] = value;
            }
        }
        file.close();
    } else {
        cout << "Warning: Could not open patient title text file: " << fileName << ". Using defaults." << endl;
    }
}
