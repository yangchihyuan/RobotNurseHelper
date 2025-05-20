#include "ThreadTablet.hpp"
#include "utility_TimeRecorder.hpp"
#include "utility_directory.hpp"
#include "utility_string.hpp"
#include "utility_csv.hpp"
#include "utility_directory.hpp"
#ifdef USE_KEBBI
    #include "Kebbi/RobotCommand.pb.h"
#elif USE_ZENBO
    #include "Zenbo/RobotCommand.pb.h"
#endif

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
            int speed = 100;
            int volume = 80;
            int ZenboExpressionIndex = 0;
            switch( faceIndex )
            {
                case 0:
                    //str_RobotSpeakSentence = "It is great that you are not painful.";
                    str_RobotSpeakSentence = "太好了，你不痛了。";
                    speed = 120;
                    volume = 50;
                    ZenboExpressionIndex = RobotCommandProtobuf::RobotCommand::FaceEnum::RobotCommand_FaceEnum_HAPPY;
                    break;
                case 1:
                    //str_RobotSpeakSentence = "I hope you still bear a little pain.";
                    str_RobotSpeakSentence = "還有一點點痛，看看是不是很快會結束了。";
                    ZenboExpressionIndex = RobotCommandProtobuf::RobotCommand::FaceEnum::RobotCommand_FaceEnum_EXPECTING;
                    break;
                case 2:
                    //str_RobotSpeakSentence = "Hold on. It will pass soon.";
                    str_RobotSpeakSentence = "忍耐一下，這會很快結束的。";
                    volume = 90;
                    ZenboExpressionIndex = RobotCommandProtobuf::RobotCommand::FaceEnum::RobotCommand_FaceEnum_CONFIDENT;
                    break;
                case 3:
                    //str_RobotSpeakSentence = "Are you ok?";
                    str_RobotSpeakSentence = "你還好嗎？";
                    speed = 70;
                    volume = 60;
                    ZenboExpressionIndex = RobotCommandProtobuf::RobotCommand::FaceEnum::RobotCommand_FaceEnum_INNOCENT;
                    break;
                case 4:
                    //str_RobotSpeakSentence = "I am sorry to know that you are very fainful?";
                    str_RobotSpeakSentence = "你要吃一點止痛藥嗎？";
                    ZenboExpressionIndex = RobotCommandProtobuf::RobotCommand::FaceEnum::RobotCommand_FaceEnum_QUESTIONING;
                    break;
                case 5:
                    //str_RobotSpeakSentence = "Do I need to call the doctor?";
                    str_RobotSpeakSentence = "我需要叫醫生嗎？";
                    ZenboExpressionIndex = RobotCommandProtobuf::RobotCommand::FaceEnum::RobotCommand_FaceEnum_SHOCKED;
                    break;
            }
            cout << "Pain rate: " << faceIndex *2 << std::endl;
            RobotCommandProtobuf::RobotCommand report_data;
            report_data.set_speak_sentence(str_RobotSpeakSentence);
            report_data.set_speed(speed);
            report_data.set_volume(volume);
            report_data.set_speak_pitch(100);
            report_data.set_face(ZenboExpressionIndex) ;
            pSendMessageManager->AddMessage(report_data);
        }
        else
        {
            //wait until being notified
            std::unique_lock<std::mutex> lk(mtx);
            cond_var_tablet.wait(lk);
        }
    }
    cout << "Exit while loop." << std::endl;
}
