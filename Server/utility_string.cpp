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
    string PunctuationMarks = "，。？：！「」";
    icu::UnicodeString ustr_input(input.c_str(), input.length(), "UTF-8");
    icu::UnicodeString ustr_PunctuationMarks(PunctuationMarks.c_str(), PunctuationMarks.length(), "UTF-8");

    std::regex chinese_punctuation_regex("\\p{P}", std::regex::ECMAScript | std::regex::icase | std::regex::collate);

    // Replace all matches of the pattern with an empty string
    std::string cleaned_text = std::regex_replace(input, chinese_punctuation_regex, "");

    std::cout << "Original string: " << input << std::endl;
    std::cout << "Cleaned string: " << cleaned_text << std::endl;
}