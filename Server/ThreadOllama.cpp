#include "ThreadOllama.hpp"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <ctime>
#include <cctype>
#include "ThreadProcessImage.hpp"
extern cv::Mat outFrame; // [MOHAMED]
ThreadOllama::ThreadOllama()
{
    
}

ThreadOllama::~ThreadOllama()
{
    
}
bool done = 0;
int stage_count = 0;
string chosen_action = "";
string check_summary = "";
int chosen_dance = 0;

vector<string> summary(3, "");

string ThreadOllama::validate_conversation(ollama::options options, ollama::messages &message_history, string &prompt, bool remove_message, int context_size)
{
    ollama::message check_prompt("system", prompt);
    message_history.push_back(check_prompt);
    ollama::response check_response = ollama::chat(ModelName, message_history, options);
    if (remove_message)
        message_history.pop_back();
    
    return check_response.as_simple_string();
}
void ThreadOllama::run()
{
    ollama::options options;
    options["seed"] = 1;      //I cannot fix the seed. Otherwise, the result is always the same.
    options["temperature"] = 0.1;
    options["num_ctx"] = 131072; //32768;//16384; 
    options["think"] = 1;
    //preload the model
    //How can I check if the model is available?
    bool model_loaded = ollama::load_model(ModelName);
    if( !model_loaded )
    {
        std::cerr << "Failed to load LLM model" << std::endl;
        return;
    }

    vector<ollama::message> message_buffer;

    ollama::message system_message("system", str_system_message_list[0]);
    string speak_ch = R"(請用中文回答。)";
    ollama::message speak_ch_system_message("system", speak_ch);
    ollama::messages message_history = {system_message, speak_ch_system_message};
    ollama::messages recent_history;
    time_t last_prompt_time = time(0), last_response_time = time(0); 
    while(b_WhileLoop || 1)
    {
        cout << "\nis_dancing: " << is_dancing << "\n\n";
        if (is_dancing)
            continue;
        time_t current_time = time(0);
        std::unique_lock<std::mutex> lk(mtx);
        cond_var_ollama.wait(lk);
        cout << "TIMER: " << strPrompt << " " << last_prompt_time - current_time << "\n";
        string message_sender = "user";
        if(message_buffer.size() <= 0)
        {
            if( strPrompt == "" && current_time - last_prompt_time < maximum_prompt_wait_time)
            {
                continue;
            }
            else if (strPrompt == "")
            {
                strPrompt = no_response;
                message_sender = "system";
            }
        }

        if (strPrompt != "")
        {
            last_prompt_time = current_time;
            //cv::imwrite("image_temp.jpg", outFrame);
            
            //Preprare prompt message with image for LLM
            ollama::image image = ollama::image::from_file("image_temp.jpg");
            if (message_sender == "user")
            {
                strPrompt = strResponse + "\n\n" + strPrompt; 

            }
            ollama::message message_with_image(message_sender, strPrompt, image);
            ollama::message message(message_sender, strPrompt);
            
            //Push new message prompt to message buffer/stack
            //message_history.push_back(message);    
            //recent_history.push_back(message);
            message_buffer.push_back(message);
            cout << "message_buffer SIZE: " << message_buffer.size() << "\n";
        }

        if (current_time - last_prompt_time < 2)
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
        
        
        //Gather Response from LLM
        ollama::response response = ollama::chat(ModelName, message_history, options);
        ollama::message response_message("Zenbo", response.as_simple_string());
        strResponse = response.as_simple_string();
        
        //message_history.push_back(response_message);
        recent_history.push_back(response_message);

        //Prompt for chosen action fitting to recent conversation history
        chosen_action = ThreadOllama::validate_conversation(options, recent_history, action_prompt, 1);
        cout << chosen_action << "\n";
        
        b_new_LLM_response = true;
        
        //Summarize patient data
        summary[(done) ? 2 : 0] = ThreadOllama::validate_conversation(options, message_history, bio_summary_prompt, 1);
        ollama::message summary_message("Zenbo", summary[(done) ? 2 : 0]);
        cout << "\n\n" << summary[(done) ? 2 : 0] << "\n";
        
        //Validate stages
        if (stage_count == 0)
        {
            //string check_prompt = "Has ALL the patient age, name, pain intensity/level, and symptom/main complaint information been correctly gathered? This is important to assess whether to continue asking. State yes or no. If no, state what is missing.";
            message_history.push_back(summary_message);
            check_summary = ThreadOllama::validate_conversation(options, message_history, check_stage_prompt, 1);
            message_history.pop_back();
            cout << "SUMMARY_ANALYSIS: " << " " << check_summary << "\n"; 
            for (int i = 0; i < check_summary.size(); i++)
            {
                check_summary[i] = tolower(check_summary[i]);
            }
            
            // string missing_prompt = "What is missing from the patient age, name, pain intensity/level, and symptom/main complaint information. State what is missing.";
            // string missing_summary = ThreadOllama::validate_conversation(options, message_history, missing_prompt, 1);
            // string missing_context = "Check if there is any missing information"; //"\n\nMissing info: " + missing_summary;
            //ollama::message summary_context_message("Zenbo", summary_context);
            //message_history.push_back(summary_context_message);
        }
        else if(stage_count == 1)
        {
            string dance_prompt = "Did the patient pick the Egypt Dance or the Cowboy dance? State 1 for Egypt Dance, 2 for Cowboy dance and 0 for none";
            string dance_response = ThreadOllama::validate_conversation(options, recent_history, dance_prompt, 1);
            if (dance_response.find("1") != std::string::npos)
            {
                chosen_dance = 1;
                stage_count++;
            }
            else if (dance_response.find("2") != std::string::npos)
            {
                chosen_dance = 2;
                stage_count++;
            }
            cout << "CHOSEN_DANCE: " << " " << chosen_dance << "\n"; 
        }
        if(stage_count == 0 && check_summary.find("yes") != std::string::npos)
        {
            done = 1;
            stage_count++;
            //message_history.clear(); //To start memory from scratch //Alternative would be to clear, then add the summary
            message_history.erase(message_history.begin());
            ollama::message new_system_message("system", str_system_message_list[stage_count]);
            message_history.insert(message_history.begin(), new_system_message);
            cout << "\nDONEDONEDONE\n";
        }
        else if (stage_count == 0)
        {
            message_history.erase(message_history.begin());
            string missing_context = "Check if there is any missing information"; //"\n\nMissing info: " + missing_summary;
            missing_context = str_system_message_list[stage_count] + missing_context;
            ollama::message new_system_message("system", missing_context);
            message_history.insert(message_history.begin(), new_system_message);
        }
    }
    std::cout << "Exit thread Ollama while loop." << std::endl;

}
