#include "ThreadLLM.hpp"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <ctime>
#include <cctype>
#include "ThreadProcessImage.hpp"
#include <future>
#include <chrono>
#include <utility_string.hpp>
#include <cstdlib>
#include <ctime>   // For time()

ThreadLLM::ThreadLLM()
{
    LoadJSONFile(msetting, "json/Setting.json");
}

ThreadLLM::~ThreadLLM()
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

string check_summary = "";

void ThreadLLM::run()
{
    AnythingLLM anythingLLM("127.0.0.1", 3001, msetting.AnythingLLM_API_key);

    srand(time(0));
    ollama::options options;
    //options["seed"] = 1;      
    options["seed"] = rand();
    options["temperature"] = 0.3;
    options["num_ctx"] = 131072; //number of context tokens, which is the maximum number of tokens the model can handle in a single request

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

    //warm up the model
    OllamaTask task_warmup;
    ollama::messages message_history_warmup;
    ollama::message system_message("system", "你是一台名叫凱比的機器人，是基隆長庚醫院的員工，正在為一位白內障病患介紹白內障手術，你很有禮貌，個性熱心活潑，又有點可愛。請遵守以下規則：1.請主動和對方聊天適中。2.不要輸出任何表情符號。3.不要輸出任何括號。");
    message_history_warmup.push_back(system_message);
    ollama::message assistant_message("assistant", "您好，很高興能為您服務。");
    message_history_warmup.push_back(assistant_message);
    ollama::message user_message("user", "你好");
    message_history_warmup.push_back(user_message);
    task_warmup.message_history = message_history_warmup;
    ollama::response response = ollama::chat(ModelName, task_warmup.message_history, options);

    mutex mtx;
    unique_lock<mutex> lk(mtx);
    while(b_WhileLoop)
    {
        cond_var_ollama.wait(lk);

        if(mqueue.size() > 0)
        {
            OllamaTask task = mqueue.front();
            mqueue.pop();
            //replace this part with AnythingLLM API call
            /*
            ollama::response response = ollama::chat(ModelName, task.message_history, options);
            strResponse = response.as_simple_string();        //The strResponse will be send to the robot to speak out.
            */
            strResponse = anythingLLM.ask(msetting.AnythingLLM_workspace_slug, task.message_history.back());

            b_new_LLM_response = true;
            if( task.bNotify)
                mpThreadStateControl->NotifyEvent("onLLMResult", chrono::system_clock::now() ,strResponse);
        }
    }
    cout << "Exit thread Ollama while loop." << endl;
}

void ThreadLLM::AddQueue(OllamaTask task)
{
    mqueue.push(task);
}
