#include <vector>
#include <array>

#include "ServerSend.pb.h"
#include "RobotStatus.hpp"
#include "ActionOption.hpp"

int FaceLandmarks_to_ZenboAction(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    ZenboNurseHelperProtobuf::ReportAndCommand &message);

int PoseLandmarks_to_ZenboAction(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    ZenboNurseHelperProtobuf::ReportAndCommand &message);
    