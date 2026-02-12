#ifndef __UTILITY_TIME_HPP__
#define __UTILITY_TIME_HPP__

#include <string>
#include "google/protobuf/timestamp.pb.h"
using namespace std;

string GetCurrentTimeString(bool bMillisecond = true);
chrono::time_point<chrono::system_clock> 
protobufTimestampToTimePoint(const google::protobuf::Timestamp& ts);

string ConvertTimeToString(chrono::time_point<chrono::system_clock> chrono_time, bool bMillisecond = true);
string ConvertProtobufTimestampToString(const google::protobuf::Timestamp& ts, bool bMillisecond);

#endif