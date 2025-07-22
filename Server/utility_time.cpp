#include "utility_time.hpp"
#include <chrono>   // For std::chrono
#include <ctime>    // For std::localtime
#include <iomanip>  // For std::put_time and stream manipulators

std::string GetCurrentTimeString(bool bMillisecond)
{
    // 1. Get the current time point with high precision
    auto now = std::chrono::system_clock::now();

    // 2. Convert to time_t for std::localtime (whole seconds part)
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    // 3. Convert to tm structure (for use with strftime)
    std::tm* ptm = std::localtime(&now_c); // Note: std::localtime is not thread-safe!

    // 4. Extract milliseconds (fractional part)
    auto duration_since_epoch = now.time_since_epoch();
    auto seconds_duration = std::chrono::duration_cast<std::chrono::seconds>(duration_since_epoch);
    auto milliseconds_remaining = std::chrono::duration_cast<std::chrono::milliseconds>(duration_since_epoch - seconds_duration);

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