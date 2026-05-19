#include "ThreadReceiveMessage.hpp"
#include "utility_time.hpp"
#ifdef USE_KEBBI
    #include "Kebbi/RobotCommand.pb.h"
#elif USE_ZENBO
    #include "Zenbo/RobotCommand.pb.h"
#endif


void ThreadReceiveMessage::run()
{
    while(b_WhileLoop)
    {
        std::unique_lock<std::mutex> lk(mtx);
        cond_var_receive_message.wait(lk);

        while(DataFrames_queue.size() > 0 )    //process all messages in the queue{
        {
            //Get message from the queue
            DataFrame dataframe;// = DataFrames_queue.front();
            DataFrames_queue.pop(dataframe);
            char *data_ = dataframe.data.get();
            size_t data_length = dataframe.length;

            //Here, I need to parse the protobuf object
            RobotCommandProtobuf::RobotToServerMessage RTSmessage;
            RTSmessage.ParseFromArray(data_, static_cast<int>(data_length));

            if( RTSmessage.has_description())
            {
                if(RTSmessage.description() == "onTTSComplete")
                {
                    //debug
                    {
//                        google::protobuf::Timestamp timestamp = RTSmessage.event_time();
//                        cout << "Receive Protobuf onTTSComplete signal, whose time is " << ConvertTimeToString(protobufTimestampToTimePoint(timestamp)) << endl;
//                        cout << "On received moment, system time is " << GetCurrentTimeString() << endl;
                        //They are not the same. It is difficult to control.
                        //System time may be smaller than the Protobuf time, which shoes that the two clock is not synchronized.
                    }
                    //notify ThreadStateControl
                    //2025/8/20, The robot time is different from the server's time
                    //mpThreadStateControl->NotifyEvent("onTTSComplete", protobufTimestampToTimePoint(timestamp));
                    //I have to use the server's time.
                    mpThreadStateControl->NotifyEvent("onTTSComplete", chrono::system_clock::now());
                }
                else if(RTSmessage.description() == "onCompleteOfMotionPlay")
                {
                    mpThreadProcessImage->NotifyEvent("onCompleteOfMotionPlay", chrono::system_clock::now(), RTSmessage.yaw(), RTSmessage.pitch());
                }
                else if(RTSmessage.description() == "onActivityRestart")   //The mbtx activity complete
                {
                    //debug
                    cout << "(C) Receive onActivityRestart signal" << endl;
                    mpThreadStateControl->NotifyEvent("onActivityRestart", chrono::system_clock::now());
                }
                
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

            if( RTSmessage.has_tabletcommand())
            {
                std::string str_tabletcommand = RTSmessage.tabletcommand();                int RobotExpressionIndex = 0;
                cout << "Receive tabletcommand: " << str_tabletcommand << endl;
                if( str_tabletcommand == "Restart")
                {
                    //Restart robot's control state.
                    mpThreadStateControl->Restart();
                }
                else if( str_tabletcommand == "Mandarin" )
                {
                    //TODO: change language
                }
                else if( str_tabletcommand == "English" )
                {
                    //TODO: change language
                }
                else
                {
                    cout << "Unknown tablet command." << endl;
                }
            }

        }
    }
    cout << "Exit ReceiveMessages loop." << std::endl;
}
