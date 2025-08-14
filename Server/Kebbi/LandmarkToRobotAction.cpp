#include "LandmarkToRobotAction.hpp"
#include <cmath>
#include "RobotStatus.hpp"
#include "ActionOption.hpp"

float prev_x = 0.5;

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
    RobotCommandProtobuf::RobotCommand &message)
{
    //If there are multiple faces, find the largest one.
    int num_faces = normalized_landmarks.size();

    std::array<int, 9> left_eye{{  33 , 133, 246, 161, 160, 159, 158, 157, 173 }};
    std::array<int, 9> right_eye{{ 362, 263, 390, 389, 388, 387, 386, 385, 384 }};
    std::cout << "NUM_FACES: " << num_faces << "\n";
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
            float pitch_shift = (y-0.5)*48.9;         //Kebbi's postive pitch degreee is downward
//            message.set_degree(static_cast<int>(theta));
            message.set_yaw(0);
            status.yaw_degree = 0;

            int pitch = status.pitch_degree + static_cast<int>(pitch_shift);
            if( pitch < -20 ) pitch = -20;
            if( pitch > 20 ) pitch = 20;
            message.set_pitch(pitch);
            status.pitch_degree = pitch;
            
            if(num_faces == 1)
            {
                std::cout << "Error: " << x - 0.5 << "\n";
                std::cout << "Previous_X: " << prev_x << "\n";
                std::cout << "Change: " << (x - prev_x) << "\n";
                int k_p = 2, k_d = 1.5;
                float mag = abs(x - 0.5) * k_p;
                int current_time = time(0);
                mag += (prev_x - x) * k_d; // / (current_time - prev_time + 0.1) * k_d;
                prev_x = x;
                //prev_time = current_time;
                if(x > 0.55)
                {
                    message.set_turnspeed(-30.0f * mag);
                }
                else if (x < 0.45)
                {
                    message.set_turnspeed(30.0f * mag);
                }
                else
                {
                    message.set_turnspeed(0.0f);
                }

            }

            // std::string emotion = "Neutral";  // default

            // // Normalize or precompute these based on interocular distance or other scaling if needed
            // float mouth_width = euclidean_distance(face_landmarks[61], face_landmarks[291]); // corners of mouth
            // float mouth_open = euclidean_distance(face_landmarks[13], face_landmarks[14]);   // top-bottom lips

            // float left_eye_open = std::abs(face_landmarks[159][1] - face_landmarks[145][1]);    // left eye top-bottom
            // float right_eye_open = std::abs(face_landmarks[386][1] - face_landmarks[374][1]);   // right eye top-bottom
            // float eye_open = (left_eye_open + right_eye_open) / 2.0f;

            // float left_brow_raise = (face_landmarks[159][1] + face_landmarks[145][1]) / 2.0f - face_landmarks[65][1];  // eye to brow
            // float right_brow_raise = (face_landmarks[386][1] + face_landmarks[374][1]) / 2.0f - face_landmarks[295][1];
            // float brow_raise = (left_brow_raise + right_brow_raise) / 2.0f;

            // // --- Heuristics ---
            // if (mouth_open > 0.05 && mouth_width > 0.10 && eye_open > 0.03 && brow_raise > 0.03) {
            //     emotion = "Surprised";
            // }
            // else if (mouth_width > 0.08 && mouth_open < 0.02 && brow_raise < 0.015) {
            //     emotion = "Happy";
            // }
            // else if (brow_raise < -0.01 && eye_open < 0.02) {
            //     emotion = "Angry";
            // }
            // else if (brow_raise > 0.02 && eye_open < 0.015 && mouth_open < 0.015) {
            //     emotion = "Sad";
            // }
            // else if (mouth_open < 0.015 && brow_raise < 0.015 && eye_open < 0.02) {
            //     emotion = "Neutral";
            // }
            // std::cout << "\n" << emotion << "mouth_open: " << mouth_open << "mouth_open: " << mouth_open << "mouth_open: " << mouth_open "\n\n";
        }
        else  //move head
        {
            float mag = 0;
            if(num_faces == 1)
            {
                int k_p = 2, k_d = 1.5;
                mag = abs(x - 0.5) * k_p;
                int current_time = time(0);
                mag += (prev_x - x) * k_d; // / (current_time - prev_time + 0.1) * k_d;
                prev_x = x;
                //prev_time = current_time;
                if(x > 0.7)
                {
                    message.set_turnspeed(-20.0f * mag);
                }
                else if (x < 0.3)
                {
                    message.set_turnspeed(20.0f * mag);
                }
                else
                {
                    message.set_turnspeed(0.0f);
                }
            }   
            message.set_turnspeed(0);
            float yaw_shift = -(x-0.5)*62.5;
            float pitch_shift = (y-0.5)*48.9;         //Kebbi's postive pitch degree is downward
            //I need to know current yaw
            int yaw = status.yaw_degree + static_cast<int>(yaw_shift);
            yaw *= 0.4; //[MOHAMED]
            yaw *= (1 - mag) * 1.5;
            if( yaw < -40) yaw = -40;
            if( yaw > 40) yaw = 40;
            message.set_yaw(yaw);
            status.yaw_degree = yaw;
            
            int pitch = status.pitch_degree + static_cast<int>(pitch_shift);
            pitch *= 0.4; //[MOHAMED]
            if( pitch < -20 ) pitch = -20;
            if( pitch > 20 ) pitch = 20;
            message.set_pitch(pitch);
            status.pitch_degree = pitch;
        }
    }
    if(num_faces == 0)
    {
        message.set_turnspeed(40.0f);
    }
    //message.set_turnspeed(5);
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

//        std::cout << "Pose node 0 Normalized position: (" << x << ", " << y << ")" << std::endl;
        // Calculate the distance between the eyes

        if (action_option.move_mode == action_option.MOVE_BODY)
        {
            float theta = -(x-0.5)*62.5;
            float pitch_shift = (y-0.5)*48.9;      //Kebbi's postive pitch degreee is downward
//            message.set_degree(static_cast<int>(theta));
            message.set_yaw(0);
            status.yaw_degree = 0;

            int pitch = status.pitch_degree + static_cast<int>(pitch_shift);
            if( pitch < -20 ) pitch = -20;
            if( pitch > 20 ) pitch = 20;
            message.set_pitch(pitch);
            status.pitch_degree = pitch;
        }
        else  //move head
        {
            float yaw_shift = -(x-0.5)*62.5;
            float pitch_shift = (y-0.5)*48.9;       //Kebbi's postive pitch degreee is downward
            //I need to know current yaw
            int yaw = status.yaw_degree + static_cast<int>(yaw_shift);
            if( yaw < -40) yaw = -40;
            if( yaw > 40) yaw = 40;
            message.set_yaw(yaw);
            status.yaw_degree = yaw;

            int pitch = status.pitch_degree + static_cast<int>(pitch_shift);
            if( pitch < -20 ) pitch = -20;
            if( pitch > 20 ) pitch = 20;
            message.set_pitch(pitch);
            message.set_headspeed(100);     //I need to associate with UI later.
            status.pitch_degree = pitch;
        }
    }
    //message.set_turnspeed(20);
    return 1;
}