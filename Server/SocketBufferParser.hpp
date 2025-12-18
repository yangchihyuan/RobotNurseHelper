//2025/8/19 This class handle the DataFrames tranmitted from a TCP socket.
//It detects the delimiter strings in the socket. Once it finds the head and tail delimiter strings,
//it gets the first 4 bytes as the length to do a basic check because head and tail delimiter strings
//may occur in the DataFrame content even though the probablity is low.
//Thus, the sender (robot) needs to follow the same protocol.

#ifndef SocketBufferParser_HPP
#define SocketBufferParser_HPP

#include <string>
#include <memory>  // for unique_ptr
#include <queue>
#include <mutex>
//#include "ThreadProcessImage.hpp"
#include "ThreadSafeQueue.hpp"
using namespace std;

class ThreadProcessImage;
class ThreadReceiveMessage;

struct DataFrame
{
    shared_ptr<char[]> data;
    size_t length;
};

class SocketBufferParser
{
public:
    SocketBufferParser();
    SocketBufferParser(size_t buffer_size);
    SocketBufferParser(string delimiter_head, string delimiter_tail);
    ~SocketBufferParser();
    void add_data(char* data_, size_t length);
//    size_t get_queue_length();
//    DataFrame get_head();
//    void pop_head();
//    void clear_queue();
    void set_delimiter(string delimiter_head, string delimiter_tail);
    string get_delimiter_head();
    string get_delimiter_tail();
    void set_buffer(char* data_, size_t length);
    char* get_buffer();
    size_t get_buffer_length();
    size_t get_buffer_size();
    ThreadSafeQueue<DataFrame> *pDataFrames_queue = nullptr;      
protected:
    unique_ptr<char[]> buffer;
    size_t buffer_length = 0;       //buffer_length is the length of the data in buffer
    string delimiter_head = "BeginOfADataFrame";
    string delimiter_tail = "EndOfADataFrame";
    size_t buffer_size = 0;
//    mutex queue_mutex;

    virtual void notify_thread();
};

class SocketBufferParser_Image : public SocketBufferParser
{
public:
    SocketBufferParser_Image();
    virtual ~SocketBufferParser_Image();

    virtual void notify_thread();

    ThreadProcessImage* thread_process_image = nullptr;
};

class SocketBufferParser_Message : public SocketBufferParser
{
public:
    SocketBufferParser_Message();
    virtual ~SocketBufferParser_Message();

    virtual void notify_thread();

    ThreadReceiveMessage* thread_receive_message = nullptr;
};

#endif