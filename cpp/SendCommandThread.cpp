#include "SendCommandThread.hpp"
#include <iostream>


SendCommandThread::SendCommandThread()
{
    b_KeepLoop = true;
}

void SendCommandThread::run()
{
    mutex m;
    unique_lock<mutex> lock(m);
    
    while(b_KeepLoop)
    {
        cond_var_report_result.wait(lock);
        mutex_message_buffer.lock();
        if( mQueue.size() > 0)
        {
            if( pSocket != NULL)          //socket can disconnect any time, it is in another thread
            {
                if( pSocket->isValid())   //After disconnection, this statement causes a segmentation fault. why?
                {
                    QDataStream socketStream(pSocket);
                    ZenboNurseHelperProtobuf::ReportAndCommand message = mQueue.front();
                    mQueue.pop();
                    str_results_len = message.ByteSizeLong();
                    message.SerializeToArray(str_results,message.ByteSizeLong());

//                    socketStream.startTransaction();   //This statement may be removed later.
                    pSocket->write("BeginOfAMessage");
    //                int data_written = socketStream.writeRawData(str_results, str_results_len);
                    socketStream.writeRawData(str_results, str_results_len);
    //                string debug(str_results,str_results_len);
    //                std::cout << debug << std::endl;
                    pSocket->write("EndOfAMessage");
//                    socketStream.commitTransaction();
                    pSocket->flush();       //This command is required to send out data in the buffer.
                }
            }
        }
        mutex_message_buffer.unlock();
    }
}

void SendCommandThread::AddMessage(ZenboNurseHelperProtobuf::ReportAndCommand message)
{
    mutex_message_buffer.lock();
    mQueue.push(message);
    mutex_message_buffer.unlock();
    cond_var_report_result.notify_one();
}
