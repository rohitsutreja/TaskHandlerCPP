#pragma once

#include<string>
#include<chrono>

inline std::string getLogTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm currentTime;
    localtime_s(&currentTime, &time);
    char date_buffer[26];
    std::strftime(date_buffer, sizeof(date_buffer), "%m/%d/%y %H:%M:%S", &currentTime);

    return date_buffer;
}