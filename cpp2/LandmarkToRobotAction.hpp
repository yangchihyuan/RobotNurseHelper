#include <vector>
#include <array>

#include "RobotCommand.pb.h"
#include "RobotStatus.hpp"
#include "ActionOption.hpp"

int FaceLandmarks_to_RobotAction_Zenbo(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    RobotCommandProtobuf::RobotCommand &message);

int PoseLandmarks_to_RobotAction_Zenbo(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    RobotCommandProtobuf::RobotCommand &message);
    
int FaceLandmarks_to_RobotAction_Kebbi(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    RobotCommandProtobuf::RobotCommand &message);

int PoseLandmarks_to_RobotAction_Kebbi(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    RobotCommandProtobuf::RobotCommand &message);
    