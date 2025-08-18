#include "ThreadOllama.hpp"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <ctime>
#include <cctype>
#include "ThreadProcessImage.hpp"
#include <future>
#include <chrono>
#include <utility_string.hpp>
extern cv::Mat outFrame; // [MOHAMED]       //2025/8/12 the variable is not used.

//stage 0: waiting for start
//stage 1: Ice breaker, ask about child's favourite animal, color... [50s]
//stage 2,3,4,5: Data gathering, ask about child's name, age, symptoms, and pain level. [Till data is gathered ~100s, MAX: 250s]
//stage 6: Dance choice, ask patient for prefered dance type [Till data is gathered ~20s, MAX: 50s] [Dance: ~80s/90s]
//stage 7: Animal guessing game, tell child patient some animal facts and let them guess the animal [60s]
//stage 8: Storytelling, tell the child a story or two [Open, not more than 100s]
//stage 9: Say the ending sentence.

ThreadOllama::ThreadOllama()
{
    mStageDurationLimit[0] = 50s;
    mStageDurationLimit[1] = 50s;
    mStageDurationLimit[2] = 30s;
    mStageDurationLimit[3] = 30s;
    mStageDurationLimit[4] = 30s;
    mStageDurationLimit[5] = 30s;
    mStageDurationLimit[6] = 30s;
    mStageDurationLimit[7] = 60s;         // guess animal
    mStageDurationLimit[8] = 100s;        //story telling
    mStageDurationLimit[9] = 100s;        //story telling
}

ThreadOllama::~ThreadOllama()
{
    
}

//This is a utility function for debugging
void DumpOllamaMessages(ollama::messages messages)
{
    for (const auto message : messages)
    {
        cout << message << endl;
    }
}

string chosen_face = "";            //added by Mohamed. Why did he use the global variable?
string check_summary = "";
int dancing_status = 0;             //added by Mohamed. This variable indicates whether the robot is playing a mbtx file.

//This 9 should not be hard_coded.
vector<string> summary(9, "");      //Mohamed used this vector to store the summary of each stage.
vector<string> message_log = {};

//ollama::chat is only used in the validate_conversation and in the while loop of run().
//The third parameter prompt in fact is the system prompt.
string ThreadOllama::validate_conversation(ollama::options options, ollama::messages &message_history, string &prompt)
{
    ollama::message check_prompt("system", prompt);     //The latest system prompt will replace all exsting system prompts in the message history.
    message_history.push_back(check_prompt);        //push_back the system prompt
    ollama::response check_response = ollama::chat(ModelName, message_history, options);
    message_history.pop_back();
    
    //debug
    //cout << "validate_conversation prompt:" << prompt << " check_response: " << check_response.as_simple_string() << endl;

    return check_response.as_simple_string();
}

//This function checks the current stage and decides whether to change the stage or not.
//There are two options in the parameters.
bool ThreadOllama::stage_check(ollama::options options, ollama::options options_short, ollama::messages &message_history, ollama::messages &recent_history, bool remove_message)
{
    //2025/8/13 This is the only place to use the bio_summary_prompt. However, bio_summary is only collected in stage 1. It is unnecessary to use it in other stages.
//    summary[stage_index] = ThreadOllama::validate_conversation(options, message_history, bio_summary_prompt, 1);
    bool change_stage = 0;
//    auto current_time = chrono::high_resolution_clock::now();
    auto current_time = chrono::system_clock::now();
    //The criteria for changing the stage 0 is time. If the time is more than 50 seconds, change the stage.
    if (stage_index == 0)
    {
        if(chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_index]) > mStageDurationLimit[stage_index])
        {
            change_stage = 1;
        }
    }
    else if (stage_index >= 1 && stage_index <= 4)
    {
//        check_summary = ThreadOllama::validate_conversation(options, message_history, check_stage_prompt);
//        recent_history.pop_back();
//        cout << "SUMMARY_ANALYSIS: " << " " << check_summary << "\n"; 
//        transform(check_summary.begin(), check_summary.end(), check_summary.begin(), ::tolower);  //transform is a standard library function that converts all characters in a string to lowercase.
        
        //Wrong. The check_summary is in Chinese.
        //Problem: The LLM generate wrong answer. Our flow control will be wrong.
//        if(check_summary.find("是") != string::npos || chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_index]) > mStageDurationLimit[stage_index])
//        {
//            change_stage = 1;
            
//            cout << "\nDONEDONEDONE\n";
//        }
        if(chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_index]) > mStageDurationLimit[stage_index])
        {
            change_stage = 1;
        }
    }
    else if(stage_index == 5)
    {
        cout << "stage 5 check string: " << mstrUserInput << endl;
        if(chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_index]) > mStageDurationLimit[stage_index])
        {
            change_stage = 1;
            cout << "Time out, choose Egyptian dance." << endl;
            chosen_dance = 1;
            cout << "CHOSEN_DANCE: " << chosen_dance << "\n"; 
        }
        else        //string comparison
        {
            if (mstrUserInput.find("及") != string::npos || mstrUserInput.find("吉") != string::npos || mstrUserInput.find("極") != string::npos || mstrUserInput.find("級") != string::npos)
            {
                change_stage = 1;
                chosen_dance = 1;
                cout << "CHOSEN_DANCE: " << chosen_dance << "\n"; 
            }
            else if (mstrUserInput.find("牛") != string::npos || mstrUserInput.find("仔") != string::npos )
            {
                change_stage = 1;
                chosen_dance = 2;
                cout << "CHOSEN_DANCE: " << chosen_dance << "\n"; 
            }
        }
    }
/*    else if(stage_index == 6)       //eye health care 
    {
        if(chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_index]) > mStageDurationLimit[stage_index])
        {
            change_stage = 1;
        }
    }
    else if(stage_index == 6)       //eye health care education video
    {
        chosen_dance = 4; 
        if(chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_index]) > mStageDurationLimit[stage_index])
        {
            change_stage = 1;
        }
    }
    */
    else if (stage_index == 6)
    {
        if(chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_index]) > mStageDurationLimit[stage_index])
        {
            change_stage = 1;
        }
    }
    else if (stage_index == 7)
    {
        if(chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_index]) > mStageDurationLimit[stage_index])
        {
            change_stage = 1;
        }
    }
    else if (stage_index == 8)
    {
        change_stage = false;
    }

    return change_stage;
}

void ThreadOllama::run()
{
    ollama::options options;
    options["seed"] = 1;      //I cannot fix the seed. Otherwise, the result is always the same.
    options["temperature"] = 0.3;
    options["num_ctx"] = 131072; //131072; //32768;//16384; number of context tokens, which is the maximum number of tokens the model can handle in a single request

    ollama::options options_short;
    options_short["seed"] = 1;
    options_short["temperature"] = 0.5;
    options_short["num_ctx"] = 16384; //131072; //32768;//16384; //This is the major difference.
    //Why does Mohamed create a shorter options? For speed? For diversity?

    //preload the model
    std::vector<std::string> models = ollama::list_models();
    //check if the ModelName is in the list of models
    if (std::find(models.begin(), models.end(), ModelName) == models.end())
    {
        cerr << "Model " << ModelName << " not found. Please check the model name." << endl;
        return;
    }
    
    // Load the model
    cout << "Loading model: " << ModelName << endl;

    bool model_loaded = ollama::load_model(ModelName);
    if( !model_loaded )
    {
        cerr << "Failed to load LLM model" << endl;
        return;
    }
    
    stage_index = start_stage_input;
    bool bToInitializeStageState = true;

    //2025/8/14: Debug: The original statement is stage_start_time[0]=chrono::high_resolution_clock::now();. It will make a mistake if the start_stage_input is not 0
    stage_start_time[stage_index] = chrono::high_resolution_clock::now();
    ollama::message system_message("system", str_system_message_list[stage_index]);
    ollama::messages message_history = {system_message};
    ollama::messages recent_history; //What is the purpose of this recent_history?
    last_prompt_time = chrono::high_resolution_clock::now(); // time(0);
    auto last_response_time = chrono::high_resolution_clock::now();
    unique_lock<mutex> lk(mtx);
    
    int loop_cnt = 0;

    // Load previous context if available. This is Mohamed's addition.
    // However, this is a debugging feature, not used in the run_server_side_program.
    if (previous_context_path != "")
    {
        std::ifstream file(previous_context_path);
    
        // Check if the file was opened successfully
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << previous_context_path << std::endl;
            return;
        }
    
        // Read the entire file content into a string
        std::string fileContent((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
        
        ollama::message previous_summary_message("user", fileContent);
        message_history.push_back(previous_summary_message);
        // Close the file (optional, as it will be closed when `file` goes out of scope)
        file.close();
        cout << "FILE_CONTEXT ADDED\n\n";
    }

    auto speak_time = chrono::system_clock::now();
    const int multiply_factor = 150;
    chrono::milliseconds ignore_timespan;
    std::chrono::time_point<std::chrono::system_clock> current_time = chrono::system_clock::now();
    while(b_WhileLoop)
    {
        bool change_stage = 0;
        

        //2025/8/12 While the robot is still playing a mbtx file, Hohamed disable all LLM inputs.
        //The dancing_status will be set to 0 in the mainwindow.cpp::timer_event. 
        if (dancing_status != 0)
        {
            stage_start_time[stage_index] = chrono::high_resolution_clock::now();
            //wait for 100 ms to prevent the loop runs too frequently
            this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        current_time = chrono::system_clock::now();
        //2025/8/13 I guess this section of code is to handle the case that there are several meaningless prompts generated by the Whisper model. Today I have solve the problem. Do I still need this section of code?
        /*
        if(message_buffer.size() <= 0)   //message_buffer is a vector. It is impossible to be less than 0.
        {
            cout << "message_buffer.size() <= 0" << endl;
            //The prompt[0] == '!' means that the prompt is wrongly generated by Whisper. Usually it is "!!!!!!!!!!!!!!!!!!!".
            //It seems that Mohamed need to handle the wrong prompts.
            //Here, Mohamed wants to deal with the case that the patient does not respond to the robot's question.
            if( (strPrompt == "" && chrono::duration_cast<chrono::milliseconds>(current_time - last_prompt_time) < maximum_prompt_wait_time[stage_index]) || (strPrompt.size() > 2 && strPrompt[0] == '!'))
            {
                if (chrono::duration_cast<chrono::milliseconds>(current_time - last_prompt_time) > chrono::milliseconds(11000) && chrono::duration_cast<chrono::milliseconds>(current_time - last_prompt_time) < chrono::milliseconds(14000) && recent_history.size() > 1)
                {
                    auto history_copy_extra = recent_history;
                    auto bound_fn_extra = bind(
                        &ThreadOllama::validate_conversation,
                        this,
                        options_short,
                        ref(history_copy_extra),
                        ref(action_prompt)
                    );
                    future<string> fut_extra = async(launch::async, bound_fn_extra);
                    chosen_action = fut_extra.get();    //I don't understand. get() after async() is meaningless.
                    cout << "EXTRA_ACTION: " << chosen_action << "\n";
                }
                //Why does Mohamed use this continue statement here?
                continue;
            }
            else if (strPrompt == "")
            {
                strPrompt = no_response;        //R"(病患沒有回應。請繼續你正在說的內容。)";
            }
        }
        */

        if(bToInitializeStageState)
        {
            bToInitializeStageState = false;
            cout << "Enter stage " << stage_index << endl;
            stage_start_time[stage_index] = chrono::system_clock::now();

            if( stage_index == 0 )
            {
                message_history.clear();
                ollama::message new_system_message("system", str_system_message_list[stage_index]);
                message_history.push_back(new_system_message);
                strResponse = "你好，很高興見到你，你今天過得好嗎？";

                cout << strResponse << " Number of Chinese characteres: " << GetChineseCharacterNumberWithoutPunctuationMarks(strResponse) << endl;

                ollama::message response_message("assistant", strResponse);
                message_history.push_back(response_message);
                b_new_LLM_response = true;
                speak_time = chrono::system_clock::now();
                ignore_timespan = chrono::milliseconds(GetChineseCharacterNumberWithoutPunctuationMarks(strResponse) * multiply_factor);
            }
            else if( stage_index == 1 )
            {
                message_history.clear();
                ollama::message new_system_message("system", str_system_message_list[stage_index]);
                message_history.push_back(new_system_message);
                strResponse = "請問你叫什麼名字？";
                ollama::message response_message("assistant", strResponse);
                message_history.push_back(response_message);
                b_new_LLM_response = true;
                speak_time = chrono::system_clock::now();
                ignore_timespan = chrono::milliseconds(GetChineseCharacterNumberWithoutPunctuationMarks(strResponse) * multiply_factor);
            }
            else if( stage_index == 2 )
            {
                strResponse = "請問你幾歲了？";
                ollama::message response_message("assistant", strResponse);
                message_history.push_back(response_message);
                b_new_LLM_response = true;      //Here is the signal to let timer_event() send a speaking sentence.
                speak_time = chrono::system_clock::now();
                ignore_timespan = chrono::milliseconds(GetChineseCharacterNumberWithoutPunctuationMarks(strResponse) * multiply_factor);
            }
            else if( stage_index == 3 )
            {
                strResponse = "請問你生的是什麼病啊？";
                ollama::message response_message("assistant", strResponse);
                message_history.push_back(response_message);
                b_new_LLM_response = true;
                speak_time = chrono::system_clock::now();
                ignore_timespan = chrono::milliseconds(GetChineseCharacterNumberWithoutPunctuationMarks(strResponse) * multiply_factor);
            }
            else if( stage_index == 4 )
            {
                strResponse = "請你用一到五的等級告訴我你現在的感覺如何？一是很不好，五是很好。";
                ollama::message response_message("assistant", strResponse);
                message_history.push_back(response_message);
                b_new_LLM_response = true;      //Here is the signal to let timer_event() send a speaking sentence.
                speak_time = chrono::system_clock::now();
                ignore_timespan = chrono::milliseconds(GetChineseCharacterNumberWithoutPunctuationMarks(strResponse) * multiply_factor);
            }
            else if( stage_index == 5 )
            {
                message_history.clear();
                ollama::message new_system_message("system", str_system_message_list[stage_index]);
                message_history.push_back(new_system_message);
                strResponse = "我會跳舞喲，我會跳埃及舞和牛仔舞，你想看我跳哪一種舞？";
                ollama::message response_message("assistant", strResponse);
                message_history.push_back(response_message);
                b_new_LLM_response = true;      //Here is the signal to let timer_event() send a speaking sentence.
                speak_time = chrono::system_clock::now();
                ignore_timespan = chrono::milliseconds(GetChineseCharacterNumberWithoutPunctuationMarks(strResponse) * multiply_factor);
            }
            else if( stage_index == 6 )  //animal guesing
            {
                message_history.clear();
                ollama::message new_system_message("system", str_system_message_list[stage_index]);
                message_history.push_back(new_system_message);
                strResponse = "我們來玩一個遊戲吧。我來想一個動物，你來猜，好不好啊？";
                ollama::message response_message("assistant", strResponse);
                message_history.push_back(response_message);
                b_new_LLM_response = true;
                speak_time = chrono::system_clock::now();
                ignore_timespan = chrono::milliseconds(GetChineseCharacterNumberWithoutPunctuationMarks(strResponse) * multiply_factor);
            }
            else if( stage_index == 7 )  //story telling
            {
                message_history.clear();
                ollama::message new_system_message("system", str_system_message_list[stage_index]);
                message_history.push_back(new_system_message);
                strResponse = "我會說故事喲。我可以講各種各樣的故事，像是動物的故事、王子和公主的故事、魔法的故事、星星的故事，你想聽我講什麼樣的故事呢？";
                ollama::message response_message("assistant", strResponse);
                message_history.push_back(response_message);
                b_new_LLM_response = true;
                speak_time = chrono::system_clock::now();
                ignore_timespan = chrono::milliseconds(GetChineseCharacterNumberWithoutPunctuationMarks(strResponse) * multiply_factor);
            }
            else if( stage_index == 8 )  //ending
            {
                message_history.clear();
                ollama::message new_system_message("system", str_system_message_list[stage_index]);
                message_history.push_back(new_system_message);
                strResponse = "今天很高興認識你。跟你聊了很多話，我很開心。希望你的病很快就會好起來，你能高高興興的回家。下次還有機會再和你聊。";
                ollama::message response_message("assistant", strResponse);
                message_history.push_back(response_message);
                b_new_LLM_response = true;
                speak_time = chrono::system_clock::now();
                ignore_timespan = chrono::milliseconds(GetChineseCharacterNumberWithoutPunctuationMarks(strResponse) * multiply_factor);
            }
            else
            {
                cout << "No such stage" << endl;
            }
        }

        //Use strResponse length to estimate the ignore time span
        if(current_time < speak_time + ignore_timespan)
        {
            cout << "*" << flush;
        }
        else if (strPrompt != "")
        {
            cout << "strPrompt " << strPrompt << endl;
            mstrUserInput = strPrompt;
            strPrompt = "";             //clear the strPrompt to prevent infinite loop
            last_prompt_time = current_time;
            
            ollama::message message("user", mstrUserInput);
            
            cout << "recent_history SIZE: " << recent_history.size() << endl;
            DumpOllamaMessages(recent_history);
            cout << "message_history SIZE: " << message_history.size() << endl;
            DumpOllamaMessages(message_history);

            recent_history.clear();
            last_response_time = current_time;          //Is this wrong? last_response_time is never used.
            message_history.push_back(message);
            recent_history.push_back(message);

            cout << "++++++++++++++++Call LLM++++++++++++++++++++++++\n\n";
            
            //Gather Response from LLM
            ollama::response response = ollama::chat(ModelName, message_history, options);
            strResponse = response.as_simple_string();        //The strResponse will be send to the robot to speak out.
            b_new_LLM_response = true;      //Here is the signal to let timer_event() send a speaking sentence.
            speak_time = chrono::high_resolution_clock::now();
            ignore_timespan = chrono::milliseconds(strResponse.length() * multiply_factor);
            cout << "response " << response << endl;
            ollama::message response_message("assistant", response);
            message_history.push_back(response_message);

            
            recent_history.push_back(response_message);     //This is the 2nd place to push the response message to recent_history.
            auto history_copy1 = recent_history;     //This is where recent_history is used, and the purpose is to choose face and motion.
            auto history_copy2 = recent_history;

            auto bound_fn1 = bind(
                &ThreadOllama::validate_conversation,
                this,                                        //For non-static member function, this is required to access the member variables and functions.
                options_short,
                ref(history_copy1),                          //ref() means pass by reference, used for async()
                ref(action_prompt)                          //the action prompt is used to ask the LLM to choose an action based on the recent conversation context.
            );
            
            future<string> fut1;
            if (loop_cnt % 3 || strResponse.size() > 25)   //Only a long response will trigger a motion
            {
                fut1 = async(launch::async, bound_fn1);
            }
            
            //Prompt for chosen a face from recent conversation
            auto bound_fn2 = bind(
                &ThreadOllama::validate_conversation,
                this,
                options_short,
                ref(history_copy2),
                ref(face_prompt)
            );
            
            future<string> fut2 = async(launch::async, bound_fn2);
                
            if (loop_cnt % 3 || strResponse.size() > 25)
            {
                chosen_action = fut1.get();
            }
            else
            {
                chosen_action = "BA_Nodhead";       //I did not see this motion, why?
            }
            
            //Here, the chosen_face is set, and in the MainWindow::timer_event() function, a RobotCommand is sent to the robot.
            chosen_face = fut2.get();       //In fact, the fut2.get() will block the thread until the future is ready.
            

            //Don't use LLM to choose motion and face
            //chosen_action =

            //chosen_face = 
        }

        change_stage = stage_check(options, options_short, message_history, recent_history, true);
        if(change_stage)
        {
            stage_index++;
            bToInitializeStageState = true;
        }

        loop_cnt++;
        
        //wait for 100 ms to prevent the loop runs too frequently
        this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    cout << "Exit thread Ollama while loop." << endl;
}
