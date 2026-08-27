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
#include "silero-vad-onnx.hpp"
#include "Logger.hpp"
#include "utility_time.hpp"
#include "Setting.hpp"

using namespace std;

struct whisper_params
{
    int32_t n_threads = std::min(4, (int32_t)std::thread::hardware_concurrency()); // this does not matter because I need to use GPU to run it.
    int32_t step_ms = 500;
    int32_t length_ms = 5000;
    int32_t keep_ms = 4500;
    int32_t max_tokens = 32;
    int32_t audio_ctx = 0;
    int32_t beam_size = 6;

    //    float vad_thold    = 0.6f;
    float vad_thold = 1.0f;    // 0.9f;  //0.8f
    float freq_thold = 150.0f; // 80.0f; //100.0f;

    bool translate = false;
    bool no_fallback = false;
    bool print_special = false;
    bool no_context = false;
    bool no_timestamps = false;
    bool tinydiarize = false;
    bool save_audio = false; // save audio to wav file
    bool use_gpu = true;
    bool flash_attn = true;

    std::string fname_out;
};

struct WhisperData
{
    string sOutput;
    chrono::time_point<chrono::system_clock> tSpeechStart;
    chrono::time_point<chrono::system_clock> tSpeechEnd;
    chrono::time_point<chrono::system_clock> tSTTComplete;
};

class ThreadWhisper : public QThread
{
    Q_OBJECT

public:
    ThreadWhisper();
    ~ThreadWhisper();

    bool b_WhileLoop = true;
    QBuffer *pOperatorBuffer = nullptr; // This buffer is used by operator.
    bool bOperatorBuffer_open = false;
    std::vector<float> pcmf32;
    std::vector<float> pcmf32_new;
    int bufferlength = 0; // When new audio data comes, the bufferlength will be increased, and the data will be copied to pcmf32_new.
    std::vector<float> pcmf32_detect;

    std::vector<whisper_token> prompt_tokens;
    mutex mtx_whisper_buffer;
    string strOperatorSentence;
    bool b_new_OperatorSentence = false;
    string strTemp;

    QString model_file_path;
    string strLanguage = "zh"; // default language is Chinese

    void ClearBuffer();
    void SkipCurrentSpeech();

    // Echo-cancellation for TTS self-hearing (ZenboJrII).
    // onTTSComplete does not line up exactly with the true end of the audible speech,
    // so the microphone can still pick up the tail of the robot's own voice for a
    // short while after the signal arrives. StartEchoIgnoreWindow() discards audio
    // arriving after the signal until its volume decays back down to the ambient
    // noise floor (tracked continuously at runtime, so there is no fixed millisecond
    // value to hand-tune), bounded by iEchoIgnoreMinMs/iEchoIgnoreMaxMs as safety rails.
    void StartEchoIgnoreWindow();

    VadIterator *pVad = nullptr; // This is the Silero VAD iterator.

    WhisperData getLatestResult();
    Logger *mpLogger = nullptr; // This is the logger pointer. It is used to log the whisper result.
    Setting *mpsetting = nullptr;

protected:
    void run();
    whisper_context *ctx = nullptr;

    whisper_params params;

    int n_samples_step;
    int n_samples_len;
    int n_samples_keep;
    int n_samples_silent;
    mutex mtx;

    float ComputeVolume(const std::vector<float> &pcmf32);
    float ComputeVolume(const float *data, size_t n);
    WhisperData mResult;
    bool bSkipCurrentSpeech = false; // This variable is used to skip the current speech.

    // Set by StartEchoIgnoreWindow() when RobotModel == "ZenboJrII" and onTTSComplete fires.
    // While true, newly arrived audio is dropped instead of being merged into pcmf32,
    // until its volume decays to the ambient floor (or the safety ceiling is hit).
    bool bIgnoreWhisperInput = false;
    chrono::steady_clock::time_point tEchoIgnoreMinUntil; // hard floor: never release before this
    chrono::steady_clock::time_point tEchoIgnoreMaxUntil; // safety ceiling: always release by this

    // Ambient microphone noise floor, self-calibrating at runtime via a fast-attack/
    // slow-release tracker (see run()): it snaps down immediately whenever a quieter
    // chunk arrives, but only creeps up slowly during loud audio (echo or speech), so
    // it converges on the true room noise floor without a separate calibration step.
    bool bAmbientCalibrated = false;
    float mAmbientNoiseFloor = 0.0f;
};

#endif