#include "ThreadTablet.hpp"
#include "utility_TimeRecorder.hpp"
#include "utility_directory.hpp"
#include "utility_string.hpp"
#include "utility_csv.hpp"
#include "utility_directory.hpp"

void ThreadTablet::run()
{
    std::string str_home_path(getenv("HOME"));

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
            cout << "Pain rate: " << faceIndex *2 << std::endl;
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
