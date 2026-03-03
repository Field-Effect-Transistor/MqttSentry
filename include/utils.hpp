//  include/utils.hpp
#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <cstdio>
#include <ctime>

//  splits a string into tokens using a separator character
inline std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

inline std::string formatTime(const unsigned long seconds) {
    auto s = seconds % 60;
    auto m = seconds / 60 % 60;
    auto h = seconds / 60 / 60;
    char buf[32];
    snprintf(buf, sizeof(buf), "%luг %luхв %luс", h, m, s);
    return std::string(buf);
}

inline uint64_t stringToUnix(const std::string& timestamp) {
    struct tm t = {0};
    int year, month, day, hour, min, sec;
                                //"2026-02-26 14:04:42"
    if (sscanf(timestamp.c_str(), "%d-%d-%d %d:%d:%d", 
               &year, &month, &day, &hour, &min, &sec) == 6) 
    {
        t.tm_year = year - 1900; // Роки відраховуються від 1900s
        t.tm_mon = month - 1;    // Місяці 0-11
        t.tm_mday = day;
        t.tm_hour = hour;
        t.tm_min = min;
        t.tm_sec = sec;
        t.tm_isdst = -1;         // Автоматичне визначення літнього часу

        return static_cast<uint64_t>(mktime(&t));
    }
    return 0;
}
