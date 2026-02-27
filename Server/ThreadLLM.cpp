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




void ThreadLLM::run()
{
    AnythingLLM anythingLLM("127.0.0.1", 3001, msetting.AnythingLLM_API_key);

   
    mutex mtx;
    unique_lock<mutex> lk(mtx);
    while(b_WhileLoop)
    {
        cond_var_thread_LLM.wait(lk);

        if(mqueue.size() > 0)
        {
            LLMTask task = mqueue.front();
            mqueue.pop();
            strResponse = anythingLLM.ask(msetting.AnythingLLM_workspace_slug, task.str_message);

            b_new_LLM_response = true;
            if( task.bNotify)
                mpThreadStateControl->NotifyEvent("onLLMResult", chrono::system_clock::now() ,strResponse);
        }
    }
    cout << "Exit thread LLM while loop." << endl;
}

void ThreadLLM::AddQueue(LLMTask task)
{
    mqueue.push(task);
}
