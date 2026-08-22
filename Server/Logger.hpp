#pragma once

#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace std;

class Logger
{
public:
    static inline string getCurrentDateTime(string s)
    {
        const auto now = chrono::system_clock::now();
        const auto milliseconds = chrono::duration_cast<chrono::milliseconds>(
                                      now.time_since_epoch()) %
                                  1000;
        const time_t timeNow = chrono::system_clock::to_time_t(now);
        struct tm tstruct;
        char buf[80];
        tstruct = *localtime(&timeNow);
        if (s == "now")
        {
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tstruct);
            ostringstream result;
            result << buf << '.' << setfill('0') << setw(3)
                   << milliseconds.count();
            return result.str();
        }
        else if (s == "date")
            strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);
        return string(buf);
    };

    void SetLogDirectory(string sLogDirectory)
    {
        m_slogDirectory = sLogDirectory;
    }

    void SetLogFileName(string sLogFileName)
    {
        m_slogFileName = sLogFileName;
    }

    void LogToFile(string logMsg)
    {
        string now = getCurrentDateTime("now");
        ofstream ofs(m_slogDirectory + "/" + m_slogFileName, std::ios_base::out | std::ios_base::app);
        ofs << now << '\t' << logMsg << '\n';
        ofs.close();
    }

protected:
    string m_slogDirectory;
    string m_slogFileName;
};
