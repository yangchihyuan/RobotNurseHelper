#include "LandmarkToRobotAction.hpp"
#include <cmath>
#include "RobotStatus.hpp"
#include "ActionOption.hpp"

float prev_x = 0.5;     //Mohamed's variable, to store the previous x position of the face

void ComputeTargetYawPitch(float x, float y, const RobotStatus &status, bool bUseVisualCompass, int &target_yaw, int &target_pitch)
{
    if (bUseVisualCompass)
    {
        double h_fov_rad = 62.5 * M_PI / 180.0;
        double v_fov_rad = 48.9 * M_PI / 180.0;
        double xc = 2.0 * (x - 0.5) * tan(h_fov_rad / 2.0);
        double yc = 2.0 * (y - 0.5) * tan(v_fov_rad / 2.0);
        double zc = 1.0;

        double theta = status.yaw_degree * M_PI / 180.0;
        double phi = status.pitch_degree * M_PI / 180.0;

        // Kebbi rotation: positive pitch is downward
        double x_g = xc * cos(theta) + yc * sin(theta) * sin(phi) - zc * sin(theta) * cos(phi);
        double y_g = yc * cos(phi) + zc * sin(phi);
        double z_g = xc * sin(theta) - yc * cos(theta) * sin(phi) + zc * cos(theta) * cos(phi);

        double x_z_norm = sqrt(x_g * x_g + z_g * z_g);
        double target_theta = atan2(-x_g, z_g) * 180.0 / M_PI;
        double target_phi = atan2(y_g, x_z_norm) * 180.0 / M_PI;

        target_yaw = static_cast<int>(round(target_theta));
        target_pitch = static_cast<int>(round(target_phi));
    }
    else
    {
        float yaw_shift = -(x - 0.5f) * 62.5f;
        float pitch_shift = (y - 0.5f) * 48.9f;
        target_yaw = status.yaw_degree + static_cast<int>(yaw_shift);
        target_pitch = status.pitch_degree + static_cast<int>(pitch_shift);
    }
}

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
//            command.set_degree(static_cast<int>(theta));
            command.set_yaw(0);
            status.yaw_degree = 0;

            int dummy_yaw, pitch;
            ComputeTargetYawPitch(x, y, status, action_option.bUseVisualCompass, dummy_yaw, pitch);
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
            int yaw, pitch;
            ComputeTargetYawPitch(x, y, status, action_option.bUseVisualCompass, yaw, pitch);
            if( yaw < -40) yaw = -40;
            if( yaw > 40) yaw = 40;
            command.set_yaw(yaw);
            status.yaw_degree = yaw;
            
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
            //Only Zenbo has theta, Kebbi does not have it.
//            float theta = -(x-0.5)*62.5;
//            command.set_degree(static_cast<int>(theta));
            command.set_yaw(0);
            status.yaw_degree = 0;

            int dummy_yaw, pitch;
            ComputeTargetYawPitch(x, y, status, action_option.bUseVisualCompass, dummy_yaw, pitch);
            if( pitch < -20 ) pitch = -20;
            if( pitch > 20 ) pitch = 20;
            command.set_pitch(pitch);
            status.pitch_degree = pitch;

            //Mohamed's code works.
            //What is this?
            //std::cout << "Error: " << x - 0.5 << "\n";
            //std::cout << "Previous_X: " << prev_x << "\n";
            //std::cout << "Change: " << (x - prev_x) << "\n";
            int k_p = 2, k_d = 1.5;     //What is ths k_p and k_d?
            float mag = abs(x - 0.5) * k_p;
            mag += (prev_x - x) * k_d; // / (current_time - prev_time + 0.1) * k_d;
            prev_x = x;
            if(x > 0.55)
            {
                command.set_turnspeed(-30.0f * mag);
            }
            else if (x < 0.45)
            {
                command.set_turnspeed(30.0f * mag);
            }
            else
            {
                command.set_turnspeed(0.0f);
            }
        }
        else  //move head
        {
            int yaw, pitch;
            ComputeTargetYawPitch(x, y, status, action_option.bUseVisualCompass, yaw, pitch);
            if( yaw < -40) yaw = -40;
            if( yaw > 40) yaw = 40;
            command.set_yaw(yaw);
            status.yaw_degree = yaw;

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

int PoseLandmarks_to_RobotAction_yolo(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
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
            //Only Zenbo has theta, Kebbi does not have it.
//            float theta = -(x-0.5)*62.5;
//            command.set_degree(static_cast<int>(theta));
            command.set_yaw(0);
            status.yaw_degree = 0;

            int dummy_yaw, pitch;
            ComputeTargetYawPitch(x, y, status, action_option.bUseVisualCompass, dummy_yaw, pitch);
            if( pitch < -20 ) pitch = -20;
            if( pitch > 20 ) pitch = 20;
            command.set_pitch(pitch);
            status.pitch_degree = pitch;

            //Mohamed's code works.
            //What is this?
            //std::cout << "Error: " << x - 0.5 << "\n";
            //std::cout << "Previous_X: " << prev_x << "\n";
            //std::cout << "Change: " << (x - prev_x) << "\n";
            int k_p = 2, k_d = 1.5;     //What is ths k_p and k_d?
            float mag = abs(x - 0.5) * k_p;
            mag += (prev_x - x) * k_d; // / (current_time - prev_time + 0.1) * k_d;
            prev_x = x;
            if(x > 0.55)
            {
                //This is Kebbi's command. I can only control the rotation speed.
                command.set_turnspeed(-30.0f * mag);
            }
            else if (x < 0.45)
            {
                command.set_turnspeed(30.0f * mag);
            }
            else
            {
                command.set_turnspeed(0.0f);
            }
        }
        else  //move head
        {
            int yaw, pitch;
            ComputeTargetYawPitch(x, y, status, action_option.bUseVisualCompass, yaw, pitch);
            if( yaw < -40) yaw = -40;
            if( yaw > 40) yaw = 40;
            command.set_yaw(yaw);
            status.yaw_degree = yaw;

            if( pitch < -20 ) pitch = -20;
            if( pitch > 20 ) pitch = 20;
            command.set_pitch(pitch);
            command.set_headspeed(100);     //I need to associate with UI later.
            status.pitch_degree = pitch;
        }
    }
    return 1;
}