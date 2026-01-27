//  include/dotenv.cpp

#include <iostream>

inline void set_environment_variable(const std::string& key, const std::string& value) {
#ifdef _WIN32
    _putenv_s(key.c_str(), value.c_str());
#else
    setenv(key.c_str(), value.c_str(), 0);
#endif
}

inline void load_env(std::istream& is) {
    std::string line;
    while (std::getline(is, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t delimiter_pos = line.find('=');
        if (delimiter_pos != std::string::npos) {
            std::string key = line.substr(0, delimiter_pos);
            std::string value = line.substr(delimiter_pos + 1);

            set_environment_variable(key, value);
        }
    }
}
