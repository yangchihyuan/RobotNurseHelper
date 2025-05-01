#include "ThreadWhisper.hpp"

ThreadWhisper::ThreadWhisper():
    pcmf32(n_samples_30s, 0.0f),
    pcmf32_new(n_samples_30s, 0.0f)
{
}

void ThreadWhisper::run()
{
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

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.translate = false;
    wparams.language = "zh";        // "zh" for Chinese, "en" for English
    wparams.n_threads = 4;
    wparams.no_context = true;

    auto t_last  = std::chrono::high_resolution_clock::now();
    const auto t_start = t_last;
    int n_iter = 0;
    bool use_vad = false;

    const int n_new_line = !use_vad ? std::max(1, params.length_ms / params.step_ms - 1) : 1; // number of steps to print new line

    while(b_WhileLoop)
    {
        //wait until being notified
        std::unique_lock<std::mutex> lk(mtx);
//        cond_var_whisper.wait(lk);

        if(OperatorBuffer.size() > 0)
        {
            //Why is the length /4? Because the input format is float rather than short.
            //But the recording format is short.
            if (whisper_full(ctx, wparams, (float*)OperatorBuffer.buffer().constData(), OperatorBuffer.size() / 4) == 0)
            {
                result = "";
                const int n_segments = whisper_full_n_segments(ctx);
                for (int i = 0; i < n_segments; ++i)
                    result += whisper_full_get_segment_text(ctx, i);
                b_new_result = true;
            }
        }

        while (true) {
            if ((int) pcmf32_new.size() >= n_samples_step) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }


        //Get the data
        mtx_whisper_buffer.lock();
        int n_samples_new = pcmf32_new.size();

        // take up to params.length_ms audio from previous iteration
        int n_samples_take = std::min((int) pcmf32_old.size(), std::max(0, n_samples_keep + n_samples_len - n_samples_new));

        //printf("processing: take = %d, new = %d, old = %d\n", n_samples_take, n_samples_new, (int) pcmf32_old.size());

        pcmf32.resize(n_samples_new + n_samples_take);

        for (int i = 0; i < n_samples_take; i++) {
            pcmf32[i] = pcmf32_old[pcmf32_old.size() - n_samples_take + i];
        }

        memcpy(pcmf32.data() + n_samples_take, pcmf32_new.data(), n_samples_new*sizeof(float));

        pcmf32_old = pcmf32;
        //clear pcmf32_new
        pcmf32_new.clear();
        mtx_whisper_buffer.unlock();


        // run the inference
        {
            whisper_full_params wparams = whisper_full_default_params(params.beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY);

            wparams.print_progress   = false;
            wparams.print_special    = params.print_special;
            wparams.print_realtime   = false;
            wparams.print_timestamps = !params.no_timestamps;
            wparams.translate        = params.translate;
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

            if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
                fprintf(stderr, "failed to process audio\n");
                return;
            }

            // print result;
            {
                if (!use_vad) {
                    printf("\33[2K\r");

                    // print long empty line to clear the previous line
                    printf("%s", std::string(100, ' ').c_str());

                    printf("\33[2K\r");
                } else {
                    const int64_t t1 = (t_last - t_start).count()/1000000;
                    const int64_t t0 = std::max(0.0, t1 - pcmf32.size()*1000.0/WHISPER_SAMPLE_RATE);

                    printf("\n");
                    printf("### Transcription %d START | t0 = %d ms | t1 = %d ms\n", n_iter, (int) t0, (int) t1);
                    printf("\n");
                }

                const int n_segments = whisper_full_n_segments(ctx);
                for (int i = 0; i < n_segments; ++i) {
                    const char * text = whisper_full_get_segment_text(ctx, i);

                    if (params.no_timestamps) {
                        printf("%s", text);
                        fflush(stdout);

//                        if (params.fname_out.length() > 0) {
//                            fout << text;
//                        }
                    } else {
                        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
                        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

//                        std::string output = "[" + to_timestamp(t0, false) + " --> " + to_timestamp(t1, false) + "]  " + text;
                        std::string output = text;

                        if (whisper_full_get_segment_speaker_turn_next(ctx, i)) {
                            output += " [SPEAKER_TURN]";
                        }

                        output += "\n";

                        printf("%s", output.c_str());
                        fflush(stdout);

//                        if (params.fname_out.length() > 0) {
//                            fout << output;
//                        }
                    }
                }

//                if (params.fname_out.length() > 0) {
//                    fout << std::endl;
//                }

                if (use_vad) {
                    printf("\n");
                    printf("### Transcription %d END\n", n_iter);
                }
            }

            ++n_iter;

            if (!use_vad && (n_iter % n_new_line) == 0) {
                printf("\n");

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
            }
            fflush(stdout);
        }



    }
    cout << "Exit thread whisper while loop." << std::endl;
    whisper_free(ctx);

}
