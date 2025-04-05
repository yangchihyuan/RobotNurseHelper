#include "ThreadWhisper.hpp"

ThreadWhisper::ThreadWhisper()
{
}

void ThreadWhisper::run()
{
    std::string str_home_path(getenv("HOME"));
//    std::string file_path = str_home_path + "/whisper.cpp/models/ggml-base.bin";
//    std::string file_path = str_home_path + "/whisper.cpp/models/ggml-medium.bin";
    std::string file_path = str_home_path + "/whisper.cpp/models/ggml-large-v3-turbo.bin";
    // initial whisper.cpp
    whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = true;
    //language is not set in the cparams
    ctx = whisper_init_from_file_with_params(file_path.c_str(), cparams);
    if (ctx == NULL) {
        std::cerr << "Failed to initialize whisper context" << std::endl;
        return;
    }

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.translate = false;
    wparams.language = "zh";
    wparams.n_threads = 4;
    wparams.no_context = true;

    while(b_WhileLoop)
    {
        //wait until being notified
        std::unique_lock<std::mutex> lk(mtx);
        cond_var_whisper.wait(lk);

        //whisper.cpp takes too much computational time and pauses the socket read. Thus, I need to create a new thread to run it.
        if(buffer.size() > 0)
        {
            if (whisper_full(ctx, wparams, (float*)buffer.buffer().constData(), buffer.size() / 4) == 0)
            {
                result = "";
                const int n_segments = whisper_full_n_segments(ctx);
                for (int i = 0; i < n_segments; ++i)
                    result += whisper_full_get_segment_text(ctx, i);
                b_new_result = true;
            }
        }
    }
    cout << "Exit thread whisper while loop." << std::endl;
    whisper_free(ctx);

}
