#include "ThreadOllama.hpp"

ThreadOllama::ThreadOllama()
{
    
}

ThreadOllama::~ThreadOllama()
{
    
}

void ThreadOllama::run()
{
    ollama::options options;
//    options["seed"] = 1;      //I cannot fix the seed. Otherwise, the result is always the same.
    //preload the model
    bool model_loaded = ollama::load_model("taide");
    if( !model_loaded )
    {
        std::cerr << "Failed to load LLM model" << std::endl;
        return;
    }

    while(b_WhileLoop)
    {
        std::unique_lock<std::mutex> lk(mtx);
        cond_var_ollama.wait(lk);
        if( strPrompt == "" )
        {
            continue;
        }
        ollama::message message("user", strPrompt );
        ollama::message system_message("system", str_system_message);
        ollama::messages messages = {system_message, message};
        ollama::response response = ollama::chat("taide", messages, options);
        strResponse = response.as_simple_string();
        b_new_LLM_response = true;
    }
    std::cout << "Exit thread Ollama while loop." << std::endl;

}
