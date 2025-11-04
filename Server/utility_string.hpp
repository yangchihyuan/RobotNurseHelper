#include <string>
#include <vector>
#include <filesystem>

using namespace std;

string RemoveFileExtension(string fullname);
vector<string> LoadFileList(string filelist_path);
int GetChineseCharacterNumberWithoutPunctuationMarks(string input);
string LoadFileToString(string filepath);