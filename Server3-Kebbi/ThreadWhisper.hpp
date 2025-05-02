#ifndef ThreadWhipser_hpp
#define ThreadWhipser_hpp

#include <QThread>
#include <QBuffer>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <whisper.h>
#include <queue>


using namespace std;

struct whisper_params {
    int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());    //this does not matter because I need to use GPU to run it.
    int32_t step_ms    = 500;
    int32_t length_ms  = 5000;
    int32_t keep_ms    = 200;
    int32_t max_tokens = 32;
    int32_t audio_ctx  = 0;
    int32_t beam_size  = -1;

    float vad_thold    = 0.6f;  
    float freq_thold   = 100.0f;

    bool translate     = false;
    bool no_fallback   = false;
    bool print_special = false;
    bool no_context    = true;
    bool no_timestamps = false;
    bool tinydiarize   = false;
    bool save_audio    = false; // save audio to wav file
    bool use_gpu       = true;
    bool flash_attn    = false;

    std::string fname_out;
};

class ThreadWhisper: public QThread
{
    Q_OBJECT

public:
    ThreadWhisper();

    bool b_WhileLoop = true;
    condition_variable cond_var_whisper;
    QBuffer OperatorBuffer;             //This buffer is used by operator.
    std::vector<float> pcmf32;//    (n_samples_30s, 0.0f);
    std::vector<float> pcmf32_old;
    std::vector<float> pcmf32_new;
    int bufferlength = 0;

    std::vector<whisper_token> prompt_tokens;
    mutex mtx_whisper_buffer;
    string result;
    bool b_new_result = false;
    QString model_file_path;

protected:
    void run();
    whisper_context* ctx = nullptr;

    whisper_params params;

    int n_samples_step;// = (1e-3*params.step_ms  )*WHISPER_SAMPLE_RATE;
    int n_samples_len;//  = (1e-3*params.length_ms)*WHISPER_SAMPLE_RATE;
    int n_samples_keep;// = (1e-3*params.keep_ms  )*WHISPER_SAMPLE_RATE;
    int n_samples_30s;

private:
    mutex mtx;

    vector<float> QueueToVector(queue<float>& queue);    
};

#endif