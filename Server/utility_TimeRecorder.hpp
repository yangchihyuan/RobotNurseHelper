#include <string>
#include <chrono>

using namespace std;
using namespace std::chrono;

#ifndef UTILITY_TIMERECORDER_HPP
#define UTILITY_TIMERECORDER_HPP

class TimeRecorder
{
    public:
    TimeRecorder();
    void Start();
    void Stop();
    string GetDurationString();

    private:
    system_clock::time_point time_detection_start;    //inluded in <chrono>
    system_clock::time_point time_detection_end;
};

#endif