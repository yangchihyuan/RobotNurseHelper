//2025/08/06 This is designed for Rachael's project. She need to use a tablet to get the level of uncomfortable of the child.

#include "ThreadReceiveMessages.hpp"
//#include "utility_TimeRecorder.hpp"
//#include "utility_directory.hpp"
//#include "utility_string.hpp"
//#include "utility_directory.hpp"
#include "utility_time.hpp"
#ifdef USE_KEBBI
    #include "Kebbi/RobotCommand.pb.h"
#elif USE_ZENBO
    #include "Zenbo/RobotCommand.pb.h"
#endif


void ThreadReceiveMessages::run()
{
    while(b_WhileLoop)
    {
        if( pSocketHandler->get_queue_length() > 0 )    //here is an infinite loop
        {

            //Get message from the queue
            DataFrame dataframe = pSocketHandler->get_head();
            pSocketHandler->pop_head();
            char *data_ = dataframe.data.get();

            //Here, I need to parse the protobuf object
            RobotCommandProtobuf::RobotToServerMessage RTSmessage;
            RTSmessage.ParseFromString(data_);

            if( RTSmessage.has_description() && RTSmessage.description() == "onTTSComplete")
            {
                //debug
                {
                google::protobuf::Timestamp timestamp = RTSmessage.event_time();
                cout << "Receive onTTSComplete signal, whose time is " << ConvertTimeToString(protobufTimestampToTimePoint(timestamp)) << endl;
                cout << "On received moment, system time is" << GetCurrentTimeString() << endl;
                }
                //notify ThreadStateControl
                //2025/8/20, The robot time is different from the server's time
                //mpThreadStateControl->NotifyEvent("onTTSComplete", protobufTimestampToTimePoint(timestamp));
                //I have to use the server's time.
                mpThreadStateControl->NotifyEvent("onTTSComplete", chrono::system_clock::now());
            }

            if( RTSmessage.has_numberpressed())
            {
                int numberpressed = RTSmessage.numberpressed(); 

                std::string str_RobotSpeakSentence;
                int RobotExpressionIndex = 0;
                cout << "Receive number " << numberpressed << endl;
                switch( numberpressed )
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
        }
        else
        {
            //wait until being notified
            std::unique_lock<std::mutex> lk(mtx);
            cond_var_receive_messages.wait(lk);
        }
    }
    cout << "Exit ReceiveMessages loop." << std::endl;
}
