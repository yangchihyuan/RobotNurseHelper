#include <QObject>
#include "ollama.hpp"

#include <memory>
#include <nlohmann/json.hpp>
#include <iostream>

#include <vector>
#include <string>
#include "AnythingLLM.hpp"

std::string ManualMemoryNurse::askWithManualHistory(std::string new_question) {
    std::string full_prompt = "";
    
    // 1. 疊加歷史 (只取最近 3 輪，避免 Token 爆炸)
    int start = (history.size() > 3) ? history.size() - 3 : 0;
    for (int i = start; i < history.size(); ++i) {
        full_prompt += "User: " + history[i].first + "\n";
        full_prompt += "Assistant: " + history[i].second + "\n";
    }
    
    // 2. 加入當前問題
    full_prompt += "User: " + new_question;

    // 3. 呼叫 API (注意：mode 要用 query，避免伺服器端又疊加一次)
    nlohmann::json body = {
        {"message", full_prompt},
        {"mode", "query"} 
    };

    // ... 發送 POST 請求並取得 response_text ...
    std::string response_text = "從API取得的回覆"; 

    // 4. 更新歷史紀錄
    history.push_back({new_question, response_text});
    
    return response_text;
}

AnythingLLM::AnythingLLM(std::string host, int port, std::string api_key) 
    : key("Bearer " + api_key) {
    
    cli = std::make_unique<httplib::Client>(host, port);
    startNewPatient();
    cli->set_connection_timeout(5, 0); 
    cli->set_read_timeout(60, 0); 
}

std::string AnythingLLM::ask(std::string slug, std::string message) {
    nlohmann::json body = {
        {"message", message}, 
        {"mode", "chat"}, 
        {"sessionId", current_session}
    };
    
    httplib::Headers headers = {
        {"Authorization", key}, 
        {"Content-Type", "application/json"}
    };
    
    std::string path = "/api/v1/workspace/" + slug + "/chat";
    
    auto res = cli->Post(path.c_str(), headers, body.dump(), "application/json");
    
    if (res) {
        if (res->status == 200) {
            auto res_json = nlohmann::json::parse(res->body);
            return res_json["textResponse"];
        } else {
            return "HTTP Error: " + std::to_string(res->status) + " - " + res->body;
        }
    }
    
    return "Connection Failed.";
}

void AnythingLLM::startNewPatient() {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    current_session = "patient-" + std::to_string(now);
    std::cout << "[System] New conversation session ID: " << current_session << std::endl;
}    
