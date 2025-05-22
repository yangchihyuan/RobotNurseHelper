#include <string>
#include <memory>  // for unique_ptr
#include <queue>
#include <mutex>
using namespace std;

#ifndef SocketHandler_hpp
#define SocketHandler_hpp

struct Message
{
    shared_ptr<char[]> data;
    size_t length;
};

class SocketHandler
{
public:
    SocketHandler();
    SocketHandler(size_t buffer_size);
    SocketHandler(string delimiter_head, string delimiter_tail);
    ~SocketHandler();
    void add_data(char* data_, size_t length);
    size_t get_queue_length();
    Message get_head();
    void pop_head();
    void clear_queue();
    void set_delimiter(string delimiter_head, string delimiter_tail);
    string get_delimiter_head();
    string get_delimiter_tail();
    void set_buffer(char* data_, size_t length);
    char* get_buffer();
    size_t get_buffer_length();
    size_t get_buffer_size();
    queue<Message> messages_queue;
private:
    unique_ptr<char[]> buffer;
    size_t buffer_length = 0;       //buffer_length is the length of the data in buffer
    string delimiter_head = "Begin:";
    string delimiter_tail = "EndOfAFrame";
    size_t buffer_size = 0;
    mutex queue_mutex;
};

#endif