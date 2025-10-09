#include "LandmarkToRobotAction.hpp"
#include <cmath>
#include "RobotStatus.hpp"
#include "ActionOption.hpp"

float prev_x = 0.5;     //Mohamed's variable, to store the previous x position of the face

// 3D Euclidean distance
float euclidean_distance(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    return std::sqrt(
        std::pow(a[0] - b[0], 2) +
        std::pow(a[1] - b[1], 2) +
        std::pow(a[2] - b[2], 2)
    );
}

int FaceLandmarks_to_RobotAction(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,
    RobotCommandProtobuf::RobotCommand &command)
{
    //If there are multiple faces, find the largest one. I don't have the code yet.
    int num_faces = normalized_landmarks.size();

    std::array<int, 9> left_eye{{  33 , 133, 246, 161, 160, 159, 158, 157, 173 }};
    std::array<int, 9> right_eye{{ 362, 263, 390, 389, 388, 387, 386, 385, 384 }};
    for(int i=0; i<num_faces; i++)
    {
        std::vector<std::array<float, 3>> face_landmarks = normalized_landmarks[i];
        float left_eye_x = 0;
        float left_eye_y = 0;
        float right_eye_x = 0;
        float right_eye_y = 0;

        for(int j=0; j<left_eye.size(); j++)
        {
            left_eye_x += face_landmarks[left_eye[j]][0];
            left_eye_y += face_landmarks[left_eye[j]][1];
            right_eye_x += face_landmarks[right_eye[j]][0];
            right_eye_y += face_landmarks[right_eye[j]][1];
        }

        left_eye_x /= left_eye.size();
        left_eye_y /= left_eye.size();
        right_eye_x /= right_eye.size();
        right_eye_y /= right_eye.size();

        float x = (left_eye_x + right_eye_x) / 2;
        float y = (left_eye_y + right_eye_y) / 2;

        //std::cout << "center_of_two_eyes: (" << x << ", " << y << ")" << std::endl;
        // Calculate the distance between the eyes

        //Because there is no API to set the turn angle, it is better to monitor every frame rather than sending a command every second.
        //Currently, Mohamed lets kebbi turn slowly. But I don't think it is a good idea.
        if (action_option.move_mode == action_option.MOVE_BODY)
        {
            //Only Zenbo has theta, Kebbi does not have it.
//            float theta = -(x-0.5)*62.5;
            float pitch_shift = (y-0.5)*48.9;         //Kebbi's postive pitch degreee is downward
//            command.set_degree(static_cast<int>(theta));
            command.set_yaw(0);
            status.yaw_degree = 0;

            int pitch = status.pitch_degree + static_cast<int>(pitch_shift);
            if( pitch < -20 ) pitch = -20;
            if( pitch > 20 ) pitch = 20;
            command.set_pitch(pitch);
            status.pitch_degree = pitch;

            //Mohamed's code works.
            //std::cout << "Error: " << x - 0.5 << "\n";
            //std::cout << "Previous_X: " << prev_x << "\n";
            //std::cout << "Change: " << (x - prev_x) << "\n";
            int k_p = 2, k_d = 1.5;
            float mag = abs(x - 0.5) * k_p;
//            int current_time = time(0);
            mag += (prev_x - x) * k_d; // / (current_time - prev_time + 0.1) * k_d;
            prev_x = x;
            //prev_time = current_time;
            if(x > 0.55)
            {
                command.set_turnspeed(-30.0f * mag);
//                command.set_turnspeed(-50.0f);
            }
            else if (x < 0.45)
            {
                command.set_turnspeed(30.0f * mag);
//                command.set_turnspeed(50.0f);
            }
            else
            {
                command.set_turnspeed(0.0f);
            }
        }
        else  //move head
        {
            float yaw_shift = -(x-0.5)*62.5;
            float pitch_shift = (y-0.5)*48.9;         //Kebbi's postive pitch degree is downward
            //I need to know current yaw
            int yaw = status.yaw_degree + static_cast<int>(yaw_shift);
            if( yaw < -40) yaw = -40;
            if( yaw > 40) yaw = 40;
            command.set_yaw(yaw);
            status.yaw_degree = yaw;
            
            int pitch = status.pitch_degree + static_cast<int>(pitch_shift);
            if( pitch < -20 ) pitch = -20;
            if( pitch > 20 ) pitch = 20;
            command.set_pitch(pitch);
            status.pitch_degree = pitch;
        }
    }
    return 1;
}

int PoseLandmarks_to_RobotAction(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,
    RobotCommandProtobuf::RobotCommand &command)
{
    //Currently, Mediapipe only detects one person.
    int num_poses = normalized_landmarks.size();

    for(int i=0; i<num_poses; i++)
    {
        std::vector<std::array<float, 3>> pose_landmarks = normalized_landmarks[i];

        //index 0 is the nose
        float x = pose_landmarks[0][0];
        float y = pose_landmarks[0][1];

//        std::cout << "Pose node 0 Normalized position: (" << x << ", " << y << ")" << std::endl;
        // Calculate the distance between the eyes

        if (action_option.move_mode == action_option.MOVE_BODY)
        {
            float theta = -(x-0.5)*62.5;
            float pitch_shift = (y-0.5)*48.9;      //Kebbi's postive pitch degreee is downward
//            command.set_degree(static_cast<int>(theta));
            command.set_yaw(0);
            status.yaw_degree = 0;

            int pitch = status.pitch_degree + static_cast<int>(pitch_shift);
            if( pitch < -20 ) pitch = -20;
            if( pitch > 20 ) pitch = 20;
            command.set_pitch(pitch);
            status.pitch_degree = pitch;
        }
        else  //move head
        {
            float yaw_shift = -(x-0.5)*62.5;        //The 62.5 and 48.9 are Zenbo's camera horizontal and vertical view angle
            float pitch_shift = (y-0.5)*48.9;       //Kebbi's postive pitch degreee is downward
            //I need to know current yaw
            int yaw = status.yaw_degree + static_cast<int>(yaw_shift);
            if( yaw < -40) yaw = -40;
            if( yaw > 40) yaw = 40;
            command.set_yaw(yaw);
            status.yaw_degree = yaw;

            int pitch = status.pitch_degree + static_cast<int>(pitch_shift);
            if( pitch < -20 ) pitch = -20;
            if( pitch > 20 ) pitch = 20;
            command.set_pitch(pitch);
            command.set_headspeed(100);     //I need to associate with UI later.
            status.pitch_degree = pitch;
        }
    }
    //command.set_turnspeed(20);
    return 1;
}