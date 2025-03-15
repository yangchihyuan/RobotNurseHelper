#include "utility_string.hpp"
#include <fstream>

string RemoveFileExtension(string fullname)
{
    size_t lastindex = fullname.find_last_of("."); 
    string rawname = fullname.substr(0, lastindex); 
    return rawname;
}

vector<string> LoadFileList(string filelist_path)
{
    string line;
    vector<string> returned_vector;
    ifstream out(filelist_path);
    while(getline(out, line)) {
        returned_vector.push_back(line);
    }
    out.close();
    return returned_vector;
}