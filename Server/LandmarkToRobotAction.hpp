#ifndef __LANDMRK_TO_ROBOT_ACTION_hpp__
#define __LANDMRK_TO_ROBOT_ACTION_hpp__

#include <vector>
#include <array>

#ifdef USE_KEBBI
    #include "Kebbi/RobotCommand.pb.h"
#elif USE_ZENBO
    #include "Zenbo/RobotCommand.pb.h"
#endif

#include "RobotStatus.hpp"
#include "ActionOption.hpp"

using namespace std;

int FaceLandmarks_to_RobotAction(vector<std::vector<array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    RobotCommandProtobuf::RobotCommand &command);

int PoseLandmarks_to_RobotAction(vector<vector<array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    RobotCommandProtobuf::RobotCommand &command);

int PoseLandmarks_to_RobotAction_yolo(vector<vector<array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    RobotCommandProtobuf::RobotCommand &command);
    

#endif