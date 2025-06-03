#include <vector>
#include <array>


extern std::vector<std::vector<std::array<float, 3>>> global_landmarks; //[MOHAMED]

#ifdef USE_KEBBI
    #include "Kebbi/RobotCommand.pb.h"
#elif USE_ZENBO
    #include "Zenbo/RobotCommand.pb.h"
#endif
#include "RobotStatus.hpp"
#include "ActionOption.hpp"

int FaceLandmarks_to_RobotAction(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    RobotCommandProtobuf::RobotCommand &message);

int PoseLandmarks_to_RobotAction(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,    
    RobotCommandProtobuf::RobotCommand &message);
  
