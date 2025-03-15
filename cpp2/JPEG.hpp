#include <string>
using namespace std;

void save_image_JPEG(char* data_, size_t length, string filename);

class JPEG_buffer
{
public:
    JPEG_buffer();
    ~JPEG_buffer();
    void set_buffer(char* data_, size_t length);
    char* get_buffer();
    size_t get_length();
private:
    char* data;
    size_t length;
};
