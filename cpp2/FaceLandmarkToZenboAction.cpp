#include "FaceLandmarkToZenboAction.hpp"
#include <cmath>
#include "RobotStatus.hpp"
#include "ActionOption.hpp"

int FaceLandmarks_to_ZenboAction(std::vector<std::vector<std::array<float, 3>>> normalized_landmarks, 
    RobotStatus &status, 
    ActionOption action_option,
    ZenboNurseHelperProtobuf::ReportAndCommand &message)
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
            float pitch_shift = (y-0.5)*48.9;
            message.set_degree(theta);
            message.set_yaw(0);
            status.yaw_degree = 0;

            int pitch = status.pitch_degree + static_cast<int>(pitch_shift);
            if( pitch < -15 ) pitch = -15;
            if( pitch > 55 ) pitch = 55;
            message.set_pitch(pitch);
            status.pitch_degree = pitch;
        }
        else  //move head
        {
            float yaw_shift = -(x-0.5)*62.5;
            float pitch_shift = (y-0.5)*48.9;
            //I need to know current yaw
            int yaw = status.yaw_degree + static_cast<int>(yaw_shift);
            if( yaw < -45) y = -45;
            if( yaw > 45) y = 45;
            message.set_yaw(yaw);
            status.yaw_degree = yaw;

            int pitch = status.pitch_degree + static_cast<int>(pitch_shift);
            if( pitch < -15 ) pitch = -15;
            if( pitch > 55 ) pitch = 55;
            message.set_pitch(pitch);
            status.pitch_degree = pitch;
        }

//        float eye_distance = sqrt(pow(right_eye_x - left_eye_x, 2) + pow(right_eye_y - left_eye_y, 2));

        //The coordinate system is Cardinal.
        //The left bottom of the image is (0,0), and the right top corner is (1,1).

        // Convert to Zenbo's coordinate system

        // Check if the distance is within a certain threshold
    }

    return 1;
}