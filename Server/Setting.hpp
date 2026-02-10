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
    bool bVideoWindowFullScreen = true;
    bool bShowPreviewWindow = false;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Setting, StateControlFile, bServerPlaysRobotReceivedAudio, bVideoWindowFullScreen, bShowPreviewWindow
)

#endif // SETTING_HPP