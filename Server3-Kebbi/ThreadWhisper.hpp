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
#include <chrono>

using namespace std;

struct whisper_params {
    int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());    //this does not matter because I need to use GPU to run it.
    int32_t step_ms    = 500;
    int32_t length_ms  = 5000;
    int32_t keep_ms    = 1000;  //whisper.cpp requests at least 1 second to recognize the audio
    int32_t max_tokens = 32;
    int32_t audio_ctx  = 0;
    int32_t beam_size  = 8;

//    float vad_thold    = 0.6f;  
    float vad_thold    = 0.8f;  
    float freq_thold   = 100.0f;

    bool translate     = false;
    bool no_fallback   = false;
    bool print_special = false;
    bool no_context    = false;
    bool no_timestamps = false;
    bool tinydiarize   = false;
    bool save_audio    = false; // save audio to wav file
    bool use_gpu       = true;
    bool flash_attn    = true;

    std::string fname_out;
};

class ThreadWhisper: public QThread
{
    Q_OBJECT

public:
    ThreadWhisper();
    ~ThreadWhisper();

    bool b_WhileLoop = true;
    QBuffer *pOperatorBuffer = NULL;             //This buffer is used by operator.
    bool bOperatorBuffer_open = false;
    std::vector<float> pcmf32;
    std::vector<float> pcmf32_old;
    std::vector<float> pcmf32_new;
    int bufferlength = 0;
    std::vector<float> pcmf32_detect;

    std::vector<whisper_token> prompt_tokens;
    mutex mtx_whisper_buffer;
    string strOperatorSentence;
    bool b_new_OperatorSentence = false;
    string strRobotSentence;
    string strTemp;
    string strFixed;
    bool b_new_RobotSentence = false;
    bool b_RobotSentence_End = false;    
    QString model_file_path;

    void setStartTime();
    void ClearBuffer();

protected:
    void run();
    whisper_context* ctx = nullptr;

    whisper_params params;

    int n_samples_step;
    int n_samples_len;
    int n_samples_keep;
    mutex mtx;

    std::chrono::high_resolution_clock::time_point t_last;
    std::chrono::high_resolution_clock::time_point t_start;
};

#endif