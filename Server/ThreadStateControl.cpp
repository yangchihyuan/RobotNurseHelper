/*

*/

#include "ThreadStateControl.hpp"
#include "utility_time.hpp"

#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()
#include "utility_json.hpp"

ThreadStateControl::ThreadStateControl()
{

}

ThreadStateControl::~ThreadStateControl()
{
    
}

void ThreadStateControl::InitializeStates()
{
    LoadJSONFile(msetting, "json/Setting.json");
    LoadJSONFile(mStates, msetting.StateControlFile);
    for( size_t i = 0; i < mStates.size(); i++)
    {
        mStates[i].m_secDurationLimit = chrono::seconds(mStates[i].iDurationLimitSeconds);
    }
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
    string assistant_message_str;
    string str_voice_source = "Patient"; //or "Video"
    while(b_WhileLoop)
    {
        current_time = chrono::system_clock::now();

        if(mStates[m_iStateIndex].bInitial)
        {
            cout << "Enter state " << m_iStateIndex << endl;
            for( string s : mStates[m_iStateIndex].v_str_Action)
            {
                if( s.find("PlayVideo:") != string::npos )
                {
                    size_t pos = s.find(":");
                    emit playVideoRequest(s.substr(pos + 1).c_str());
                    str_voice_source = "Video";
                }

                if( s == "PlayVideo")
                {
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
                    assistant_message_str = s.substr(pos + 1);
                }
            }
            
            mStates[m_iStateIndex].bInitial = false;
            mStates[m_iStateIndex].m_Start_time = chrono::system_clock::now();
            if( mStates[m_iStateIndex].m_strFirstSentence != "")
            {
                if( !bAccumulateConversation)
                {
                    ollama::message system_message("system", mStates[m_iStateIndex].m_strSystemMessage);
                    mStates[m_iStateIndex].message_history.push_back(system_message);
                }
                ollama::message assistant_message("assistant", mStates[m_iStateIndex].m_strFirstSentence + assistant_message_str);
                assistant_message_str = "";   //clear the string
                mStates[m_iStateIndex].message_history.push_back(assistant_message);
                RobotCommandProtobuf::RobotCommand command;
                command.set_speak_sentence(mStates[m_iStateIndex].m_strFirstSentence);
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
                mpThreadOllama->strResponse = mStates[m_iStateIndex].m_strFirstSentence;
                mpThreadOllama->b_new_LLM_response = true;

                //prepare random number
                //if( mStates[m_iStateIndex].vSmallMotion.size() > 0 )
                //    pDistribution = unique_ptr<uniform_int_distribution<int>>(new uniform_int_distribution<int>(0,mStates[m_iStateIndex].vSmallMotion.size()));
            }
            mbWaitForTTSComplete = mStates[m_iStateIndex].bWaitForTTSComplete; 
            mbOldStateComplete = false;              //In the initial state, the old state is not complete.
            mbReadyToChangeState = false;

        }

        if( mStates[m_iStateIndex].sStateType == "PlayVideo")
        {
            //Monitor WhisperResult, but do not generate LLM response
            WhisperData WhisperResult = mpThreadWhisper->getLatestResult();
//            cout << "WhisperResult: " << WhisperResult.sOutput << endl;
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

        }
        else if( mStates[m_iStateIndex].sStateType == "Conversation")
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

                        ollama::message user_message("user", WhisperResult.sOutput);
                        mStates[m_iStateIndex].message_history.push_back(user_message);

                        if(mbReadyToChangeState )
                        {
    //                        cout << "(G) mbOldStateComplete = true;" << endl;
                            mbOldStateComplete = true;
                        }
                        else
                        {
                            //generate LLM response
                            //debug
                            //cout << "(H)" << endl;
                            DumpOllamaMessages(mStates[m_iStateIndex].message_history);
                            //generate LLM result;
                            mbWaitForTTSComplete = false;
                            mbWaitForLLMResult = true;
                            OllamaTask task;
                            task.message_history = mStates[m_iStateIndex].message_history;
                            task.timestamp = chrono::system_clock::now();
                            task.bNotify = true;
                            mpThreadOllama->AddQueue(task);
                            mpThreadOllama->cond_var_ollama.notify_one();
                        }
                    }
                }
            }

            if( mbWaitForLLMResult)
            {
                if( mbLLMResult)
                {
                    ollama::message assistant_message("assistant", msLLMResult);
                    mStates[m_iStateIndex].message_history.push_back(assistant_message);
                    //debug
                    if(false)
                    {
                        //debug
                        //cout << "(D)" << endl;
                        DumpOllamaMessages(mStates[m_iStateIndex].message_history);
                    }
                    
                    //Here is the command to let Kebbi speak the LLM result
                    if( !mbKeepSilent)
                    {
                        RobotCommandProtobuf::RobotCommand command;
                        command.set_speak_sentence(msLLMResult);
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
        else //neither PlayVideo nor Conversation
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
