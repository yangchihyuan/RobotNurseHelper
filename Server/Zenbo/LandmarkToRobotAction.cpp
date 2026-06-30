#include "LandmarkToRobotAction.hpp"
#include <cmath>
#include "RobotStatus.hpp"
#include "ActionOption.hpp"

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

        // Zenbo rotation: positive pitch is upward
        double x_g = xc * cos(theta) - yc * sin(theta) * sin(phi) - zc * sin(theta) * cos(phi);
        double y_g = yc * cos(phi) - zc * sin(phi);
        double z_g = xc * sin(theta) + yc * cos(theta) * sin(phi) + zc * cos(theta) * cos(phi);

        double x_z_norm = sqrt(x_g * x_g + z_g * z_g);
        double target_theta = atan2(-x_g, z_g) * 180.0 / M_PI;
        double target_phi = atan2(-y_g, x_z_norm) * 180.0 / M_PI;

        target_yaw = static_cast<int>(round(target_theta));
        target_pitch = static_cast<int>(round(target_phi));
    }
    else
    {
        float yaw_shift = -(x - 0.5f) * 62.5f;
        float pitch_shift = -(y - 0.5f) * 48.9f;
        target_yaw = status.yaw_degree + static_cast<int>(yaw_shift);
        target_pitch = status.pitch_degree + static_cast<int>(pitch_shift);
    }
}


int FaceLandmarks_to_RobotAction(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,
    RobotCommandProtobuf::RobotCommand &message)
{
    //If there are multiple faces, find the largest one.
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

        std::cout << "center_of_two_eyes: (" << x << ", " << y << ")" << std::endl;
        // Calculate the distance between the eyes

        if (action_option.move_mode == action_option.MOVE_BODY)
        {
            float theta = -(x-0.5)*62.5;
            message.set_degree(static_cast<int>(theta));
            message.set_yaw(0);
            status.yaw_degree = 0;

            int dummy_yaw, pitch;
            ComputeTargetYawPitch(x, y, status, action_option.bUseVisualCompass, dummy_yaw, pitch);
            if( pitch < -15 ) pitch = -15;
            if( pitch > 55 ) pitch = 55;
            message.set_pitch(pitch);
            status.pitch_degree = pitch;
        }
        else  //move head
        {
            int yaw, pitch;
            ComputeTargetYawPitch(x, y, status, action_option.bUseVisualCompass, yaw, pitch);
            if( yaw < -45) yaw = -45;
            if( yaw > 45) yaw = 45;
            message.set_yaw(yaw);
            status.yaw_degree = yaw;

            if( pitch < -15 ) pitch = -15;
            if( pitch > 55 ) pitch = 55;
            message.set_pitch(pitch);
            status.pitch_degree = pitch;
        }
    }

    return 1;
}

int PoseLandmarks_to_RobotAction(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,
    RobotCommandProtobuf::RobotCommand &message)
{
    //If there are multiple faces, find the largest one.
    int num_poses = normalized_landmarks.size();

//    std::array<int, 9> left_eye{{  33 , 133, 246, 161, 160, 159, 158, 157, 173 }};
//    std::array<int, 9> right_eye{{ 362, 263, 390, 389, 388, 387, 386, 385, 384 }};
 
    for(int i=0; i<num_poses; i++)
    {
        std::vector<std::array<float, 3>> pose_landmarks = normalized_landmarks[i];

        //index 0 is the nose
        float x = pose_landmarks[0][0];
        float y = pose_landmarks[0][1];

        std::cout << "Pose node 0 Normalized position: (" << x << ", " << y << ")" << std::endl;
        // Calculate the distance between the eyes

        if (action_option.move_mode == action_option.MOVE_BODY)
        {
            float theta = -(x-0.5)*62.5;
            message.set_degree(static_cast<int>(theta));
            message.set_yaw(0);
            status.yaw_degree = 0;

            int dummy_yaw, pitch;
            ComputeTargetYawPitch(x, y, status, action_option.bUseVisualCompass, dummy_yaw, pitch);
            if( pitch < -15 ) pitch = -15;
            if( pitch > 55 ) pitch = 55;
            message.set_pitch(pitch);
            status.pitch_degree = pitch;
        }
        else  //move head
        {
            int yaw, pitch;
            ComputeTargetYawPitch(x, y, status, action_option.bUseVisualCompass, yaw, pitch);
            if( yaw < -45) yaw = -45;
            if( yaw > 45) yaw = 45;
            message.set_yaw(yaw);
            status.yaw_degree = yaw;

            if( pitch < -15 ) pitch = -15;
            if( pitch > 55 ) pitch = 55;
            message.set_pitch(pitch);
            status.pitch_degree = pitch;
        }
    }

    return 1;
}
