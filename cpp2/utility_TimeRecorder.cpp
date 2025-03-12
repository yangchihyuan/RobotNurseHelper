#include "utility_TimeRecorder.hpp"

TimeRecorder::TimeRecorder(){
    time_detection_start = system_clock::now();
}

void TimeRecorder::Stop(){
    time_detection_end = system_clock::now();
}

string TimeRecorder::GetDurationString()
{
    auto duration_for_detection = duration_cast<milliseconds>(time_detection_end - time_detection_start);
    string str = to_string(duration_for_detection.count());
    return str;
}
