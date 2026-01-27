//  src/config.cpp
#include "config.hpp"

using json = nlohmann::json;

namespace Settings {
    ConfigManager::ConfigManager(const std::string& configFileName):
    _configFileName(configFileName) {
        _load();        
    }

    void to_json(json& j, const tg& t) {
        j = json{
            {"token", t.token},
            {"users", t.users}
        };
    }

    //  left with unresolved possible exception on purpose.
    //  program should NOT run without token
    void from_json(const json& j, tg& t) {
        t.token = j["token"];
        t.users = j.value("users", std::vector<std::string>{});
    }

    void to_json(json& j, const mqtt& m) {
        j = json{
            {"broker", m.broker},
            {"port", m.port},
            {"client_id", m.client_id},
            {"client_name", m.client_name},
            {"pwd", m.pwd},
            {"topic", m.topic}
        };
    }
    void from_json(const json& j, mqtt& m) {
        if (j.contains("broker")) {
            j.at("broker").get_to(m.broker);
        }
        if (j.contains("port")) {
            j.at("port").get_to(m.port);
        }
        if (j.contains("client_id")) {
            j.at("client_id").get_to(m.client_id);
        }
        if (j.contains("client_name")) {
            j.at("client_name").get_to(m.client_name);
        }
        if (j.contains("pwd")) {
            j.at("pwd").get_to(m.pwd);
        }
        if (j.contains("topic")) {
            j.at("topic").get_to(m.topic);
        }
    }

    bool ConfigManager::_load() {
        if (_loaded) {
            return true;
        }

        std::ifstream configFile(_configFileName);

        if (!configFile.is_open() || configFile.peek() == std::ifstream::traits_type::eof()) {
            std::cout << "Info: Config file not found or empty. Creating a default one." << std::endl;
            bool writeSuccess = _write();
            
            _loaded = writeSuccess;
            return writeSuccess;
        }

        try {
            json data = json::parse(configFile);

            if (data.contains("tg")) {
                _tg = data.at("tg").get<tg>();
            }
            if (data.contains("mqtt")) {
                _mqtt = data.at("mqtt").get<mqtt>();
            }

        } catch (const json::parse_error& e) {
            std::cerr << "[ERROR] Error parsing config file: " << e.what() << std::endl;
            return false;
        }

        _loaded = true;
        return true;
    }

    bool ConfigManager::_write() {
        std::ofstream configFile(_configFileName);
        if (!configFile.is_open()) {
            std::cerr << "[ERROR] Could not open config file for writing: " << _configFileName << std::endl;
            return false;
        }

        json data;
        data["tg"] = _tg;
        data["mqtt"] = _mqtt;

        configFile << data.dump(4);
        
        if (configFile.fail()) {
            std::cerr << "[ERROR] Failed to write to config file: " << _configFileName << std::endl;
            return false;
        }

        std::cout << "Info: Configuration saved to " << _configFileName << std::endl;
        return true;
    }

    template<typename U>
    bool ConfigManager::update(const std::string& key, const U& value) {
        bool updated = false;

        //  TELEGRAM
        if (key == "tg.token") {
            if constexpr (std::is_same_v<U, decltype(_tg.token)>) {
                _tg.token = value;
                updated = true;
            }
        } else if (key == "tg.users") {
            if constexpr (std::is_same_v<U, decltype(_tg.users)>) {
                _tg.users = value;
                updated = true;
            }
        } else

        //  MQTT
        if (key == "mqtt.broker") {
            if constexpr (std::is_same_v<U, decltype(_mqtt.broker)>) {
                _mqtt.broker = value;
                updated = true;
            }
        } else if (key == "mqtt.port") {
            if constexpr (std::is_convertible_v<U, decltype(_mqtt.port)>) {
                _mqtt.port = value;
                updated = true;
            }
        } else if (key == "mqtt.client_id") {
            if constexpr (std::is_same_v<U, decltype(_mqtt.client_id)>) {
                _mqtt.client_id = value;
                updated = true;
            }
        } else if (key == "mqtt.client_name") {
            if constexpr (std::is_same_v<U, decltype(_mqtt.client_name)>) {
                _mqtt.client_name = value;
                updated = true;
            }
        } else if (key == "mqtt.pwd") {
            if constexpr (std::is_same_v<U, decltype(_mqtt.pwd)>) {
                _mqtt.pwd = value;
                updated = true;
            }
        } else if (key == "mqtt.topic") {
            if constexpr (std::is_same_v<U, decltype(_mqtt.topic)>) {
                _mqtt.topic = value;
                updated = true;
            }
        }

        if (updated) {
            std::cout << "Info: value of " << key << "was updated\n";
            return _write();
        } else {
            std::cerr << "[ERROR] failed to update value of " << key;
            return false;
        }
    }
    template bool ConfigManager::update<std::string>(const std::string&, const std::string&);
    template bool ConfigManager::update<const char*>(const std::string&, const char* const&);
    template bool ConfigManager::update<int>(const std::string&, const int&);
    template bool ConfigManager::update<unsigned short>(const std::string&, const unsigned short&);
    template bool ConfigManager::update<std::vector<std::string>>(const std::string&, const std::vector<std::string>&);

    template<typename U>
    std::optional<U> ConfigManager::get(const std::string& key) {
        //  TELEGRAM
        if (key == "tg.token") {
            if constexpr (std::is_same_v<U, decltype(_tg.token)>) {
                return _tg.token;
            }
        } else if ( key == "tg.users") {
            if constexpr (std::is_same_v<U, decltype(_tg.users)>) {
                return _tg.users;
            }
        } else 

        //  MQTT
        if (key == "mqtt.broker") {
            if constexpr(std::is_same_v<U, decltype(_mqtt.broker)>) {
                return _mqtt.broker;
            }
        } else if (key == "mqtt.port") {
            if constexpr (std::is_convertible_v<U, decltype(_mqtt.port)>) {
                return _mqtt.port;
            }
        } else if (key == "mqtt.client_id") {
            if constexpr (std::is_same_v<U, decltype(_mqtt.client_id)>) {
                return _mqtt.client_id;
            }
        } else if (key == "mqtt.client_name") {
            if constexpr (std::is_same_v<U, decltype(_mqtt.client_name)>) {
                return _mqtt.client_name;
            }
        } else if (key == "mqtt.pwd") {
            if constexpr (std::is_same_v<U, decltype(_mqtt.pwd)>) {
                return _mqtt.pwd;
            }
        } else if (key == "mqtt.topic") {
            if constexpr (std::is_same_v<U, decltype(_mqtt.topic)>) {
                return _mqtt.topic;
            }
        }

        return std::nullopt;
    }
    template std::optional<std::string> ConfigManager::get<std::string>(const std::string&);
    template std::optional<unsigned short> ConfigManager::get<unsigned short>(const std::string&);
    template std::optional<std::vector<std::string>> ConfigManager::get<std::vector<std::string>>(const std::string&);


    bool ConfigManager::userExist(const std::string& u) {
        auto v = *get<std::vector<std::string>>("tg.users");
        for (auto it = v.begin(); it != v.end(); ++it) {
            if (*it == u) {
                return true;
            }
        }
        return false;
    }

    bool ConfigManager::addUser(const std::string& u) {
        auto v = *get<std::vector<std::string>>("tg.users");
        bool exist = false;
        for (auto it = v.begin(); it != v.end(); ++it) {
            if(*it == u) {
                exist = true;
                break;
            }
        }

        if(!exist) {
            v.push_back(u);
            return update("tg.users", v);
        }

        return true;
    }

    bool ConfigManager::removeUser(const std::string& u) {
        auto v = *get<std::vector<std::string>>("tg.users");
        if (v.empty()) {
            return true;
        } else {
            std::vector<std::string> temp;
            for(auto it =  v.begin(); it != v.end(); ++it) {
                if (*it != u) {
                    temp.push_back(*it);
                }
            }
            return update("tg.users", temp);
        }
    }
}

