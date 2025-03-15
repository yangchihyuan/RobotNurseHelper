#include "ProcessAudioThread.hpp"
#include "portaudio.h"

std::mutex gMutex_audio_buffer;
std::queue<short> AudioBuffer;
std::condition_variable cond_var_audio;
PaStream *stream;
bool gbPlayAudio = true;

static int patestCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    std::queue<short> *data = (std::queue<short>*)userData;
    short *out = (short*)outputBuffer;
    unsigned long i;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;

    std::mutex Mutex_enough_buffer;
    std::unique_lock<std::mutex> lock(Mutex_enough_buffer);
    if( data->size() < framesPerBuffer * 2)   //2 is the number of channels
    {
        cond_var_audio.wait(lock);
    }

    if( gbPlayAudio)
    {
        gMutex_audio_buffer.lock();
        for( i=0; i<framesPerBuffer; i++ )
        {
            *out++ = data->front();  /* left */
            data->pop();

            *out++ = data->front();  /* right */
            data->pop();
        }
        gMutex_audio_buffer.unlock();

        return paContinue;
    }
    else
        return paComplete;
}

int PortAudio_initialize()
{
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (512)
    PaStreamParameters outputParameters;
    PaError err;
    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
        fprintf(stderr,"Error: No default output device.\n");
        goto error;
    }
    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = paInt16;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &AudioBuffer );
    if( err != paNoError ) goto error;

//    sprintf( data.message, "No Message" );
//    err = Pa_SetStreamFinishedCallback( stream, &StreamFinished );
//    if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

//    printf("Play for %d seconds.\n", NUM_SECONDS );
//    Pa_Sleep( NUM_SECONDS * 1000 );
    return err;

error:
    Pa_Terminate();
    fprintf( stderr, "An error occurred while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}

//The function is not used.
int PortAudio_stop_and_terminate()
{
    PaError err;
    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;

    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;

    Pa_Terminate();
//    printf("Test finished.\n");

    return err;
    
error:
    Pa_Terminate();
    fprintf( stderr, "An error occurred while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}

ProcessAudioThread::ProcessAudioThread()
{
}

void ProcessAudioThread::run()
{
    PortAudio_initialize();

}
