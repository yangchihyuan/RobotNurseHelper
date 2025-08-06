//2025/08/06 This is designed for Rachael's project. She need to use a tablet to get the level of uncomfortable of the child.

#include "ThreadTablet.hpp"
#include "utility_TimeRecorder.hpp"
#include "utility_directory.hpp"
#include "utility_string.hpp"
#include "utility_directory.hpp"

void ThreadTablet::run()
{
    while(b_WhileLoop)
    {
        if( pSocketHandler->get_queue_length() > 0 )    //here is an infinite loop
        {

            //Get message from the queue
            Message message = pSocketHandler->get_head();
            pSocketHandler->pop_head();
            char *data_ = message.data.get();
            int faceIndex;
            memcpy(&faceIndex, data_, sizeof(int));
            std::string str_RobotSpeakSentence;
            int RobotExpressionIndex = 0;
            switch( faceIndex )
            {
                case 1:
                    str_RobotSpeakSentence = "一";
                    RobotExpressionIndex = RobotCommandProtobuf::RobotCommand::FaceEnum::RobotCommand_FaceEnum_TTS_JoyA;
                    break;
                case 2:
                    str_RobotSpeakSentence = "二";
                    RobotExpressionIndex = RobotCommandProtobuf::RobotCommand::FaceEnum::RobotCommand_FaceEnum_TTS_JoyB;
                    break;
                case 3:
                    str_RobotSpeakSentence = "三";
                    RobotExpressionIndex = RobotCommandProtobuf::RobotCommand::FaceEnum::RobotCommand_FaceEnum_TTS_JoyC;
                    break;
                case 4:
                    str_RobotSpeakSentence = "四";
                    RobotExpressionIndex = RobotCommandProtobuf::RobotCommand::FaceEnum::RobotCommand_FaceEnum_TTS_SadnessA;
                    break;
                case 5:
                    str_RobotSpeakSentence = "五";
                    RobotExpressionIndex = RobotCommandProtobuf::RobotCommand::FaceEnum::RobotCommand_FaceEnum_TTS_SadnessB;
                    break;
            }
            RobotCommandProtobuf::RobotCommand robot_command;
            robot_command.set_speak_sentence(str_RobotSpeakSentence);
            robot_command.set_face(RobotExpressionIndex);
            pSendMessageManager->AddMessage(robot_command);
        }
        else
        {
            //wait until being notified
            std::unique_lock<std::mutex> lk(mtx);
            cond_var_tablet.wait(lk);
        }
    }
    cout << "Exit tablet while loop." << std::endl;
}
