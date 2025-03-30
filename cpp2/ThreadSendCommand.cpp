#include "ThreadSendCommand.hpp"
#include <iostream>
#include <signal.h>

ThreadSendCommand::ThreadSendCommand()
{
    b_KeepLoop = true;
}

void ThreadSendCommand::run()
{
    mutex m;
    unique_lock<mutex> lock(m);

    //supress the SIGPIPE signal, sometimes the socket may be closed by the robot
    //and the write command will cause a SIGPIPE signal.
    //This signal will cause the program to exit.
    //The only way to ignore this signal is to set the signal handler to SIG_IGN.
    //This is a global signal handler, so it will affect all threads.
    //It is not a good idea to set the signal handler in a thread.
    //It is better to set it in the main thread.
    //But in this case, we have to set it here.
    ::signal(SIGPIPE, SIG_IGN);
    
    while(b_KeepLoop)
    {
        cond_var_report_result.wait(lock);
        mutex_message_buffer.lock();
        if( mQueue.size() > 0)
        {
            if( pSocket != NULL)          //socket can disconnect any time, it is in another thread
            {
                if( pSocket->isValid())
                {
                    QDataStream socketStream(pSocket);
                    ZenboNurseHelperProtobuf::ReportAndCommand message = mQueue.front();
                    mQueue.pop();
                    str_results_len = message.ByteSizeLong();
                    message.SerializeToArray(str_results,message.ByteSizeLong());

                    pSocket->write("BeginOfAMessage");
                    socketStream.writeRawData(str_results, str_results_len);
                    pSocket->write("EndOfAMessage");
                    //The robot may close the connection suddenly.
                    //pSocket->isValid cannot prevent it.
                    //The only way is to igoore the SIGPIPE signal.
                    pSocket->flush();       //This command is required to send out data in the buffer.
                }
            }
        }
        mutex_message_buffer.unlock();
    }
}

void ThreadSendCommand::AddMessage(ZenboNurseHelperProtobuf::ReportAndCommand message)
{
    mutex_message_buffer.lock();
    mQueue.push(message);
    mutex_message_buffer.unlock();
    cond_var_report_result.notify_one();
}
