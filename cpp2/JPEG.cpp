#include "JPEG.hpp"
#include <fstream>

//save JPEG data into a file
void save_image_JPEG(char* data_, std::size_t length, std::string filename)
{
    std::ofstream outfile( filename,std::ofstream::binary);
    outfile.write(data_, length);
    outfile.close();
}
