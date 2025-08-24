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

string check_summary = "";

void ThreadOllama::run()
{
    ollama::options options;
    options["seed"] = 1;      //I cannot fix the seed. Otherwise, the result is always the same.
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

    mutex mtx;
    unique_lock<mutex> lk(mtx);
    while(b_WhileLoop)
    {
        cond_var_ollama.wait(lk);

        if(mqueue.size() > 0)
        {
            cout << "(G) process an Ollama task." << endl;
            OllamaTask task = mqueue.front();
            mqueue.pop();
            ollama::response response = ollama::chat(ModelName, task.message_history, options);
            strResponse = response.as_simple_string();        //The strResponse will be send to the robot to speak out.
            b_new_LLM_response = true;
            if( task.bNotify)
                mpThreadStateControl->NotifyEvent("onLLMResult", chrono::system_clock::now() ,strResponse);
        }
    }
    cout << "Exit thread Ollama while loop." << endl;
}

void ThreadOllama::AddQueue(OllamaTask task)
{
    mqueue.push(task);
}
