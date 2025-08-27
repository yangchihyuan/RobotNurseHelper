#include "ThreadStateControl.hpp"
#include "utility_time.hpp"

#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()
ThreadStateControl::ThreadStateControl()
{

}

ThreadStateControl::~ThreadStateControl()
{
    
}

void ThreadStateControl::InitializeStates()
{
    mStates.resize(m_iNumberOfStates);
    int state_index = 0;

    //state_index = 0;
    mStates[state_index].m_strStateName = "Wait for start";
    mStates[state_index].m_strSystemMessage = "";
    mStates[state_index].m_strFirstSentence = "我準備好了。";
    mStates[state_index].m_secDurationLimit = 500s;
    mStates[state_index].iNextStateIndex = 1;
    mStates[state_index].sFace = "TTS_PeaceA";
    mStates[state_index].sMotion = "666_BA_RArmR180";       //Raise Arm Right 180 degree

    //state_index = 1;
    state_index++;
    mStates[state_index].m_strStateName = "Warm up";
    mStates[state_index].m_strSystemMessage = R"(你是一台名叫凱比的醫療機器人，正在和一位年幼的小朋友病患聊天。請遵守以下規則：
        1. 回答要用非常簡單、親切的中文，不能使用其他語言。
        2. 一開始請輕鬆地問一些有趣的問題來暖場，例如：你最喜歡的顏色是什麼？你最喜歡哪種動物？你喜歡上什麼課？你現在是幾年級呢？
        3. 請不要重複或輸出你已經收到的資訊。
        4. 請不要輸出任何表情符號。
        5. 請不要輸出任何括號。
        )";
    mStates[state_index].m_strFirstSentence = "你好，很高興見到你，你今天過得好嗎？";
    mStates[state_index].m_secDurationLimit = 30s;
    mStates[state_index].iNextStateIndex = 2;
    mStates[state_index].sFace = "TTS_PeaceB";
    mStates[state_index].sMotion = "666_SP_Cheer";
    mStates[state_index].vSmallMotion.push_back("666_BA_ArmSCircle");
    mStates[state_index].vSmallMotion.push_back("666_BA_ArmSSquare");
    mStates[state_index].vSmallMotion.push_back("666_BA_RArmCircleL");
    mStates[state_index].vSmallMotion.push_back("666_BA_RArmCircleR");

    //state_index = 2;
    state_index++;
    mStates[state_index].m_strStateName = "Ask name";
    mStates[state_index].m_strSystemMessage = R"(你是一台名叫凱比的醫療機器人，正在與一位兒童病患交談。請遵守以下規則：
        1. 回答必須使用非常簡潔的中文，不能使用其他語言。
        2. 所有數字必須使用對應的繁體中文字表示，例如「一」、「二」、「三」，不可使用阿拉伯數字。
        3. 請不要輸出任何表情符號。
        4. 請不要輸出任何括號。
        )";
    mStates[state_index].m_strFirstSentence = "請問你叫什麼名字？";
    mStates[state_index].m_secDurationLimit = 10s;
    mStates[state_index].iNextStateIndex = 3;
    mStates[state_index].sFace = "TTS_PeaceA";
    mStates[state_index].sMotion = "666_SA_Think";

    //state_index = 3;
    state_index++;
    mStates[state_index].m_strStateName = "Ask age";
    mStates[state_index].m_strSystemMessage = R"(你是一台名叫凱比的醫療機器人，正在與一位兒童病患交談。請遵守以下規則：
        1. 回答必須使用非常簡潔的中文，不能使用其他語言。
        2. 所有數字必須使用對應的繁體中文字表示，例如「一」、「二」、「三」，不可使用阿拉伯數字。
        3. 請不要輸出任何表情符號。
        4. 請不要輸出任何括號。
        )";
    mStates[state_index].m_strFirstSentence = "請問你幾歲了？";
    mStates[state_index].m_secDurationLimit = 10s;
    mStates[state_index].iNextStateIndex = 4;
    mStates[state_index].sFace = "TTS_PeaceB";
    mStates[state_index].sMotion = "666_BA_Nodhead";

    //state_index = 4;
    state_index++;
    mStates[state_index].m_strStateName = "Ask symdrone";
    mStates[state_index].m_strSystemMessage = R"(你是一台名叫凱比的醫療機器人，正在與一位兒童病患交談。請遵守以下規則：
        1. 回答必須使用非常簡潔的中文，不能使用其他語言。
        2. 所有數字必須使用對應的繁體中文字表示，例如「一」、「二」、「三」，不可使用阿拉伯數字。
        3. 請不要輸出任何表情符號。
        4. 請不要輸出任何括號。
        )";
    mStates[state_index].m_strFirstSentence = "請問你生的是什麼病啊？";
    mStates[state_index].m_secDurationLimit = 10s;
    mStates[state_index].iNextStateIndex = 5;
    mStates[state_index].sFace = "TTS_SadnessA";
    mStates[state_index].sMotion = "666_DA_LookFor";

    //state_index = 5;
    state_index++;
    mStates[state_index].m_strStateName = "Ask one to five";
    mStates[state_index].m_strSystemMessage = R"(你是一台名叫凱比的醫療機器人，正在與一位兒童病患交談。請遵守以下規則：
        1. 回答必須使用非常簡潔的中文，不能使用其他語言。
        2. 所有數字必須使用對應的繁體中文字表示，例如「一」、「二」、「三」，不可使用阿拉伯數字。
        3. 請不要輸出任何表情符號。
        4. 請不要輸出任何括號。
        )";
    mStates[state_index].m_strFirstSentence = "請你用一到五的等級告訴我你現在的感覺如何？一是很不好，五是很好。";
    mStates[state_index].m_secDurationLimit = 10s;
    mStates[state_index].iNextStateIndex = 6;
    mStates[state_index].sFace = "TTS_PeaceA";
    mStates[state_index].sMotion = "666_DA_Take";

    //state_index = 6;
    state_index++;
    mStates[state_index].m_strStateName = "Ask dance";
    mStates[state_index].m_strSystemMessage = R"(你是一台名叫凱比的醫療機器人，正在與一位年幼的兒童病患交談。請遵守以下規則：
        1. 請避免提到自己。
        2. 詢問小朋友是否想讓你跳「埃及舞」或「牛仔舞」。
        3. 請不要輸出任何表情符號。
        4. 請不要輸出任何括號。
        )";
    mStates[state_index].m_strFirstSentence = "我會跳舞喲，我會跳埃及舞和牛仔舞，你想看我跳哪一種舞？";
    mStates[state_index].m_secDurationLimit = 15s;
    mStates[state_index].iNextStateIndex = 7;
    mStates[state_index].sFace = "TTS_Surprise";
    mStates[state_index].sMotion = "666_PE_Drums";

    //state_index = 7;
    state_index++;
    mStates[state_index].m_strStateName = "Guess animal";
    mStates[state_index].m_strSystemMessage = R"(你是一台名叫凱比的醫療機器人，正在與一位年幼的兒童病患交談。請遵守以下規則：
        1. 不要重複同樣的問題。
        2. 回答時請使用非常簡潔且友善的語氣。
        3. 每次回答只能包含一句或兩句簡短的句子。
        4. 和小朋友玩一個猜動物的遊戲：給出關於一種動物的簡短提示，讓小朋友猜。
        5. 如果小朋友猜錯，請提供一個友善的提示，讓他們再試一次。
        6. 如果小朋友提問，請回答他們的問題。
        7. 不要輸出任何表情符號。
        8. 不要輸出任何括號。
        )";
    mStates[state_index].m_strFirstSentence = "我們來玩一個遊戲吧。我來想一個動物，你來猜，好不好啊？";
    mStates[state_index].m_secDurationLimit = 60s;
    mStates[state_index].iNextStateIndex = 8;
    mStates[state_index].sFace = "TTS_PeaceB";    
    mStates[state_index].sMotion = "666_PE_Harmonica";
    mStates[state_index].vSmallMotion.push_back("666_BA_ArmSCircle");
    mStates[state_index].vSmallMotion.push_back("666_BA_ArmSSquare");
    mStates[state_index].vSmallMotion.push_back("666_BA_RArmCircleL");
    mStates[state_index].vSmallMotion.push_back("666_BA_RArmCircleR");

    //state_index = 8;
    state_index++;
    mStates[state_index].m_strStateName = "Tell a story";
    mStates[state_index].m_strSystemMessage = R"(你是一台名叫凱比的醫療機器人，正在與一位年幼的兒童病患交談。請遵守以下規則：
        1. 請說一個簡短有趣的故事逗病患開心。
        2. 接著請詢問小朋友是否對這個故事有任何問題想問。
        3. 不要輸出任何表情符號。
        4. 不要輸出任何括號。
        )";
    mStates[state_index].m_strFirstSentence = "我會說故事喲。我可以講各種各樣的故事，像是動物的故事、王子和公主的故事、魔法的故事、星星的故事，你想聽我講什麼樣的故事呢？";
    mStates[state_index].m_secDurationLimit = 100s;
    mStates[state_index].iNextStateIndex = 9;
    mStates[state_index].sFace = "TTS_PeaceA";
    mStates[state_index].sMotion = "666_PE_Sorcery";
    mStates[state_index].vSmallMotion.push_back("666_BA_ArmSCircle");
    mStates[state_index].vSmallMotion.push_back("666_BA_ArmSSquare");
    mStates[state_index].vSmallMotion.push_back("666_BA_RArmCircleL");
    mStates[state_index].vSmallMotion.push_back("666_BA_RArmCircleR");

    //state_index = 9;
    state_index++;
    mStates[state_index].m_strStateName = "Say goodbye";
    mStates[state_index].m_strSystemMessage = R"(你是一台名叫凱比的醫療機器人，正在與一位年幼的兒童病患交談。請遵守以下規則：
        1. 你要跟兒童病患道別了。
        2. 你要說很多好話祝他早日康復，重新快快樂樂的過生活。
        3. 不要輸出任何表情符號。
        4. 不要輸出任何括號。
        5. 不要提問任何問題。
        )";
    mStates[state_index].m_strFirstSentence = "今天很高興認識你。跟你聊了很多話，我很開心。希望你的病很快就會好起來，你能高高興興的回家。下次還有機會再和你聊。";
    mStates[state_index].m_secDurationLimit = 40s;
    mStates[state_index].iNextStateIndex = -1;
    mStates[state_index].bEndState = true;
    mStates[state_index].sFace = "TTS_PeaceB";
    mStates[state_index].sMotion = "666_PE_Hug";
    mStates[state_index].vSmallMotion.push_back("666_BA_ArmSCircle");
    mStates[state_index].vSmallMotion.push_back("666_BA_ArmSSquare");
    mStates[state_index].vSmallMotion.push_back("666_BA_RArmCircleL");
    mStates[state_index].vSmallMotion.push_back("666_BA_RArmCircleR");
}

void ThreadStateControl::NextState()
{
    m_iStateIndex++;
}

void ThreadStateControl::run()
{
    chrono::time_point<chrono::system_clock> current_time;
    bool bReadyToChangeState = false;
    bool bOldStateComplete = false;
    //something wrong here, ollama get sucked. I havn't figured out the reason.
    //to Warmup Ollama
//    OllamaTask task;
//    task.message_history = mStates[1].message_history;
//    task.timestamp = chrono::system_clock::now();
//    task.bNotify = false;
//    mpThreadOllama->AddQueue(task);
//    mpThreadOllama->cond_var_ollama.notify_one();

    //wait until Kebbi is connected.
    mutex mtx;
    unique_lock<mutex> lk(mtx);
    cond_var_state_control.wait(lk);
    chrono::milliseconds tolerance_duration(1000);      //tolerance for onTTSComplete and patient's tSpeechStart
    chrono::seconds dance_wait_duration;
    chrono::time_point<chrono::system_clock> dance_start_time;

    //initialize the random seed.
    srand(time(0));

    while(b_WhileLoop)
    {
        current_time = chrono::system_clock::now();

        if(mStates[m_iStateIndex].bInitial)
        {
            cout << "Enter state " << m_iStateIndex << endl;
            mStates[m_iStateIndex].bInitial = false;
            mStates[m_iStateIndex].m_Start_time = chrono::system_clock::now();
            if( mStates[m_iStateIndex].m_strFirstSentence != "")
            {
                ollama::message system_message("system", mStates[m_iStateIndex].m_strSystemMessage);
                mStates[m_iStateIndex].message_history.push_back(system_message);
                ollama::message assistant_message("assistant", mStates[m_iStateIndex].m_strFirstSentence);
                mStates[m_iStateIndex].message_history.push_back(assistant_message);
                RobotCommandProtobuf::RobotCommand command;
                command.set_speak_sentence(mStates[m_iStateIndex].m_strFirstSentence);
                command.set_sface(mStates[m_iStateIndex].sFace);
                if(mStates[m_iStateIndex].sMotion != "")
                {
                    command.set_smotion(mStates[m_iStateIndex].sMotion);
                    if( KebbiResetHead(command.smotion()) )
                    {
                        mpThreadProcessImage->NotifyEvent("KebbiResetHead", chrono::system_clock::now()); //pause watching patient
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
            bOldStateComplete = false;
            bReadyToChangeState = false;

        }

        if( mbWaitForTTSComplete)
        {
            if( mbTTSComplete)
            {
                WhisperData WhisperResult = mpThreadWhisper->getLatestResult();
                if( mStates[m_iStateIndex].m_strStateName == "Wait for start")
                {
                    if( WhisperResult.sOutput.find("開始") != string::npos || WhisperResult.sOutput.find("开始") != string::npos)
                    {
                        bReadyToChangeState = true;
                        bOldStateComplete = true;
                    }
                }
                else if( mStates[m_iStateIndex].m_strStateName == "Ask dance")
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
                        //Use time to control
/*
                        if(current_time - dance_start_time > dance_wait_duration)
                        {
                            bOldStateComplete = true;
                        }
*/
                        //Use signal to control
                        if( mbActivity_mbtx_Complete )
                        {
                            bOldStateComplete = true;
                        }
                    }
                }
                //There are two cases I accept a patient's response
                //1. The patient's tSpeechStart is within the tolerance_duration before the mtimestamp_TTSComplete 
                //2. The patient's tSpeechEnd is after the mtimestamp_TTSComplete, and current_time is a few seconds after tSpeechEnd
                else if( WhisperResult.tSpeechStart > mtimestamp_TTSComplete - tolerance_duration || 
                         (WhisperResult.tSpeechEnd > mtimestamp_TTSComplete && current_time - WhisperResult.tSpeechEnd > 3s) )
                {
                    //debug
                    if(false)
                    {
                        cout << "(F)" << endl;
                        cout << "mtimestamp_TTSComplete " << ConvertTimeToString(mtimestamp_TTSComplete) << endl;
                        cout << "mpThreadWhisper->result.tSpeechStart " << ConvertTimeToString(WhisperResult.tSpeechStart) << endl;
                        cout << "mpThreadWhisper->result.tSpeechEnd " << ConvertTimeToString(WhisperResult.tSpeechEnd) << endl;
                        cout << "mpThreadWhisper->result.tSTTComplete " << ConvertTimeToString(WhisperResult.tSTTComplete) << endl;
                        cout << "sOutput " << WhisperResult.sOutput << endl;
                    }
                    ollama::message user_message("user", WhisperResult.sOutput);
                    mStates[m_iStateIndex].message_history.push_back(user_message);

                    if(bReadyToChangeState )
                    {
                        cout << "(G) bOldStateComplete = true;" << endl;
                        bOldStateComplete = true;
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
                    cout << "(D)" << endl;
                    DumpOllamaMessages(mStates[m_iStateIndex].message_history);
                }
                
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

        //Check if the time exceed the state limit
        if(current_time - mStates[m_iStateIndex].m_Start_time > mStates[m_iStateIndex].m_secDurationLimit)
        {
            bReadyToChangeState = true;
        }

        if( bReadyToChangeState && bOldStateComplete)
        {
            if(mStates[m_iStateIndex].bEndState)
            {
                b_WhileLoop = false;
            }
            else
            {
                m_iStateIndex = mStates[m_iStateIndex].iNextStateIndex;
                bReadyToChangeState = false;
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
    else if( description == "onActivityResult")
    {
        mbActivity_mbtx_Complete = true;
        mtimestamp_Activity_mbtx_Complete = timestamp;
    }
    
}
