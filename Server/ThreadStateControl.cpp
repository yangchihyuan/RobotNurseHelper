#include "ThreadStateControl.hpp"

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
    mStates[state_index].m_secDurationLimit = 50s;

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
    mStates[state_index].m_secDurationLimit = 30s;

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
    mStates[state_index].m_secDurationLimit = 30s;

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
    mStates[state_index].m_secDurationLimit = 30s;

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
    mStates[state_index].m_secDurationLimit = 30s;

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
    mStates[state_index].m_secDurationLimit = 30s;

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
    mStates[state_index].m_secDurationLimit = 100s;
}

void ThreadStateControl::NextState()
{
    m_iStateIndex++;
}

void ThreadStateControl::run()
{

//    mutex mtx_state_control;
//    unique_lock<std::mutex> lk(mtx_state_control);
    chrono::time_point<chrono::system_clock> current_time;
    while(b_WhileLoop)
    {
//        cond_var_state_control.wait(lk);
        current_time = chrono::system_clock::now();

        if(mStates[m_iStateIndex].bInitial)
        {
            cout << "Enter state " << m_iStateIndex << endl;
            mStates[m_iStateIndex].bInitial = false;
            mStates[m_iStateIndex].m_Start_time = chrono::system_clock::now();
            if( mStates[m_iStateIndex].m_strFirstSentence != "")
            {
                ollama::message response_message("assistant", mStates[m_iStateIndex].m_strFirstSentence);
                mStates[m_iStateIndex].message_history.push_back(response_message);
                RobotCommandProtobuf::RobotCommand command;
                command.set_speak_sentence(mStates[m_iStateIndex].m_strFirstSentence);
                m_pSendMessageManager->AddMessage(command);
                mbTTSComplete = false;
            }
            mbWaitForTTSComplete = mStates[m_iStateIndex].bWaitForTTSComplete; 
        }

        if( mbWaitForTTSComplete)
        {
            if( mbTTSComplete)
            {
                //check the new Whisper result by comparing the time
                if( mpThreadWhisper->b_new_result )
                {
                    if( mpThreadWhisper->result.tStart > mtimestamp_TTSComplete)
                    {
                        cout << "(A)" << mpThreadWhisper->result.sOutput << endl;
                        //generate LLM result;
                        mbWaitForTTSComplete = false;
                        mbWaitForLLMResult = true;
                    }
                }
            }
        }

        if( mbWaitForLLMResult)
        {
            if( mbLLMResult)
            {
                ollama::message response_message("assistant", mStates[m_iStateIndex].m_strFirstSentence);
                mStates[m_iStateIndex].message_history.push_back(response_message);
                RobotCommandProtobuf::RobotCommand command;
                command.set_speak_sentence(mStates[m_iStateIndex].m_strFirstSentence);
                m_pSendMessageManager->AddMessage(command);
                mbTTSComplete = false;
            }
        }

        //Check if the time exceed the state limit
        if(chrono::duration_cast<chrono::milliseconds>(current_time - mStates[m_iStateIndex].m_Start_time) > mStates[m_iStateIndex].m_secDurationLimit)
        {
            m_iStateIndex++;
        }

        //wait for the start command
        if( m_iStateIndex == 0)
        {
            if( mpThreadWhisper->b_new_result)
            {
                if( mpThreadWhisper->result.sOutput.find("開始") != string::npos )
                {
                    m_iStateIndex++;
                }
            }
        }

        this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void ThreadStateControl::NotifyEvent(string description, chrono::time_point<chrono::system_clock> timestamp)
{
    if( description == "onTTSComplete")
    {
        mbTTSComplete = true;
        mtimestamp_TTSComplete = timestamp;
    }
    else if( description == "onLLMResult")
    {
        mbLLMResult = true;
        mtimestamp_LLMResult = timestamp;
    }
    
}
