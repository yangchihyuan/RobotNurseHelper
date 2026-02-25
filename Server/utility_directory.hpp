#include <vector>
#include <string>
#include <iostream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

//return 1 if successful, -1 if failed
int ListFiles(const string& directory, const string& file_extension, vector<string>& file_list);

//not sorted
vector<string> ListFiles(const string& directory, const string& file_extension );
vector<string> ListFiles_Sorted(const string& directory, const string& file_extension);
void CreateDirectory(const string& directory);
bool CheckDirectoryExist(const string& directory);
string ReplaceShellVariable(const string& path);
