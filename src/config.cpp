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
}
