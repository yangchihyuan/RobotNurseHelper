#include "ThreadOllama.hpp"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <ctime>
#include <cctype>
#include "ThreadProcessImage.hpp"
#include <future>
#include <chrono>
extern cv::Mat outFrame; // [MOHAMED]       //2025/8/12 the variable is not used.
ThreadOllama::ThreadOllama()
{
    
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

vector<string> summary(7, "");      //Mohamed used this vector to store the summary of each stage.
vector<string> message_log = {};

//ollama::chat is only used in the validate_conversation and in the while loop of run().
//The third parameter prompt in fact is the system prompt.
string ThreadOllama::validate_conversation(ollama::options options, ollama::messages &message_history, string &prompt, bool remove_message)
{
    ollama::message check_prompt("system", prompt);     //The latest system prompt will replace all exsting system prompts in the message history.
    message_history.push_back(check_prompt);        //push_back the system prompt
    ollama::response check_response = ollama::chat(ModelName, message_history, options);
    if (remove_message)
        message_history.pop_back();
    
    return check_response.as_simple_string();
}

//This function checks the current stage and decides whether to change the stage or not.
//There are two options in the parameters.
bool ThreadOllama::stage_check(ollama::options options, ollama::options options_short, ollama::messages &message_history, ollama::messages &recent_history, bool remove_message)
{
    //2025/8/13 This is the only place to use the bio_summary_prompt. However, bio_summary is only collected in stage 1. It is unnecessary to use it in other stages.
//    summary[stage_count] = ThreadOllama::validate_conversation(options, message_history, bio_summary_prompt, 1);
    bool change_stage = 0;
    auto current_time = chrono::high_resolution_clock::now();
    //The criteria for changing the stage 0 is time. If the time is more than 50 seconds, change the stage.
    if (stage_count == 0)
    {
        if(chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_count]) > chrono::milliseconds(50000))
        {
            change_stage = 1;
        }
    }
    // Stage 1: Try 200 seconds to gather a patient's basic information.
    else if (stage_count == 1)
    {
        //cout << "summary[stage_count] " << summary[stage_count] << endl;      //2025/8/12 Meaningless.
        //string check_prompt = "Has ALL the patient age, name, pain intensity/level, and symptom/main complaint information been correctly gathered? This is important to assess whether to continue asking. State yes or no. If no, state what is missing.";
        check_summary = ThreadOllama::validate_conversation(options, message_history, check_stage_prompt, 1);
        //check_stage_prompt = "是否已完整收集病患的年齡、姓名、疼痛強度（或等級）以及症狀／主要主訴資訊？這對於判斷是否繼續提問非常重要。請回答是或否。如果是否，請說明缺失的資訊。"
        recent_history.pop_back();
        cout << "SUMMARY_ANALYSIS: " << " " << check_summary << "\n"; 
        transform(check_summary.begin(), check_summary.end(), check_summary.begin(), ::tolower);  //transform is a standard library function that converts all characters in a string to lowercase.
        
        if(check_summary.find("yes") != string::npos || chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_count]) > chrono::milliseconds(200000))
        {
            change_stage = 1;
            
            cout << "\nDONEDONEDONE\n";
            //2025/8/12 Mohamed use the 9/10 to adjust the wait time. It is a parameter tuning.
            last_prompt_time -= (maximum_prompt_wait_time[stage_count] * 9)/10;
        }
    }
    else if(stage_count == 2)
    {
        //2025/8/12 Something wrong here. The Whisper may fail to recognize Egyptian dance so the LLM will not be able to choose the dance.
        //In addition, Mohamed used English to ask the LLM to choose the dance. It is not suitable for a Chinese child. 
        //string dance_prompt = "Did the patient pick the Egypt Dance or the Cowboy dance? State 1 for Egypt Dance, 2 for Cowboy dance and 0 for none. Strictly only output 0, 1, or 2.";
        string dance_prompt = "使用者的回覆中有没有提到埃及或是吉？有的話請回覆1。有沒有提到牛仔，有的話請回覆2。如果都沒有的話，請回覆0。";
        string dance_response = ThreadOllama::validate_conversation(options_short, recent_history, dance_prompt, 1);
        cout << "CHOSEN_DANCE_PROMPT: " << dance_response << "\n";
        if (dance_response.find("1") != string::npos)
        {
            change_stage = 1;
            chosen_dance = 1;
        }
        else if (dance_response.find("2") != string::npos || chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_count]) > chrono::milliseconds(50000))
        {
            change_stage = 1;
            chosen_dance = 2;
        }
        cout << "CHOSEN_DANCE: " << chosen_dance << "\n"; 
    }
    else if(stage_count == 3)
    {
        if(chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_count]) > chrono::milliseconds(80000))
        {
            change_stage = 1;
            chosen_dance = 4; 
        }
    }
    else if(stage_count == 4)
    {
        if(chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_count]) > chrono::milliseconds(60000))
        {
        }
        change_stage = 1;
    }
    else if (stage_count == 5)
    {
        if(chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_count]) > chrono::milliseconds(60000))
        {
            change_stage = 1;
        }
    }
    else if (stage_count == 6)
    {
        //2025/8/12 It appears that Mohamed forget to add a time limit for stage 6. So I need it here.
        if(chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_count]) > chrono::milliseconds(100000))
        {
            change_stage = 1;
        }
        std::string elapsed_time_str = std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                current_time - stage_start_time[stage_count]
            ).count()
        ) + " ms";
        summary[stage_count] = "ELAPSED_TIME: " + elapsed_time_str + "\n\n" + summary[stage_count];
    }
    return change_stage;
}

//stage 0: Ice breaker, ask about child's favourite animal, color... [50s]
//stage 1: Data gathering, ask about child's name, age, symptoms, and pain level. [Till data is gathered ~100s, MAX: 250s]
//stage 2: Dance choice, ask patient for prefered dance type [Till data is gathered ~20s, MAX: 50s] [Dance: ~80s/90s]
//stage 3: Riddle games, tell child patient some riddles and assess their answers [80s]
//stage 4: Educational video, do nothing [60s]
//stage 5: Animal guessing game, tell child patient some animal facts and let them guess the animal [60s]
//stage 6: Storytelling, tell the child a story or two [Open, not more than 100s]

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
    
    ollama::messages message_buffer;                 //message_buffer looks like the history of strPrompt.

    stage_count = start_stage_input;    //2025/8/6 stage_count is the current stage of the conversation.
    ollama::message system_message("system", str_system_message_list[start_stage_input]);
    ollama::messages message_history = {system_message};     //Here is the declaration of message_history.
    ollama::messages recent_history; //What is the purpose of this recent_history?
    last_prompt_time = chrono::high_resolution_clock::now(); // time(0);
    auto last_response_time = chrono::high_resolution_clock::now(); //time(0); 
    unique_lock<mutex> lk(mtx);
    
    stage_start_time[0] = chrono::high_resolution_clock::now(); //time(0);
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

    while(b_WhileLoop)
    {
        auto current_time = chrono::high_resolution_clock::now(); //time(0);
        bool change_stage = 0;
        
        cout << "TIMER: " << chrono::duration_cast<chrono::milliseconds>(current_time - last_prompt_time).count() << " STAGE_TIMER: " << chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_count]).count() << "\n";
        cond_var_ollama.wait(lk);  // Wait for new input
        
        cout << "\ndancing_status: " << dancing_status << " stage_count: " << stage_count << "\n\n";
        string message_sender = "user";

        //2025/8/12 While the robot is still playing a mbtx file, Hohamed disable all LLM inputs.
        if (dancing_status != 0)
        {
            stage_start_time[stage_count] = chrono::high_resolution_clock::now(); //time(0);
            continue;
        }
        if(change_stage && stage_count == 3)
        {
            strPrompt = dance_complete;    //R"(病人選擇的舞蹈已經完成)";
            message_sender = "system";
            cout << "DANCE IS COMPLETED\n";
        }
        
        current_time = chrono::high_resolution_clock::now(); //time(0);
        if(message_buffer.size() <= 0)   //message_buffer is a vector. It is impossible to be less than 0.
        {
            //The prompt[0] == '!' means that the prompt is wrongly generated by Whisper. Usually it is "!!!!!!!!!!!!!!!!!!!".
            //It seems that Mohamed need to handle the wrong prompts.
            //Here, Mohamed wants to deal with the case that the patient does not respond to the robot's question.
            if( (strPrompt == "" && chrono::duration_cast<chrono::milliseconds>(current_time - last_prompt_time) < maximum_prompt_wait_time[stage_count]) || (strPrompt.size() > 2 && strPrompt[0] == '!'))
            {
                if (chrono::duration_cast<chrono::milliseconds>(current_time - last_prompt_time) > chrono::milliseconds(11000) && chrono::duration_cast<chrono::milliseconds>(current_time - last_prompt_time) < chrono::milliseconds(14000) && recent_history.size() > 1)
                {
                    auto history_copy_extra = recent_history;
                    auto bound_fn_extra = bind(
                        &ThreadOllama::validate_conversation,
                        this,
                        options_short,
                        ref(history_copy_extra),
                        ref(action_prompt),
                        true
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
                message_sender = "system";
            }
        }

        //Purpose of this section is to prepare the prompt for LLM.
        cout << "---------------Prepare Prompt-------------------------\n\n";
        if (strPrompt != "")
        {
            last_prompt_time = current_time;
            
            ollama::message message("user", strPrompt);
            
            //Push new message prompt to message buffer
            message_buffer.push_back(message);                 //This is the only place message_buffer is pushed.
            cout << "message_buffer SIZE: " << message_buffer.size() << endl;
            DumpOllamaMessages(message_buffer); 
            cout << "recent_history SIZE: " << recent_history.size() << endl;
            DumpOllamaMessages(recent_history);
            cout << "message_history SIZE: " << message_history.size() << endl;
            DumpOllamaMessages(message_history);
        }

            recent_history.clear();
            last_response_time = current_time;          //Is this wrong? last_response_time is never used.
            while(message_buffer.size() > 0)            //This while loop will move all message_buffer to message_history
            {
                message_history.push_back(message_buffer[0]);
                recent_history.push_back(message_buffer[0]);
                message_buffer.erase(message_buffer.begin());
            }

        cout << "++++++++++++++++Call LLM++++++++++++++++++++++++\n\n";
        
        //Gather Response from LLM
        ollama::response response = ollama::chat(ModelName, message_history, options);
        strResponse = response.as_simple_string();        //The strResponse will be send to the robot to speak out.
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
            ref(action_prompt),                          //the action prompt is used to ask the LLM to choose an action based on the recent conversation context.
            true
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
            ref(face_prompt),
            true
        );
        
        future<string> fut2 = async(launch::async, bound_fn2);
            
        b_new_LLM_response = true;
        if (loop_cnt % 3 || strResponse.size() > 25)
        {
            chosen_action = fut1.get();
        }
        else
        {
            chosen_action = "BA_Nodhead";
        }
        
        //Here, the chosen_face is set, and in the MainWindow::timer_event() function, a RobotCommand is sent to the robot.
        chosen_face = fut2.get();       //In fact, the fut2.get() will block the thread until the future is ready.

        change_stage = stage_check(options, options_short, message_history, recent_history, true);
        if(change_stage)
        {
            std::string elapsed_time_str = std::to_string(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    current_time - stage_start_time[stage_count]
                ).count()
            ) + " ms";

            summary[stage_count] = "ELAPSED_TIME: " + elapsed_time_str + "\n\n" + summary[stage_count];
            stage_count++;
            stage_start_time[stage_count] = chrono::high_resolution_clock::now(); //time(0);
            
            ollama::message new_system_message("system", str_system_message_list[stage_count]);
            message_history.push_back(new_system_message);
            
            cout << "\nSTAGE_DONE\n";
        }
        loop_cnt++;
    }
    
    cout << "Exit thread Ollama while loop." << endl;
}
