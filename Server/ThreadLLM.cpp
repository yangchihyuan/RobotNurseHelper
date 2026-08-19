#include "ThreadLLM.hpp"
#include "utility_string.hpp"
#include <regex>
#include <cctype>
#include <map>

namespace
{
    //Hardcoded nursing-assistant persona, prepended to every AnythingLLM request.
    //AnythingLLM.cpp is off-limits, so this is injected into the "message" text
    //itself rather than as a separate system-prompt field in the request payload.
    const string kPersonaPrompt =
        "You are Zenbo Junior, an empathetic, highly intelligent nursing assistant. "
        "Keep answers brief, warm, and conversational. Always prepend your response "
        "with an emotion tag from this list: [HAPPY], [SAD], [EXPECTING], [QUESTION], "
        "[PLEASED], [ACTIVE].";

    //Not every persona tag has a matching RobotCommand FaceEnum name (Zenbo has no
    //"SAD" or "QUESTION" face). Map onto the closest existing expression so we
    //never hand the robot an sface string it doesn't recognize.
    const std::map<string, string> kEmotionTagToFace = {
        {"HAPPY", "HAPPY"},
        {"SAD", "WORRIED"},
        {"EXPECTING", "EXPECTING"},
        {"QUESTION", "QUESTIONING"},
        {"PLEASED", "PLEASED"},
        {"ACTIVE", "ACTIVE"},
    };

    //Finds a leading "[TAG]" (optionally followed by ":" or "-") at the start of
    //text, removes it from text in place, and returns the mapped face name.
    //Returns "" (and leaves text untouched) if no tag is present.
    string ExtractAndStripEmotionTag(string &text)
    {
        static const std::regex tagRegex(R"(^\s*\[\s*([A-Za-z_]+)\s*\]\s*[:\-]?\s*)");
        std::smatch match;
        if( !std::regex_search(text, match, tagRegex) )
            return "";

        string tag = match[1].str();
        for( char &c : tag )
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        text.erase(0, match.length(0));

        auto it = kEmotionTagToFace.find(tag);
        if( it != kEmotionTagToFace.end() )
            return it->second;

        cout << "ThreadLLM: unrecognized emotion tag [" << tag << "], defaulting to DEFAULT face." << endl;
        return "DEFAULT";
    }
}

ThreadLLM::ThreadLLM()
{
//    LoadJSONFile(msetting, "json/Setting.json");
}

ThreadLLM::~ThreadLLM()
{
}

void ThreadLLM::run()
{
    AnythingLLM anythingLLM("127.0.0.1", 3001, msetting.AnythingLLM_API_key);

    mutex mtx;
    unique_lock<mutex> lk(mtx);
    while(b_WhileLoop)
    {
        cond_var_thread_LLM.wait(lk);

        if(mqueue.size() > 0)
        {
            LLMTask task = mqueue.front();
            mqueue.pop();
            string message_with_persona = kPersonaPrompt + "\n\n" + task.str_message;
            strResponse = anythingLLM.ask(msetting.AnythingLLM_workspace_slug, message_with_persona);

            //Strip the leading emotion tag so the TTS engine never reads the
            //brackets out loud, and route it to the robot's face separately.
            string strFace = ExtractAndStripEmotionTag(strResponse);

            b_new_LLM_response = true;
            if( task.bNotify)
                mpThreadStateControl->NotifyEvent("onLLMResult", chrono::system_clock::now(), strResponse, strFace);
        }
    }
    cout << "Exit thread LLM while loop." << endl;
}

void ThreadLLM::AddQueue(LLMTask task)
{
    mqueue.push(task);
}

void ThreadLLM::SetSettingFile(const QString &filePath)
{
    LoadJSONFile(msetting, filePath.toStdString());
}