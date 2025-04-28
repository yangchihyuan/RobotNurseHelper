#include <vector>
#include <array>

#include "RobotCommand.pb.h"
#include "RobotStatus.hpp"
#include "ActionOption.hpp"

int FaceLandmarks_to_ZenboAction(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    RobotCommandProtobuf::ReportAndCommand &message);

int PoseLandmarks_to_ZenboAction(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    RobotCommandProtobuf::ReportAndCommand &message);
    
int FaceLandmarks_to_RobotAction_Kebbi(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    RobotCommandProtobuf::ReportAndCommand &message);

int PoseLandmarks_to_RobotAction_Kebbi(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    RobotCommandProtobuf::ReportAndCommand &message);
    