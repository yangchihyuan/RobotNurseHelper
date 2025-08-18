#include "utility_string.hpp"
#include <fstream>
#include <unicode/unistr.h>
#include <regex> // C++11 standard library regex
#include <iostream>

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

int GetChineseCharacterNumberWithoutPunctuationMarks(string input)
{
    std::regex chinese_punctuation_regex("，|。|？|：|！|「|」");
    std::string cleaned_text = std::regex_replace(input, chinese_punctuation_regex, "");

    //debug
    //std::cout << "Original string: " << input << std::endl;
    //std::cout << "Cleaned string: " << cleaned_text << std::endl;

    int char_count = 0;
    for (size_t i = 0; i < cleaned_text.length(); ++i) {
        if ((cleaned_text[i] & 0xC0) != 0x80) {
            char_count++;
        }
    }

    return char_count;
}