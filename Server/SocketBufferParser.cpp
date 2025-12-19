#include "SocketBufferParser.hpp"
#include "ThreadProcessImage.hpp"
#include "ThreadReceiveMessage.hpp"

#define _DEFAULT_BUFFER_SIZE 5000000
#include <iostream>  // for cout and endl
#include <string>  // for string
#include <memory>  // for unique_ptr
#include <queue>  // for queue
#include <algorithm>  // for copy
#include <string.h>   // for memcpy
using namespace std;

SocketBufferParser::SocketBufferParser()
{
    buffer_size = _DEFAULT_BUFFER_SIZE;
    buffer = make_unique<char[]>(buffer_size);
}

SocketBufferParser::SocketBufferParser(size_t buffer_size)
{
    buffer = make_unique<char[]>(buffer_size);
}

SocketBufferParser::SocketBufferParser(string delimiter_head, string delimiter_tail)
{
    buffer_size = _DEFAULT_BUFFER_SIZE;
    buffer = make_unique<char[]>(buffer_size);
    this->delimiter_head = delimiter_head;
    this->delimiter_tail = delimiter_tail;
}

SocketBufferParser::~SocketBufferParser()
{
}

void SocketBufferParser::set_delimiter(string delimiter_head, string delimiter_tail)
{
    this->delimiter_head = delimiter_head;
    this->delimiter_tail = delimiter_tail;
}

void SocketBufferParser::add_data(char* data_, size_t length)
{
    size_t buffer_length_old = buffer_length;

    //copy data_ to buffer
    //whisper.cpp pauses the thread, as a result, the length may exceed the buffer size.
    if( buffer_length + length > buffer_size)
    {
        cout << "Buffer size is not enough. Drop out the old data." << endl;
        //2025/3/30, I need to handle here later
        buffer_length = 0;
    }
    else
    {
        copy(data_, data_+length, buffer.get()+buffer_length);
        buffer_length += length;
    }

    //find delimiter_tail in buffer
    int delimiter_tail_length = delimiter_tail.length();
    
    string buffer_section;      //the section of buffer to search for delimiter_tail
    int begin_pos = 0;
    if(buffer_length_old >= delimiter_tail_length)
    {
        begin_pos = buffer_length_old - delimiter_tail_length;
        buffer_section.assign(buffer.get() + buffer_length_old - delimiter_tail_length, length + delimiter_tail_length); 
    }
    else
        buffer_section.assign(buffer.get(), buffer_length); 

    bool bloop_find = true;
    while( bloop_find )
    {
        size_t n = buffer_section.find(delimiter_tail);

        //if found, check the delimiter_head and prefixed length
        if (n != string::npos)
        {
            //length1 means the length between two delimiter_tail, including the delimiter_tail.
            int length1 = begin_pos + n + delimiter_tail.length();

            //check the delimiter_head
            string buffer_section_head(buffer.get(), delimiter_head.length());
            if( buffer_section_head != delimiter_head)
            {
                cout << "Received Delimiter_head: " << buffer_section_head << endl;
                cout << "Delimiter head is incorrect. Drop out this DataFrame" << endl;
                continue;
            }
            else{
                //check the DataFrame length
                int DataFrame_length;
                memcpy(&DataFrame_length, buffer.get() + delimiter_head.length(), sizeof(int));       //here is wrong, why?
                int length2 = length1 - delimiter_head.length() - delimiter_tail.length();
                if( DataFrame_length != length2 - sizeof(int))
                {
                    cout << "DataFrame_length is incorrect. Drop out this DataFrame" << endl;
                    cout << "length1: " <<  length1 << endl;
                    cout << "length2: " <<  length2 << endl;
                    cout << "DataFrame_length: " <<  DataFrame_length << endl;
                    //On an emulator, there will no WiFi error. But on a real robot, there will be WiFi error.
                    //I need to skip this frame.
                }
                else
                {
                    //copy buffer to queue
                    DataFrame this_DataFrame;
                    this_DataFrame.data = shared_ptr<char[]>(new char[DataFrame_length]);
                    this_DataFrame.length = DataFrame_length;
                    memcpy(this_DataFrame.data.get(), buffer.get()+delimiter_head.length()+sizeof(int), DataFrame_length);
                    pDataFrames_queue->push(this_DataFrame);
                    if( pNofitiedCondVar != nullptr )
                        pNofitiedCondVar->notify_one();
                }
            }
 
            //update buffer_length
            buffer_length -= length1;

            //move the remaining data to the beginning of the buffer
            if( buffer_length > 0)
                copy(buffer.get()+length1,
                     buffer.get()+length1+buffer_length,
                     buffer.get());
            
            //update buffer_section and begin_pos
            buffer_section.assign(buffer.get(), buffer_length);
            begin_pos = 0;
        }
        else
        {
            bloop_find = false;
        }
    }

}

string SocketBufferParser::get_delimiter_head()
{
    return delimiter_head;
}

string SocketBufferParser::get_delimiter_tail()
{
    return delimiter_tail;
}

void SocketBufferParser::set_buffer(char* data_, size_t length)
{
    copy(data_, data_+length, buffer.get());
    buffer_length = length;
}

char* SocketBufferParser::get_buffer()
{
    return buffer.get();
}

size_t SocketBufferParser::get_buffer_length()
{
    return buffer_length;
}

size_t SocketBufferParser::get_buffer_size()
{
    return buffer_size;
}