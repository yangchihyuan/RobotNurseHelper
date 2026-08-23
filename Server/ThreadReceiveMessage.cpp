#include "ThreadReceiveMessage.hpp"
#include "utility_time.hpp"

void ThreadReceiveMessage::run()
{
    while (b_WhileLoop)
    {
        std::unique_lock<std::mutex> lk(mtx);
        cond_var_receive_message.wait(lk);

        while (DataFrames_queue.size() > 0) // process all messages in the queue{
        {
            // Get message from the queue
            DataFrame dataframe; // = DataFrames_queue.front();
            DataFrames_queue.pop(dataframe);
            char *data_ = dataframe.data.get();
            size_t data_length = dataframe.length;

            // Here, I need to parse the protobuf object
            RobotCommandProtobuf::RobotToServerMessage RTSmessage;
            RTSmessage.ParseFromArray(data_, static_cast<int>(data_length));

            if (RTSmessage.has_description())
            {
                if (RTSmessage.description() == "onTTSComplete")
                {
                    // debug
                    {
                        //                        google::protobuf::Timestamp timestamp = RTSmessage.event_time();
                        //                        cout << "Receive Protobuf onTTSComplete signal, whose time is " << ConvertTimeToString(protobufTimestampToTimePoint(timestamp)) << endl;
                        //                        cout << "On received moment, system time is " << GetCurrentTimeString() << endl;
                        // They are not the same. It is difficult to control.
                        // System time may be smaller than the Protobuf time, which shoes that the two clock is not synchronized.
                    }
                    // notify ThreadStateControl
                    // 2025/8/20, The robot time is different from the server's time
                    // mpThreadStateControl->NotifyEvent("onTTSComplete", protobufTimestampToTimePoint(timestamp));
                    // I have to use the server's time.
                    mpThreadStateControl->NotifyEvent("onTTSComplete", chrono::system_clock::now());
                    mpWhisper->SkipCurrentSpeech(); // skip the current speech, because the robot has hear its own voice.
                    mpLogger->LogToFile("Receive onTTSComplete signal");
                }
                else if (RTSmessage.description() == "onCompleteOfMotionPlay")
                {
                    mpThreadProcessImage->NotifyEvent("onCompleteOfMotionPlay", chrono::system_clock::now(), RTSmessage.yaw(), RTSmessage.pitch());
                }
                else if (RTSmessage.description() == "onActivityRestart") // The mbtx activity complete
                {
                    // debug
                    cout << "(C) Receive onActivityRestart signal" << endl;
                    mpThreadStateControl->NotifyEvent("onActivityRestart", chrono::system_clock::now());
                }
            }

            if (RTSmessage.has_numberpressed())
            {
                int numberpressed = RTSmessage.numberpressed();

                std::string str_RobotSpeakSentence;
                int RobotExpressionIndex = 0;
                string sFace = "Unknown";
                cout << "Receive number " << numberpressed << endl;
                switch (numberpressed)
                {
                case 1:
                    str_RobotSpeakSentence = "一";
                    if (s_RobotModel == "Zenbo" || s_RobotModel == "ZenboJrII")
                        sFace = "ACTIVE";
                    else if (s_RobotModel == "Kebbi")
                        sFace = "TTS_JoyA";
                    else
                        cout << "Unknown robot model." << endl;
                    break;
                case 2:
                    str_RobotSpeakSentence = "二";
                    if (s_RobotModel == "Zenbo" || s_RobotModel == "ZenboJrII")
                        sFace = "AWARE_LEFT";
                    else if (s_RobotModel == "Kebbi")
                        sFace = "TTS_JoyB";
                    else
                        cout << "Unknown robot model." << endl;
                    break;
                case 3:
                    str_RobotSpeakSentence = "三";
                    if (s_RobotModel == "Zenbo" || s_RobotModel == "ZenboJrII")
                        sFace = "AWARE_RIGHT";
                    else if (s_RobotModel == "Kebbi")
                        sFace = "TTS_JoyC";
                    else
                        cout << "Unknown robot model." << endl;
                    break;
                case 4:
                    str_RobotSpeakSentence = "四";
                    if (s_RobotModel == "Zenbo" || s_RobotModel == "ZenboJrII")
                        sFace = "CONFIDENT";
                    else if (s_RobotModel == "Kebbi")
                        sFace = "TTS_SadnessA";
                    else
                        cout << "Unknown robot model." << endl;
                    break;
                case 5:
                    str_RobotSpeakSentence = "五";
                    if (s_RobotModel == "Zenbo" || s_RobotModel == "ZenboJrII")
                        sFace = "DOUBTING";
                    else if (s_RobotModel == "Kebbi")
                        sFace = "TTS_SadnessB";
                    else
                        cout << "Unknown robot model." << endl;
                    break;
                }
                RobotCommandProtobuf::RobotCommand robot_command;
                robot_command.set_speak_sentence(str_RobotSpeakSentence);
                robot_command.set_sface(sFace);
                pSendMessageManager->AddMessage(robot_command);
            }

            if (RTSmessage.has_tabletcommand())
            {
                std::string str_tabletcommand = RTSmessage.tabletcommand();
                int RobotExpressionIndex = 0;
                cout << "Receive tabletcommand: " << str_tabletcommand << endl;
                if (str_tabletcommand == "Restart")
                {
                    // Restart robot's control state.
                    mpThreadStateControl->Restart();
                }
                else if (str_tabletcommand == "Mandarin")
                {
                    // TODO: change language
                }
                else if (str_tabletcommand == "English")
                {
                    // TODO: change language
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
