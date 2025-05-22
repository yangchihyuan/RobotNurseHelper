#include "JPEG.hpp"
#include <fstream>

//save JPEG data into a file
void save_image_JPEG(char* data_, std::size_t length, std::string filename)
{
    std::ofstream outfile( filename,std::ofstream::binary);
    outfile.write(data_, length);
    outfile.close();
}

JPEG_buffer::JPEG_buffer()
{
    data = nullptr;
    length = 0;
}

JPEG_buffer::~JPEG_buffer()
{
    if(data != nullptr)
    {
        delete[] data;
    }
}

void JPEG_buffer::set_buffer(char* data_, std::size_t length)
{
    if(data != nullptr)
    {
        delete[] data;
    }
    data = new char[length];
    std::copy(data_, data_ + length, data);
    length = length;
}

char* JPEG_buffer::get_buffer()
{
    return data;
}

std::size_t JPEG_buffer::get_length()
{
    return length;
}
