//  include/utils.hpp
#pragma once

#include <vector>
#include <string>
#include <sstream>

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
