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
#include <condition_variable>
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
    void set_delimiter(string delimiter_head, string delimiter_tail);
    string get_delimiter_head();
    string get_delimiter_tail();
    void set_buffer(char* data_, size_t length);
    char* get_buffer();
    size_t get_buffer_length();
    size_t get_buffer_size();
    ThreadSafeQueue<DataFrame> *pDataFrames_queue = nullptr;
    condition_variable *pNofitiedCondVar = nullptr;
protected:
    unique_ptr<char[]> buffer;
    size_t buffer_length = 0;       //buffer_length is the length of the data in buffer
    string delimiter_head = "BeginOfADataFrame";
    string delimiter_tail = "EndOfADataFrame";
    size_t buffer_size = 0;
};

#endif