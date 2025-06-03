#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "silero-vad-onnx.hpp"

//#define __DEBUG_SPEECH_PROB___

#include "wav.h" // For reading WAV files

// VadIterator class: uses ONNX Runtime to detect speech segments.
// Loads the ONNX model.
void VadIterator::init_onnx_model(const std::string& model_path) {
        init_engine_threads(1, 1);
        session = std::make_shared<Ort::Session>(env, model_path.c_str(), session_options);
    }

    // Initializes threading settings.
void VadIterator::init_engine_threads(int inter_threads, int intra_threads) {
        session_options.SetIntraOpNumThreads(intra_threads);
        session_options.SetInterOpNumThreads(inter_threads);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    }

    // Resets internal state (_state, _context, etc.)
void VadIterator::reset_states() {
        std::memset(_state.data(), 0, _state.size() * sizeof(float));
        triggered = false;
        temp_end = 0;
        current_sample = 0;
        prev_end = next_start = 0;
        speeches.clear();
        current_speech = timestamp_t();
        std::fill(_context.begin(), _context.end(), 0.0f);
    }

    // Inference: runs inference on one chunk of input data.
    // data_chunk is expected to have window_size_samples samples.
void VadIterator::predict(const std::vector<float>& data_chunk) {
        // Build new input: first context_samples from _context, followed by the current chunk (window_size_samples).
        std::vector<float> new_data(effective_window_size, 0.0f);
        std::copy(_context.begin(), _context.end(), new_data.begin());
        std::copy(data_chunk.begin(), data_chunk.end(), new_data.begin() + context_samples);
        input = new_data;

        // Create input tensor (input_node_dims[1] is already set to effective_window_size).
        Ort::Value input_ort = Ort::Value::CreateTensor<float>(
            memory_info, input.data(), input.size(), input_node_dims, 2);
        Ort::Value state_ort = Ort::Value::CreateTensor<float>(
            memory_info, _state.data(), _state.size(), state_node_dims, 3);
        Ort::Value sr_ort = Ort::Value::CreateTensor<int64_t>(
            memory_info, sr.data(), sr.size(), sr_node_dims, 1);
        ort_inputs.clear();
        ort_inputs.emplace_back(std::move(input_ort));
        ort_inputs.emplace_back(std::move(state_ort));
        ort_inputs.emplace_back(std::move(sr_ort));

        // Run inference.
        ort_outputs = session->Run(
            Ort::RunOptions{ nullptr },
            input_node_names.data(), ort_inputs.data(), ort_inputs.size(),
            output_node_names.data(), output_node_names.size());

        float speech_prob = ort_outputs[0].GetTensorMutableData<float>()[0];
        float* stateN = ort_outputs[1].GetTensorMutableData<float>();
        std::memcpy(_state.data(), stateN, size_state * sizeof(float));
        current_sample += static_cast<unsigned int>(window_size_samples); // Advance by the original window size.

        // If speech is detected (probability >= threshold)
        if (speech_prob >= threshold) {
#ifdef __DEBUG_SPEECH_PROB___
            float speech = current_sample - window_size_samples;
            printf("{ start: %.3f s (%.3f) %08d}\n", 1.0f * speech / sample_rate, speech_prob, current_sample - window_size_samples);
#endif
            if (temp_end != 0) {
                temp_end = 0;
                if (next_start < prev_end)
                    next_start = current_sample - window_size_samples;
            }
            if (!triggered) {
                triggered = true;
                current_speech.start = current_sample - window_size_samples;
            }
            // Update context: copy the last context_samples from new_data.
            std::copy(new_data.end() - context_samples, new_data.end(), _context.begin());
            return;
        }

        // If the speech segment becomes too long.
        if (triggered && ((current_sample - current_speech.start) > max_speech_samples)) {
            if (prev_end > 0) {
                current_speech.end = prev_end;
                speeches.push_back(current_speech);
                current_speech = timestamp_t();
                if (next_start < prev_end)
                    triggered = false;
                else
                    current_speech.start = next_start;
                prev_end = 0;
                next_start = 0;
                temp_end = 0;
            }
            else {
                current_speech.end = current_sample;
                speeches.push_back(current_speech);
                current_speech = timestamp_t();
                prev_end = 0;
                next_start = 0;
                temp_end = 0;
                triggered = false;
            }
            std::copy(new_data.end() - context_samples, new_data.end(), _context.begin());
            return;
        }

        if ((speech_prob >= (threshold - 0.15)) && (speech_prob < threshold)) {
            // When the speech probability temporarily drops but is still in speech, update context without changing state.
            std::copy(new_data.end() - context_samples, new_data.end(), _context.begin());
            return;
        }

        if (speech_prob < (threshold - 0.15)) {
#ifdef __DEBUG_SPEECH_PROB___
            float speech = current_sample - window_size_samples - speech_pad_samples;
            printf("{ end: %.3f s (%.3f) %08d}\n", 1.0f * speech / sample_rate, speech_prob, current_sample - window_size_samples);
#endif
            if (triggered) {
                if (temp_end == 0)
                    temp_end = current_sample;
                if (current_sample - temp_end > min_silence_samples_at_max_speech)
                    prev_end = temp_end;
                if ((current_sample - temp_end) >= min_silence_samples) {
                    current_speech.end = temp_end;
                    if (current_speech.end - current_speech.start > min_speech_samples) {
                        speeches.push_back(current_speech);
                        current_speech = timestamp_t();
                        prev_end = 0;
                        next_start = 0;
                        temp_end = 0;
                        triggered = false;
                    }
                }
            }
            std::copy(new_data.end() - context_samples, new_data.end(), _context.begin());
            return;
        }
    }

void VadIterator::process(const std::vector<float>& input_wav) {
        reset_states();
        audio_length_samples = static_cast<int>(input_wav.size());
        // Process audio in chunks of window_size_samples (e.g., 512 samples)
        for (size_t j = 0; j < static_cast<size_t>(audio_length_samples); j += static_cast<size_t>(window_size_samples)) {
            if (j + static_cast<size_t>(window_size_samples) > static_cast<size_t>(audio_length_samples))
                break;
            std::vector<float> chunk(&input_wav[j], &input_wav[j] + window_size_samples);
            predict(chunk);
        }
        if (current_speech.start >= 0) {
            current_speech.end = audio_length_samples;
            speeches.push_back(current_speech);
            current_speech = timestamp_t();
            prev_end = 0;
            next_start = 0;
            temp_end = 0;
            triggered = false;
        }
    }

    // Returns the detected speech timestamps.
const std::vector<timestamp_t> VadIterator::get_speech_timestamps() {
        return speeches;
    }

// Public method to reset the internal state.
void VadIterator::reset() {
        reset_states();
    }

VadIterator::VadIterator(const std::string ModelPath,
        int Sample_rate, int windows_frame_size,
        float Threshold, int min_silence_duration_ms,
        int speech_pad_ms, int min_speech_duration_ms,
        float max_speech_duration_s)
        :sample_rate(Sample_rate), threshold(Threshold), speech_pad_samples(speech_pad_ms), prev_end(0)
    {
        sr_per_ms = sample_rate / 1000;  // e.g., 16000 / 1000 = 16
        window_size_samples = windows_frame_size * sr_per_ms; // e.g., 32ms * 16 = 512 samples
        effective_window_size = window_size_samples + context_samples; // e.g., 512 + 64 = 576 samples
        input_node_dims[0] = 1;
        input_node_dims[1] = effective_window_size;
        _state.resize(size_state);
        sr.resize(1);
        sr[0] = sample_rate;
        _context.assign(context_samples, 0.0f);
        min_speech_samples = sr_per_ms * min_speech_duration_ms;
        max_speech_samples = (sample_rate * max_speech_duration_s - window_size_samples - 2 * speech_pad_samples);
        min_silence_samples = sr_per_ms * min_silence_duration_ms;
        min_silence_samples_at_max_speech = sr_per_ms * 98;
        init_onnx_model(ModelPath);
    }