#include <nlohmann/json.hpp>
#include <iostream>

#include <vector>
#include <string>
#include "httplib.h"

class AnythingLLM {
public:
    AnythingLLM(std::string host, int port, std::string api_key);

    std::string ask(std::string slug, std::string message);

    void startNewPatient();
private:
    std::unique_ptr<httplib::Client> cli;
    std::string key;
    std::string current_session;
};
