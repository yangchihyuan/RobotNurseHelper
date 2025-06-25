#include "ThreadOllama.hpp"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <ctime>
#include <cctype>
extern cv::Mat outFrame; // [MOHAMED]
ThreadOllama::ThreadOllama()
{
    
}

ThreadOllama::~ThreadOllama()
{
    
}
bool done = 0;
int message_count = 0;
string chosen_action = "";
string summary = "";

string ThreadOllama::validate_conversation(ollama::options options, ollama::messages &message_history, string &prompt, bool remove_message, int context_size)
{
    // if (context_size)
    // {
    //     ollama::message check_prompt("system", prompt);
    //     ollama::messages recent_messages = {check_prompt};
    //     auto i = message_history.size();
    //     if (i < 10)
    //     {
    //         return "NONE";
    //     }
    //     for (auto i = message_history.size(); i > message_history.size() - context_size; i--)
    //     {  
    //         recent_messages.push_back(message_history[i]);
    //     }
    //     //ollama::message check_prompt("system", prompt);
    //     //recent_messages.push_back(check_prompt);
    //     ollama::response check_response = ollama::chat(ModelName, recent_messages, options);
    //     return check_response.as_simple_string();
    // }
    // else
    // {
    // }
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
    options["num_ctx"] = 16384; 
    options["think"] = 1;
    //preload the model
    //How can I check if the model is available?
    bool model_loaded = ollama::load_model(ModelName);
    if( !model_loaded )
    {
        std::cerr << "Failed to load LLM model" << std::endl;
        return;
    }

    ollama::message message("user", strPrompt);
    ollama::response context = ollama::chat(ModelName, message, options);
    //vector<ollama::message> message_history = {system_message};
    ollama::message system_message("system", str_system_message_list[0]);
    ollama::messages message_history = {system_message};
    ollama::messages recent_history;
    time_t last_prompt_time = time(0); 
    while(b_WhileLoop)// || 1
    {
        std::unique_lock<std::mutex> lk(mtx);
        cond_var_ollama.wait(lk);
        cout << strPrompt << " " << last_prompt_time - time(0) << "\n";
        string message_sender = "user";
        if( strPrompt == "" && time(0) - last_prompt_time < 30)
        {
            continue;
        }
        else if (strPrompt == "")
        {
            strPrompt = "No response from patient. Continue with what you are saying";
            message_sender = "system";
        }
        last_prompt_time = time(0);
        //cv::imwrite("image_temp.jpg", outFrame);

        //Preprare prompt message with image for LLM
        ollama::image image = ollama::image::from_file("image_temp.jpg");
        ollama::message message_with_image(message_sender, strPrompt, image);
        ollama::message message(message_sender, strPrompt);
        message_history.push_back(message);
        recent_history.push_back(message);
        message_count++;

        //Gather Response from LLM
        ollama::response response = ollama::chat(ModelName, message_history, options);
        ollama::message response_message("Zenbo", response.as_simple_string());
        message_history.push_back(response_message);
        recent_history.push_back(response_message);
        strResponse = response.as_simple_string();
        
        string check_prompt = "Has ALL the patient age, name, pain intensity/level, and symptom information been correctly gathered? This is important to assess whether to continue asking. State yes or no.";
        string check_summary = ThreadOllama::validate_conversation(options, message_history, check_prompt, 1);
        cout << "SUMMARY_ANALYSIS: " << " " << check_summary << "\n"; 

        string action_prompt = R"(Here is a list of available robot actions:

        "EM_Mad02", "BA_Nodhead", "SP_Swim02", "PE_RotateA", "SP_Karate", "RE_Cheer", "SP_Climb",
        "DA_Hit", "TA_DictateR", "SP_Bowling", "SP_Walk", "SA_Find", "BA_TurnHead", "SA_Toothache",
        "SA_Sick", "SA_Shocked", "SP_Dumbbell", "SA_Discover", "RE_Thanks", "PE_Changing",
        "SP_HorizontalBar", "WO_Traffic", "RE_HiR", "RE_HiL", "DA_Brushteeth", "RE_Encourage",
        "RE_Request", "PE_Brewing", "RE_Change", "PE_Phubbing", "RE_Baoquan", "SP_Cheer", "RE_Ask",
        "PE_Triangel", "PE_Sorcery", "PE_Sneak", "PE_Singing", "LE_Yoyo", "SP_Throw", "SP_RaceWalk",
        "PE_ShakeFart", "PE_RotateC", "PE_RotateB", "EM_Blush", "PE_Puff", "PE_PlayCello", "PE_Pikachu"

        Pick the best action for a suitable for the recent conversation context provided. For example, SA_Shocked for shocking responses, RE_Request for requests and so on. If a child patient requests a certain action, choose the most fitting from the above list.

        Reply only with:
        The chosen action)";
        //Pick the best action for a friendly robot talking to a child.
        //It should be fun, safe, and help the child feel happy or engaged.
        //2. A short reason why it’s a good fit)";
        chosen_action = ThreadOllama::validate_conversation(options, recent_history, action_prompt, 1, 10);

        cout << chosen_action << "\n";

        string summary_prompt = R"(Summarize only the important information gathered about patient so far. In this format (only as an example):
        **Patient Summary:**

        -Age: Conflicting reports. Patient states 35, then 30. Clarification needed.
        -Name: Muhammad
        -Main Complaint: Stomach ache.
        -Location: Stomach.
        -Pain Intensity: Additional Concern:** Feels stomach in throat.)";
        
        cout << summary << "\n";
        summary = ThreadOllama::validate_conversation(options, message_history, summary_prompt, 1);
        
        for (int i = 0; i < check_summary.size(); i++)
        {
            check_summary[i] = tolower(check_summary[i]);
        }
        if(check_summary.find("yes") != std::string::npos)
        {
            done = 1;
            //message_history.clear();
            message_history.erase(message_history.begin());
            ollama::message new_system_message("system", str_system_message_list[2]);
            //message_history.push_back(new_system_message);
            message_history.insert(message_history.begin(), new_system_message);
        }
        if (done)
        {
            cout << "DONEDONEDONE\n";
        }
        if (message_count > 1)
        {
            recent_history.pop_back();
            recent_history.pop_back();
        }
        b_new_LLM_response = true;
    }
    std::cout << "Exit thread Ollama while loop." << std::endl;

}
