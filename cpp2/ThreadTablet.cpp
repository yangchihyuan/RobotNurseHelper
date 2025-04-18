#include "ThreadTablet.hpp"
#include "utility_TimeRecorder.hpp"
#include "utility_directory.hpp"
#include "utility_string.hpp"
#include "utility_csv.hpp"
#include "utility_directory.hpp"
#include "ServerSend.pb.h"

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
            switch( faceIndex )
            {
                case 0:
                    str_RobotSpeakSentence = "It is great that you are not painful.";
                    break;
                case 1:
                    str_RobotSpeakSentence = "I hope you still bear a little pain.";
                    break;
                case 2:
                    str_RobotSpeakSentence = "Hold on. It will pass soon.";
                    break;
                case 3:
                    str_RobotSpeakSentence = "Are you ok?";
                    break;
                case 4:
                    str_RobotSpeakSentence = "I am sorry to know that you are very fainful?";
                    break;
                case 5:
                    str_RobotSpeakSentence = "Do I need to call the doctor?";
                    break;
            }
            cout << "Pain rate: " << faceIndex *2 << std::endl;
            ZenboNurseHelperProtobuf::ReportAndCommand report_data;
            report_data.set_speak_sentence(str_RobotSpeakSentence);
            pThreadSendCommand->AddMessage(report_data);        
        }
        else
        {
            //wait until being notified
            std::unique_lock<std::mutex> lk(mtx);
            cond_var_process_image.wait(lk);
        }
    }
    cout << "Exit while loop." << std::endl;
}
