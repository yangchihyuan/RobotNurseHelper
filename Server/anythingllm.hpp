#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>

class AnythingLLM {
public:
    AnythingLLM(std::string host, int port, std::string api_key) 
        : cli(host, port), key("Bearer " + api_key) {
        
        cli.set_connection_timeout(5, 0); 
        cli.set_read_timeout(60, 0); 
    }

    std::string ask(std::string slug, std::string message) {
        nlohmann::json body = {
            {"message", message}, 
            {"mode", "chat"}, 
            {"sessionId", "patient-123"}
        };
        
        httplib::Headers headers = {
            {"Authorization", key}, 
            {"Content-Type", "application/json"}
        };
        
        std::string path = "/api/v1/workspace/" + slug + "/chat";
        
        auto res = cli.Post(path.c_str(), headers, body.dump(), "application/json");
        
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

private:
    httplib::Client cli;
    std::string key;
};
