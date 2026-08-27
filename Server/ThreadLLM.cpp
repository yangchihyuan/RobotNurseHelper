#include "ThreadLLM.hpp"
#include "utility_string.hpp"

ThreadLLM::ThreadLLM()
{
}

ThreadLLM::~ThreadLLM()
{
}

void ThreadLLM::run()
{
    AnythingLLM anythingLLM("127.0.0.1", 3001, mpsetting->AnythingLLM_API_key);

    mutex mtx;
    unique_lock<mutex> lk(mtx);
    while (b_WhileLoop)
    {
        cond_var_thread_LLM.wait(lk);

        if (mqueue.size() > 0)
        {
            LLMTask task = mqueue.front();
            mqueue.pop();
            strResponse = anythingLLM.ask(mpsetting->AnythingLLM_workspace_slug, task.str_message);

            b_new_LLM_response = true;
            if (task.bNotify)
                mpThreadStateControl->NotifyEvent("onLLMResult", chrono::system_clock::now(), strResponse);
        }
    }
    cout << "Exit thread LLM while loop." << endl;
}

void ThreadLLM::AddQueue(LLMTask task)
{
    mqueue.push(task);
}
