#include "utility_directory.hpp"

#include <dirent.h>
#include <string.h>  //this is a C head file for strlen()
#include <sys/stat.h>       //for S_IRWXU

int ListFiles(const string& directory, const string& file_extension, vector<string>& file_list)
{
    DIR *dir;
    struct dirent *ent;
    int ext_length = file_extension.length();
    if ((dir = opendir(directory.c_str())) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != NULL) {
            int string_length = strlen(ent->d_name);
            if(string_length > ext_length)
                if( strcmp(ent->d_name+string_length-ext_length, file_extension.c_str()) == 0)  //==0 means equals
                { 
                    file_list.push_back(string(ent->d_name));       //ent->d_name is char[256]
                }
        }
        closedir (dir);
        return 1;
    } else {
        /* could not open directory */
        return -1;
    }
}

vector<string> ListFiles(const string& directory, const string& file_extension )
{
    DIR *dir;
    struct dirent *ent;
    int ext_length = file_extension.length();
    vector<string> file_list;
    if ((dir = opendir(directory.c_str())) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != NULL) {
            int string_length = strlen(ent->d_name);
            if(string_length > ext_length)
                if( strcmp(ent->d_name+string_length-ext_length, file_extension.c_str()) == 0)  //==0 means equals
                { 
                    file_list.push_back(string(ent->d_name));       //ent->d_name is char[256]
                }
        }
        closedir (dir);
    } else {
        /* could not open directory */
    }
    return file_list;
}

vector<string> ListFiles_Sorted(const string& directory, const string& file_extension )
{
    struct dirent **entry_list;
    int count;
    int i;    
    int ext_length = file_extension.length();
    vector<string> file_list;
    count = scandir(directory.c_str(), &entry_list, 0, alphasort);
    if (count < 0) {
        perror("scandir");
    }

    for (i = 0; i < count; i++) {
        struct dirent *entry;

        entry = entry_list[i];
        int string_length = strlen(entry->d_name);
        if(string_length > ext_length)
            if( strcmp(entry->d_name+string_length-ext_length, file_extension.c_str()) == 0)  //==0 means equals
            { 
                file_list.push_back(string(entry->d_name));       //ent->d_name is char[256]
            }
        free(entry);
    }
/* 
    free(entry_list);
    if ((dir = opendir()) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            int string_length = strlen(ent->d_name);
            if(string_length > ext_length)
                if( strcmp(ent->d_name+string_length-ext_length, file_extension.c_str()) == 0)  //==0 means equals
                { 
                    file_list.push_back(string(ent->d_name));       //ent->d_name is char[256]
                }
        }
        closedir (dir);
    } else {
    }
    */
    return file_list;
}

//recursive mkdir
static void _mkdir(const char *dir) {
        char tmp[256];
        char *p = NULL;
        size_t len;

        snprintf(tmp, sizeof(tmp),"%s",dir);
        len = strlen(tmp);
        if(tmp[len - 1] == '/')
                tmp[len - 1] = 0;
        for(p = tmp + 1; *p; p++)
                if(*p == '/') {
                        *p = 0;
                        mkdir(tmp, S_IRWXU);
                        *p = '/';
                }
        mkdir(tmp, S_IRWXU);
}

void CreateDirectory(const string& directory)
{
    _mkdir(directory.c_str());
}

bool CheckDirectoryExist(const string& directory)
{
    DIR* dir = opendir(directory.c_str());
    if (dir)
    {
        // Directory exists. 
        closedir(dir);
        return true;
    }
//    else if (ENOENT == errno)
//    {
//        /// Directory does not exist.
//        mkdir(directory.c_str(), S_IRWXU|S_IRWXG);
//    }
    return false;
}