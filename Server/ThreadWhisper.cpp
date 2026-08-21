#include "ThreadWhisper.hpp"

#include "common.h"     //for vad_simple

ThreadWhisper::ThreadWhisper()
{
    n_samples_keep = (int)(1e-3*params.keep_ms*WHISPER_SAMPLE_RATE);
    n_samples_len = (int)(1e-3*params.length_ms*WHISPER_SAMPLE_RATE);
    n_samples_step = (int) (1e-3*params.step_ms*WHISPER_SAMPLE_RATE);
    n_samples_silent = (int)(0*WHISPER_SAMPLE_RATE);               //disable the waiting time 0.3 second

    //used for the stream mode
    if( n_samples_step > 0)
        pcmf32_new.resize(30 * WHISPER_SAMPLE_RATE, 0.0f);     //allocate a buffer for 30 seceonds

    string filepath(getenv("HOME"));
    filepath += "/RobotNurseHelper_build/silero-vad/src/silero_vad/data/silero_vad.onnx";
    pVad = new VadIterator(filepath);       //Create the silero vad iterator
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
        delete pVad;                //delete the silero vad iterator
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
        throw std::invalid_argument("whiper loading model fails");
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
    wparams.beam_search.patience = 1.2f; //[MOHAMED]

    wparams.audio_ctx        = params.audio_ctx;

    wparams.tdrz_enable      = params.tinydiarize; // [TDRZ]

    // disable temperature fallback
    //wparams.temperature_inc  = -1.0f;
    wparams.temperature_inc = 0.2f;
    wparams.temperature = (strLanguage.c_str() == "zh") ? 0.5f : 0.5f;//[MOHAMED]
    wparams.temperature_inc  = params.no_fallback ? 0.0f : wparams.temperature_inc;

    wparams.prompt_tokens    = params.no_context ? nullptr : prompt_tokens.data();
    wparams.prompt_n_tokens  = params.no_context ? 0       : prompt_tokens.size();

    wparams.translate = false;
    wparams.language = strLanguage.c_str();        // "zh" for Chinese, "en" for English, "ar" for Arabic
    wparams.no_speech_thold = 0.02f; //0.6f; // silence threshold for VAD //[MOHAMED]

    while(b_WhileLoop)
    {
        // << n_iter << "R\n";
        //Now we rarely use the operator buffer.
        //The operator buffer is designed for Fang-yu, but she types fast and rarely use this feature.
        if(bOperatorBuffer_open && pOperatorBuffer->size() > 0)
        {
            //Why is the length /4? Because the input format is float (4 bytes) rather than short.
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

        //If the incoming audio data is greater than 0.5 seconds, we will merge them into our pcmf32 buffer. If the pcmf32 buffer is greater than 5 seconds, we will ignore the first-coming data.
        if(bufferlength >= n_samples_step)  //n_samples_step = 8000; bufferlength is the new coming audio data.
        {
            //n_samples_len = 80000; // 5 seconds, in samples
            mtx_whisper_buffer.lock();
            int n_samples_new = bufferlength;

            if( n_samples_new + pcmf32.size() < n_samples_len)
            {
                int old_size = pcmf32.size();
                pcmf32.resize(pcmf32.size() + n_samples_new);
                for (int i = 0; i < n_samples_new; i++) {
                    pcmf32[old_size + i] = pcmf32_new[i];
                }
                //I cannot use memcpy, which will cause a segmentation fault.
            }
            else if(n_samples_new >= n_samples_len)     //debug case
            {
                pcmf32.resize(n_samples_len);
                for (int i = 0; i < n_samples_len; i++) {
                    pcmf32[i] = pcmf32_new[n_samples_new - n_samples_len + i];
                }
            }
            else
            {
                int old_size = pcmf32.size();
                int n_samples_remove = pcmf32.size() + n_samples_new - n_samples_len;
                pcmf32.resize(pcmf32.size() + n_samples_new);
                for (int i = 0; i < n_samples_new; i++) {
                    pcmf32[old_size + i] = pcmf32_new[i];
                }
                pcmf32.erase(pcmf32.begin(), pcmf32.begin() + n_samples_remove );
            }

            bufferlength = 0;
            mtx_whisper_buffer.unlock();
        

            if( pcmf32.size() < 16000)
            {
                continue;   //not enough data, wait for more data
            }

            pVad->process(pcmf32);              //Use silero vad to check whether there is speech in the audio.
            //There is a clear problem. If the speech has not ended, the vad still returns true.
            //I need to know the end of the speech.
            int last_speech_end = 80000; // 5 seconds, in samples
            int first_speech_start = 0; 
            if( pVad->get_speech_timestamps().size() > 0)
            {
                last_speech_end = pVad->get_speech_timestamps().back().end;
                first_speech_start = pVad->get_speech_timestamps().front().start;
            }
            
            //debug
            //cout << "pcmf32.size() " << pcmf32.size() << endl;
            if( last_speech_end < pcmf32.size() - n_samples_silent)    //Now, n_samples_silent is 0.
            {
                WhisperData tempData;
                tempData.tSpeechEnd = chrono::system_clock::now();
                chrono::milliseconds period((last_speech_end - first_speech_start)*1000/WHISPER_SAMPLE_RATE);
                tempData.tSpeechStart = tempData.tSpeechEnd - period;
                // run the Whisper inference. It takes times
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
               
                //clean the pcmf32 buffer and the pcmf32_old buffer
                pcmf32.clear();
                tempData.sOutput = strTemp;
                tempData.tSTTComplete = chrono::system_clock::now();
                mtx.lock();
                mResult = tempData;
                mtx.unlock();
            }

        }
        else
              std::this_thread::sleep_for(std::chrono::milliseconds(1));



    }
    cout << "Exit thread whisper while loop." << std::endl;
    whisper_free(ctx);

}

WhisperData ThreadWhisper::getLatestResult()
{
    mtx.lock();
    WhisperData sendout = mResult;
    mtx.unlock();
    return sendout;
}

void ThreadWhisper::ClearBuffer()
{
    mtx_whisper_buffer.lock();
    pcmf32.clear();
    pcmf32_new.clear();
    bufferlength = 0;
//    strRobotSentence = "";
    strTemp = "";
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
