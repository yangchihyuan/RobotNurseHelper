#include "ThreadOllama.hpp"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <ctime>
#include <cctype>
#include "ThreadProcessImage.hpp"
#include <future>
#include <chrono>
extern cv::Mat outFrame; // [MOHAMED]
ThreadOllama::ThreadOllama()
{
    
}

ThreadOllama::~ThreadOllama()
{
    
}

// string chosen_action = "";
string chosen_face = "";
string check_summary = "";
int dancing_status = 0;
//time_t last_prompt_time;
chrono::time_point<chrono::high_resolution_clock> last_prompt_time;

vector<string> summary(7, "");
vector<string> message_log = {};


string ThreadOllama::validate_conversation(ollama::options options, ollama::messages &message_history, string &prompt, bool remove_message)
{
    ollama::message check_prompt("system", prompt);
    message_history.push_back(check_prompt);
    ollama::response check_response = ollama::chat(ModelName, message_history, options);
    if (remove_message)
        message_history.pop_back();
    
    return check_response.as_simple_string();
}

bool ThreadOllama::stage_check(ollama::options options, ollama::options options_short, ollama::messages &message_history, ollama::messages &recent_history, bool remove_message)
{
    //Summarize patient data
    summary[stage_count] = ThreadOllama::validate_conversation(options, message_history, bio_summary_prompt, 1);
    bool change_stage = 0;
    auto current_time = chrono::high_resolution_clock::now();
    if (stage_count == 0)
    {
        if(chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_count]) > chrono::milliseconds(50000))
        {
            change_stage = 1;
        }
    }
    else if (stage_count == 1)
    {
        ollama::message summary_message("Zenbo", summary[stage_count]);
        cout << "\n\n" << summary[stage_count] << "\n";
        //string check_prompt = "Has ALL the patient age, name, pain intensity/level, and symptom/main complaint information been correctly gathered? This is important to assess whether to continue asking. State yes or no. If no, state what is missing.";
        recent_history.push_back(summary_message);
        check_summary = ThreadOllama::validate_conversation(options, message_history, check_stage_prompt, 1);
        recent_history.pop_back();
        cout << "SUMMARY_ANALYSIS: " << " " << check_summary << "\n"; 
        transform(check_summary.begin(), check_summary.end(), check_summary.begin(), ::tolower);
        
        // string missing_prompt = "What is missing from the patient age, name, pain intensity/level, and symptom/main complaint information. State what is missing.";
        // string missing_summary = ThreadOllama::validate_conversation(options, message_history, missing_prompt, 1);
        // string missing_context = "Check if there is any missing information"; //"\n\nMissing info: " + missing_summary;
        //ollama::message summary_context_message("Zenbo", summary_context);
        //message_history.push_back(summary_context_message);
        if(check_summary.find("yes") != string::npos || chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_count]) > chrono::milliseconds(200000))
        {
            change_stage = 1;
            //message_history.clear(); 
            //message_history.push_back(summary_message); //To start memory from scratch //Alternative would be to clear, then add the summary
            
            cout << "\nDONEDONEDONE\n";
            last_prompt_time -= (maximum_prompt_wait_time[stage_count] * 9)/10;
        }
    }
    else if(stage_count == 2)
    {
        string dance_prompt = "Did the patient pick the Egypt Dance or the Cowboy dance? State 1 for Egypt Dance, 2 for Cowboy dance and 0 for none. Strictly only output 0, 1, or 2.";
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
        std::string elapsed_time_str = std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                current_time - stage_start_time[stage_count]
            ).count()
        ) + " ms";
        summary[stage_count] = "ELAPSED_TIME: " + elapsed_time_str + "\n\n" + summary[stage_count];
    }
    return change_stage;
}

string removeThinkSection(const string& input) {
    string result = input;
    size_t startTag = result.find("<think>");
    size_t endTag = result.find("</think>");

    if (startTag != string::npos && endTag != string::npos && endTag > startTag) {
        endTag += string("</think>").length();
        result.erase(startTag, endTag - startTag);
    }

    return result;
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
    options["num_ctx"] = 131072; //131072; //32768;//16384; 
    //options["think"] = 0;
    
    ollama::options options_short;
    options_short["seed"] = 1;      //I cannot fix the seed. Otherwise, the result is always the same.
    options_short["temperature"] = 0.5;
    options_short["num_ctx"] = 16384; //131072; //32768;//16384; 
    //options_short["think"] = 0;
    //options["max_tokens"] = 50;
    //preload the model
    //How can I check if the model is available?
    bool model_loaded = ollama::load_model(ModelName);
    if( !model_loaded )
    {
        cerr << "Failed to load LLM model" << endl;
        return;
    }
    
    vector<ollama::message> message_buffer;

    stage_count = start_stage_input;
    ollama::message system_message("system", str_system_message_list[start_stage_input]);
    string speak_ch = R"(請用中文回答。)";
    ollama::message speak_ch_system_message("system", speak_ch);
    ollama::messages message_history = {system_message}; //, speak_ch_system_message};
    ollama::messages recent_history;
    last_prompt_time = chrono::high_resolution_clock::now(); // time(0);
    auto last_response_time = chrono::high_resolution_clock::now(); //time(0); 
    unique_lock<mutex> lk(mtx);
    
    stage_start_time[0] = chrono::high_resolution_clock::now(); //time(0);
    int loop_cnt = 0;

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

    // Open the file for reading



    while(b_WhileLoop || 1)
    {
        auto current_time = chrono::high_resolution_clock::now(); //time(0);
        bool change_stage = 0;
        
        cout << "TIMER: " << chrono::duration_cast<chrono::milliseconds>(current_time - last_prompt_time).count() << " STAGE_TIMER: " << chrono::duration_cast<chrono::milliseconds>(current_time - stage_start_time[stage_count]).count() << "\n";
        cond_var_ollama.wait(lk);  // Wait for new input
        
        cout << "\ndancing_status: " << dancing_status << " stage_count: " << stage_count << "\n\n";
        string message_sender = "user";
        if (dancing_status != 0)
        {
            //strResponse = "";
            stage_start_time[stage_count] = chrono::high_resolution_clock::now(); //time(0);
            continue;
        }
        if(change_stage && stage_count == 3)
        {
            strPrompt = dance_complete;
            message_sender = "system";
            cout << "DANCE IS COMPLETED\n";
        }
        
        // ollama::image image = ollama::image::from_file("image_temp.jpg");
        // ollama::message message_with_image("system", "Say the predicted emotion: Surprised, Happy, Sad, Neutral", image);
        // ollama::messages message_image_history = {message_with_image};
        // string temp = "";
        // string emotion = ThreadOllama::validate_conversation(options, message_image_history, temp, 1);
        // cout << "PREDICTED_EMOTION: " << emotion << "\n\n";
        // continue;
        //unique_lock<mutex> lk(mtx);
        //cond_var_ollama.wait(lk);
        current_time = chrono::high_resolution_clock::now(); //time(0);
        if(message_buffer.size() <= 0)
        {
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
                    chosen_action = fut_extra.get();
                    cout << "EXTRA_ACTION: " << chosen_action << "\n";
                }
                continue;
            }
            else if (strPrompt == "")
            {
                strPrompt = no_response;
                message_sender = "system";
            }
        }

        cout << "----------------------------------------\n\n";
        if (strPrompt != "")
        {
            //strPrompt = removeThinkSection(strPrompt);
            last_prompt_time = current_time;
            
            //Preprare prompt message with image for LLM
            //ollama::image image = ollama::image::from_file("image_temp.jpg");
            string input = message_sender + std::string(": ") + strResponse;
            message_log.push_back(input);
            if (message_sender == "user")
            {
                strPrompt = strResponse + "\n\n" + strPrompt; 
                
            }
            //ollama::message message_with_image("system", "Say the predicted emotion: Surprised, Happy, Sad, Neutral", image);
            ollama::message message(message_sender, strPrompt);
            
            //Push new message prompt to message buffer/stack
            //message_history.push_back(message);    
            //recent_history.push_back(message);
            message_buffer.push_back(message);
            cout << "message_buffer SIZE: " << message_buffer.size() << "\n";
        }
        current_time = chrono::high_resolution_clock::now(); //time(0);
        if (chrono::duration_cast<chrono::milliseconds>(current_time - last_prompt_time) < chrono::milliseconds(600))
        {
            continue;
        }
        else
        {
            recent_history.clear();
            last_response_time = current_time;
            while(message_buffer.size() > 0)
            {
                message_history.push_back(message_buffer[0]);
                recent_history.push_back(message_buffer[0]);
                message_buffer.erase(message_buffer.begin());
            }
        }

        cout << "++++++++++++++++++++++++++++++++++++++++\n\n";
        
        //Gather Response from LLM
        ollama::response response = ollama::chat(ModelName, message_history, options);
        strResponse = response.as_simple_string();
        string output = std::string("Zenbo") + std::string(": ") + strResponse;
        message_log.push_back(output);
        
        ollama::message response_message("Zenbo", strResponse);
        
        //message_history.push_back(response_message);
        recent_history.push_back(response_message);
        auto history_copy1 = recent_history;
        auto history_copy2 = recent_history;

        auto bound_fn1 = bind(
            &ThreadOllama::validate_conversation,
            this,
            options_short,
            ref(history_copy1),
            ref(action_prompt),
            true
        );
        
        future<string> fut1;
        if (loop_cnt % 3 || strResponse.size() > 25)
        {
            fut1 = async(launch::async, bound_fn1);
        }
        
        //Prompt for chosen action fitting to recent conversation history
        auto bound_fn2 = bind(
            &ThreadOllama::validate_conversation,
            this,
            options_short,
            ref(history_copy2),
            ref(face_prompt),
            true
        );
        
        future<string> fut2 = async(launch::async, bound_fn2);
        // if (loop_cnt % 3 != 2 || strResponse.size() > 25)
        // {
            //fut2 = async(launch::async, bound_fn2);
            //}
            // //Summarize patient data
            // summary[stage_count] = ThreadOllama::validate_conversation(options, message_history, bio_summary_prompt, 1);
            // ollama::message summary_message("Zenbo", summary[stage_count]);
            // cout << "\n\n" << summary[stage_count] << "\n";
            
        b_new_LLM_response = true;
        if (loop_cnt % 3 || strResponse.size() > 25)
        {
            chosen_action = fut1.get();
        }
        else
        {
            chosen_action = "BA_Nodhead";
        }
        
        //if (loop_cnt % 3 != 2 || strResponse.size() > 25)
        //{
        chosen_face = fut2.get();
        //}
        // else
        // {
        //     chosen_face = (loop_cnt % 10 > 5) ? "TTS_JoyB" : "TTS_PeaceC";
        // }

        cout << chosen_action << "\n";
        cout << chosen_face << "\n";
        
        auto bound_fn4 = bind(
            &ThreadOllama::stage_check,
            this,
            options,
            options_short,
            ref(message_history),
            ref(recent_history),
            true
        );
        future<bool> fut4 = async(launch::async, bound_fn4);

        change_stage = fut4.get();
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
            
            message_history.erase(message_history.begin());
            ollama::message new_system_message("system", str_system_message_list[stage_count]);
            message_history.insert(message_history.begin(), new_system_message);
            
            cout << "\nSTAGE_DONE\n";
        }
        loop_cnt++;
    }
    
    cout << "Exit thread Ollama while loop." << endl;
}
