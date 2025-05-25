#include "ThreadWhisper.hpp"

#include "common.h"     //for vad_simple

ThreadWhisper::ThreadWhisper()
{
    n_samples_keep = (int)(1e-3*params.keep_ms*WHISPER_SAMPLE_RATE);
    n_samples_len = (int)(1e-3*params.length_ms*WHISPER_SAMPLE_RATE);
    n_samples_step = (int) (1e-3*params.step_ms*WHISPER_SAMPLE_RATE);

    //used for the stream mode
    if( n_samples_step > 0)
        pcmf32_new.resize(30 * WHISPER_SAMPLE_RATE, 0.0f);     //allocate a buffer for 30 seceonds

    string filepath(getenv("HOME"));
    filepath += "/ZenboNurseHelper_build/silero-vad/src/silero_vad/data/silero_vad.onnx";
    pVad = new VadIterator(filepath);
}

ThreadWhisper::~ThreadWhisper()
{
    if( pOperatorBuffer != NULL)
    {
        pOperatorBuffer->close();
        delete pOperatorBuffer;
        pOperatorBuffer = NULL;
    }

    if( pVad != NULL)
    {
        delete pVad;
        pVad = NULL;
    }
}

void ThreadWhisper::run()
{
    const bool use_vad = n_samples_step <= 0; // sliding window mode uses VAD
    // initial whisper.cpp
    whisper_context_params cparams = whisper_context_default_params();

    //Here, if there is no GPU, whisper.cpp will use CPU.
    cparams.use_gpu = true;
    ctx = whisper_init_from_file_with_params(model_file_path.toUtf8().constData(), cparams);
    if (ctx == NULL) {
        std::cerr << "Failed to initialize whisper context" << std::endl;
        return;
    }
    cparams.flash_attn = params.flash_attn;

    whisper_full_params wparams = whisper_full_default_params(params.beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY);

    wparams.print_progress   = false;
    wparams.print_special    = params.print_special;
    wparams.print_realtime   = false;
    wparams.print_timestamps = !params.no_timestamps;
//    wparams.translate        = params.translate;
    wparams.single_segment   = !use_vad;
    wparams.max_tokens       = params.max_tokens;
    wparams.n_threads        = params.n_threads;
    wparams.beam_search.beam_size = params.beam_size;

    wparams.audio_ctx        = params.audio_ctx;

    wparams.tdrz_enable      = params.tinydiarize; // [TDRZ]

    // disable temperature fallback
    //wparams.temperature_inc  = -1.0f;
    wparams.temperature_inc  = params.no_fallback ? 0.0f : wparams.temperature_inc;

    wparams.prompt_tokens    = params.no_context ? nullptr : prompt_tokens.data();
    wparams.prompt_n_tokens  = params.no_context ? 0       : prompt_tokens.size();

    wparams.translate = false;
    wparams.language = "zh";        // "zh" for Chinese, "en" for English
    wparams.no_speech_thold = 0.6f; // silence threshold for VAD

    int n_iter = 0;

    const int n_step_in_length = !use_vad ? std::max(1, params.length_ms / params.step_ms) : 1; // number of steps to print new line

    while(b_WhileLoop)
    {
        if(bOperatorBuffer_open && pOperatorBuffer->size() > 0)
        {
            //Why is the length /4? Because the input format is float rather than short.
            //But the recording format is short.
            if (whisper_full(ctx, wparams, (float*)pOperatorBuffer->buffer().constData(), pOperatorBuffer->size() / 4) == 0)
            {
                strOperatorSentence = "";
                const int n_segments = whisper_full_n_segments(ctx);
                for (int i = 0; i < n_segments; ++i)
                    strOperatorSentence += whisper_full_get_segment_text(ctx, i);
                b_new_OperatorSentence = true;
            }
        }

        if(bufferlength >= n_samples_step)
        {
            //Get the data
            mtx_whisper_buffer.lock();
            int n_samples_new = bufferlength;

            //The term "take" means to reserve the old data
            int n_samples_take = std::min((int) pcmf32_old.size(), std::max(0, n_samples_keep + n_samples_len - n_samples_new));

            pcmf32.resize(n_samples_new + n_samples_take);

            //move data from pcmf32_old to pcmf32
            for (int i = 0; i < n_samples_take; i++) {
                pcmf32[i] = pcmf32_old[pcmf32_old.size() - n_samples_take + i];
            }

            memcpy(pcmf32.data() + n_samples_take, pcmf32_new.data(), n_samples_new*sizeof(float));

            bufferlength = 0;
            mtx_whisper_buffer.unlock();
        
            //copy, backup
            pcmf32_old = pcmf32;

            //check the volume, if it is too low, skip the inference
//            float volume = ComputeVolume(pcmf32);
//            cout << "Volume: " << volume << std::endl;

            pVad->process(pcmf32);
            if( pVad->get_speech_timestamps().size() > 0)
            {
                // run the inference
                strTemp = "";
                if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
                    fprintf(stderr, "failed to process audio\n");
                    return;
                }

                const int n_segments = whisper_full_n_segments(ctx);
                for (int i = 0; i < n_segments; ++i) {
                    const char * text = whisper_full_get_segment_text(ctx, i);
                        strTemp += text;
                }
                
                std::cout << strTemp << std::endl;
            }
            ++n_iter;
            
            if (n_iter % n_step_in_length == 0) {

                // keep part of the audio for next iteration to try to mitigate word boundary issues
                pcmf32_old = std::vector<float>(pcmf32.end() - n_samples_keep, pcmf32.end());

                // Add tokens of the last full length segment as the prompt
                if (!params.no_context) {
                    prompt_tokens.clear();

                    const int n_segments = whisper_full_n_segments(ctx);
                    for (int i = 0; i < n_segments; ++i) {
                        const int token_count = whisper_full_n_tokens(ctx, i);
                        for (int j = 0; j < token_count; ++j) {
                            prompt_tokens.push_back(whisper_full_get_token_id(ctx, i, j));
                        }
                    }
                }
                strFixed += strTemp;
            }
            strRobotSentence = strFixed + strTemp;
            b_new_RobotSentence = true;

            //check whether there is an end of the voice.
            bool b_vad_detected = ::vad_simple(pcmf32, WHISPER_SAMPLE_RATE, 1000, params.vad_thold, params.freq_thold, false);
            if( b_vad_detected)
            {
                b_RobotSentence_End = true;
                // send the sentence to the LLM, and reset the buffer
            }
        }
        else if( false )  //temporary disable the VAD
        {
            const auto t_now  = std::chrono::high_resolution_clock::now();
            const auto t_diff = std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_last).count();

            if (t_diff < 2000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                continue;
            }

            mtx_whisper_buffer.lock();
            pcmf32_detect.resize(bufferlength);
            memcpy(pcmf32_detect.data(), pcmf32_new.data(), bufferlength*sizeof(float));
            bool b_vad_detected = ::vad_simple(pcmf32_detect, WHISPER_SAMPLE_RATE, 1000, params.vad_thold, params.freq_thold, false);
            if( b_vad_detected)
            {
                // copy the new audio to the end of the buffer
                int copy_length = min(bufferlength, n_samples_len);
                pcmf32.resize(copy_length);
                memcpy(pcmf32.data(), pcmf32_new.data()+bufferlength-copy_length, copy_length*sizeof(float));
                bufferlength = 0;
            }
            mtx_whisper_buffer.unlock();
            if( !b_vad_detected)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            t_last = t_now;

            // run the inference
            strRobotSentence = "";
            if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
                fprintf(stderr, "failed to process audio\n");
                return;
            }

            const int n_segments = whisper_full_n_segments(ctx);
            for (int i = 0; i < n_segments; ++i) {
                const char * text = whisper_full_get_segment_text(ctx, i);
                    strRobotSentence += text;
            }
            b_new_RobotSentence = true;

        }
        else
              std::this_thread::sleep_for(std::chrono::milliseconds(1));



    }
    cout << "Exit thread whisper while loop." << std::endl;
    whisper_free(ctx);

}

void ThreadWhisper::setStartTime()
{
    t_last = std::chrono::high_resolution_clock::now();
    t_start = t_last;
}

void ThreadWhisper::ClearBuffer()
{
    mtx_whisper_buffer.lock();
    pcmf32.clear();
    pcmf32_old.clear();
    pcmf32_new.clear();
    bufferlength = 0;
    strRobotSentence = "";
    strTemp = "";
    strFixed = "";
    mtx_whisper_buffer.unlock();
}

// Compute the volume of the audio signal, too simple to take an affect
float ThreadWhisper::ComputeVolume(const std::vector<float>& pcmf32)
{
    float volume = 0.0f;
    for (size_t i = 0; i < pcmf32.size(); ++i) {
        volume += pcmf32[i] * pcmf32[i];
    }
    volume /= pcmf32.size();
    return volume;
}
