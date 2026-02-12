#include "utility_time.hpp"
#include <chrono>   // For std::chrono
#include <ctime>    // For std::localtime
#include <iomanip>  // For std::put_time and stream manipulators

string GetCurrentTimeString(bool bMillisecond)
{
    // 1. Get the current time point with high precision
    auto now = chrono::system_clock::now();

    // 2. Convert to time_t for std::localtime (whole seconds part)
    time_t now_c = chrono::system_clock::to_time_t(now);

    // 3. Convert to tm structure (for use with strftime)
    tm* ptm = localtime(&now_c); // Note: std::localtime is not thread-safe!

    // 4. Extract milliseconds (fractional part)
    auto duration_since_epoch = now.time_since_epoch();
    auto seconds_duration = chrono::duration_cast<chrono::seconds>(duration_since_epoch);
    auto milliseconds_remaining = chrono::duration_cast<chrono::milliseconds>(duration_since_epoch - seconds_duration);

    // Create a buffer for strftime output
    char buffer[80]; // Choose an appropriate size for your format

    // Use strftime for the date and time up to seconds
    // Example format: YYYY-MM-DD HH:MM:SS
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H_%M_%S", ptm);     //Windows does not allow a filename to have a colon.

    // Combine strftime output with milliseconds
    std::ostringstream oss;
    oss << buffer;
    if( bMillisecond)
        oss << "_" << std::setfill('0') << std::setw(3) << milliseconds_remaining.count();

    return oss.str();    
}

// Function to convert google::protobuf::Timestamp to std::chrono::time_point
chrono::time_point<chrono::system_clock> 
protobufTimestampToTimePoint(const google::protobuf::Timestamp& ts) {
    auto d = std::chrono::seconds{ts.seconds()} + std::chrono::nanoseconds{ts.nanos()};
    return std::chrono::time_point<std::chrono::system_clock>{d};
}

//Convert chrono system_time to string
string ConvertTimeToString(chrono::time_point<chrono::system_clock> chrono_time, bool bMillisecond)
{
    // Convert time_point to std::time_t for date/time formatting
    time_t time_t_now = chrono::system_clock::to_time_t(chrono_time);
    
    // Get the milliseconds component
    auto duration_in_ms = chrono::duration_cast<chrono::milliseconds>(chrono_time.time_since_epoch());
    
    // Use stringstream to format the output
    tm timeinfo;
    std::stringstream ss;
    ss << std::put_time(localtime_r(&time_t_now, &timeinfo), "%Y-%m-%d %H:%M:%S");
    
    // Append the milliseconds to the string
    if( bMillisecond)
    {
        long long milliseconds = duration_in_ms.count() % 1000;
        ss << "." << std::setfill('0') << std::setw(3) << milliseconds;
    }
    
    return ss.str();
}

//Convert protobuf Timestamp to string
string ConvertProtobufTimestampToString(const google::protobuf::Timestamp& ts, bool bMillisecond)
{
    auto chrono_time = protobufTimestampToTimePoint(ts);
    return ConvertTimeToString(chrono_time, bMillisecond);
}