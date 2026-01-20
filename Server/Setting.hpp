#ifndef SETTING_HPP
#define SETTING_HPP

#include <string>
#include <nlohmann/json.hpp>
#include "utility_json.hpp"

using namespace std;

struct Setting
{
    string StateControlFile;
    bool bServerPlaysRobotReceivedAudio = false;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Setting, StateControlFile, bServerPlaysRobotReceivedAudio
)

#endif // SETTING_HPP