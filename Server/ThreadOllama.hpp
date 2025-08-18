#ifndef Ollama_hpp
#define Ollama_hpp

#include <QThread>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include "ollama.hpp"

using namespace std;

extern int dancing_status;
extern vector<string> summary;
extern vector<string> message_log;      //created by Mohamed, for debugging purpose.
extern string chosen_face;

class ThreadOllama: public QThread
{
    Q_OBJECT

public:
    ThreadOllama();
    ~ThreadOllama();

    int stage_index = 0;
    int mNumberOfStages = 9;
    chrono::time_point<std::chrono::high_resolution_clock> stage_start_time[9];


    bool b_WhileLoop = true;
    bool b_new_LLM_response = false;
    
//    condition_variable cond_var_ollama;
    string strPrompt;                       //The strPrompt is the user's input sentence, genrated by Whisper.
    string strResponse;
    string str_system_message;               
    string str_system_message_list[9];
    chrono::seconds mStageDurationLimit[9];
    
    string bio_summary_prompt;              
    string check_stage_prompt;              //added by Mohamed
    string no_response, dance_complete;     //added by Mohamed
    string ModelName = "gemma3:12b";
    string previous_context_path;           //added by Mohamed
    int start_stage_input;

    string action_prompt = R"(Here is a list of available robot actions:
    
    "EM_Mad02", "BA_Nodhead", "SP_Swim02", "PE_RotateA", "SP_Karate", "RE_Cheer", "SP_Climb",
    "DA_Hit", "TA_DictateR", "SP_Bowling", "SP_Walk", "SA_Find", "BA_TurnHead", "SA_Toothache",
    "SA_Sick", "SA_Shocked", "SA_Discover", "RE_Thanks", "PE_Changing", "RE_HiR", "RE_HiL", 
    "DA_Brushteeth", "RE_Encourage", "RE_Request", "PE_Brewing", "RE_Change", "PE_Phubbing", "SP_Cheer", 
    "RE_Ask", "PE_Sorcery", "PE_Sneak", "PE_Singing", "SP_Throw", "SP_RaceWalk", "PE_RotateC", 
    "PE_RotateB", "EM_Blush", "PE_Puff", "PE_PlayCello", "PE_Pikachu"
    
    Pick the best engaging and friendly action for a suitable for the recent conversation context provided. For example, SA_Shocked for shocking responses, RE_Request for requests, RE_Encourage to encourage if child is replying with short responses/shy, RE_Cheer if happy, and so on. If a child patient requests a certain action, choose the most fitting from the above list. If no option seems too fitting for the context, be more creative in choosing from the list.
    
    Reply only with:
    The chosen action)";

    string face_prompt = R"(Here is a list of available robot face animations:

    "TTS_AngerA", "TTS_AngerB", "TTS_Contempt", "TTS_Disgust", "TTS_Fear",
    "TTS_JoyA", "TTS_JoyB", "TTS_JoyC",
    "TTS_PeaceA", "TTS_PeaceB", "TTS_PeaceC",
    "TTS_SadnessA", "TTS_SadnessB", "TTS_Surprise"

    Pick the best face animation suitable for the recent conversation context provided for a child, try to be more positive. For example, TTS_JoyC for happy context, TTS_Surprise for surprising responses, TTS_SadnessA for sad moments, and so on. If no option seems perfect for the context, choose the most expressive or appropriate one from the list creatively.
    If no animation is needed, output no.
    Reply only with:
    The chosen face animation)";

    string chosen_action = "";
    int chosen_dance = 0;

protected:
    void run();
    string validate_conversation(ollama::options options, ollama::messages &message_history, string &prompt);
    bool stage_check(ollama::options options, ollama::options options_short, ollama::messages &message_history, ollama::messages &recent_history, bool remove_message);
    mutex mtx;
    chrono::time_point<chrono::high_resolution_clock> last_prompt_time;
    string mstrUserInput;
};

#endif
