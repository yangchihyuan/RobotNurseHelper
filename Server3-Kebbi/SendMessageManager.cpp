#include "SendMessageManager.hpp"
#include <iostream>
#include <signal.h>

void SendMessageManager::Send()
{
    //supress the SIGPIPE signal, sometimes the socket may be closed by the robot
    //and the write command will cause a SIGPIPE signal.
    //This signal will cause the program to exit.
    //The only way to ignore this signal is to set the signal handler to SIG_IGN.
    //This is a global signal handler, so it will affect all threads.
    //It is not a good idea to set the signal handler in a thread.
    //It is better to set it in the main thread.
    //But in this case, we have to set it here.
    ::signal(SIGPIPE, SIG_IGN);
    
    if( mQueue.size() > 0)
    {
        if( pSocket != NULL)          //socket can disconnect any time, it is in another thread
        {
            if( pSocket->isValid())
            {
                QDataStream socketStream(pSocket);
                RobotCommandProtobuf::KebbiCommand message = mQueue.front();
                mQueue.pop();
                str_results_len = message.ByteSizeLong();
                message.SerializeToArray(str_results,message.ByteSizeLong());

                pSocket->write("BeginOfAMessage");
                socketStream.writeRawData(str_results, str_results_len);
                pSocket->write("EndOfAMessage");
                cout << "send a message " << endl;

                //The robot may close the connection suddenly.
                //pSocket->isValid cannot prevent it.
                //The only way is to igoore the SIGPIPE signal.
                pSocket->flush();       //This command is required to send out data in the buffer.
            }
            else
            {
                cout << "Socket is not valid" << endl;
            }
        }
        else
        {
            cout << "Socket is NULL" << endl;
        }
    }
}

void SendMessageManager::AddMessage(RobotCommandProtobuf::KebbiCommand message)
{
    mutex_message_buffer.lock();
    cout << "Add a Message to mQueue" << endl;
    mQueue.push(message);
    mutex_message_buffer.unlock();
}
