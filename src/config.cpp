//  src/config.cpp
#include "config.hpp"

#include <fstream>

#include "utils.hpp"

using json = nlohmann::json;

namespace Settings {
    ConfigManager::ConfigManager(const std::string& configFileName):
    _configFileName(configFileName) {
        _load();        
    }

    void to_json(json& j, const tg& t) {
        j = json{
            {"token", t.token},
            {"users", t.users},
            {"admins", t.admins}
        };
    }

    //  left with unresolved possible exception on purpose.
    //  program should NOT run without token
    void from_json(const json& j, tg& t) {
        t.token = j["token"];
        t.users = j.value("users", std::vector<uint64_t>{});
        t.admins = j.value("admins", std::vector<uint64_t>{});
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

    void to_json(nlohmann::json& j, const logic& l) {
        j = json({
            { "timeout", l.timeout},
            { "timeout_limit", l.timeout_limit},
            { "disabled_codes", l.disabled_codes},
            { "code", l.code},
            { "machines", l.machines}
        });
    }

    void from_json(const nlohmann::json& j, logic& l) {
        if (j.contains("timeout")) {
            j.at("timeout").get_to(l.timeout);
        }
        if (j.contains("code")) {
            j.at("code").get_to(l.code);
        }
        if (j.contains("disabled_codes")) {
            j.at("disabled_codes").get_to(l.disabled_codes);
        }
        if (j.contains("timeout_limit")) {
            j.at("timeout_limit").get_to(l.timeout_limit);
        }
        if (j.contains("machines")) {
            j.at("machines").get_to(l.machines);
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
            if (data.contains("logic")) {
                _logic = data.at("logic"). get<logic>();
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
        data["logic"] = _logic;

        configFile << data.dump(4);
        
        if (configFile.fail()) {
            std::cerr << "[ERROR] Failed to write to config file: " << _configFileName << std::endl;
            return false;
        }

        std::cout << "Info: Configuration saved to " << _configFileName << std::endl;
        return true;
    }

    bool ConfigManager::userExist(const uint64_t u) {
        auto v = getTgConfig().users;
        for (auto it = v.begin(); it != v.end(); ++it) {
            if (*it == u) {
                return true;
            }
        }
        return false;
    }

    bool ConfigManager::addUser(const uint64_t u) {
        auto tg = getTgConfig();
        auto& v = tg.users;
        bool exist = false;
        for (auto it = v.begin(); it != v.end(); ++it) {
            if(*it == u) {
                exist = true;
                break;
            }
        }

        if(!exist) {
            v.push_back(u);
            return updateTgConfig(tg);
        }

        return true;
    }

    bool ConfigManager::removeUser(const uint64_t u) {
        auto tg = getTgConfig();
        auto& v = tg.users;
        if (v.empty()) {
            return true;
        } else {
            std::vector<uint64_t> temp;
            for(auto it =  v.begin(); it != v.end(); ++it) {
                if (*it != u) {
                    temp.push_back(*it);
                }
            }
            v = temp;
            return updateTgConfig(tg);
        }
    }


    bool ConfigManager::updateTgConfig(const tg& t) {
        std::lock_guard<std::mutex> lock(_mutex);
        _tg = t;
        return _write();
    }

    bool ConfigManager::updateMqttConfig(const mqtt& m) {
        std::lock_guard<std::mutex> lock(_mutex);
        _mqtt = m;
        return _write();
    }

    bool ConfigManager::updateLogicConfig(const logic& l) {
        std::lock_guard<std::mutex> lock(_mutex);
        _logic = l;
        return _write();
    }

    std::string ConfigManager::resolveHmiName(const std::string& hmi_id) {
        if (auto search = _logic.machines.find(hmi_id); search != _logic.machines.end()) {
            return "<code>" + search->second + "</code> : <code>" + search->first + "</code>";
        }

        return "<code>" + hmi_id + "</code>";
    }

    bool ConfigManager::addMachine(const std::string& mid, const std::string& pseudo) {
        auto mqtt = getMqttConfig();
        auto& topic = mqtt.topic;
        bool exist = false;
        for (auto& each: mqtt.topic) {
            try {
                if (split(each, '/')[1] ==  mid) {
                    exist = true;
                    break;
                }
            } catch (const std::exception& e) {
                std::cerr << "[ConfigManager] exception in addMachine method: " << e.what() << std::endl << std::flush;
            }
        }
        if (exist)
            return false;
        topic.push_back("NordFrost/" + mid + "/#");
        
        auto logic = getLogicConfig();
        auto& machines = logic.machines;
        machines[mid] = pseudo;

        return updateLogicConfig(logic) && updateMqttConfig(mqtt);
    }

    bool ConfigManager::removeMachine(const std::string& mid) {
        auto mqtt = getMqttConfig();
        auto& topic = mqtt.topic;
        std::vector<std::string> temp;

        for (auto& each: mqtt.topic) {
            try {
                if (split(each, '/')[1] ==  mid)
                    continue;
                temp.push_back(each);
            } catch (const std::exception& e) {
                std::cerr << "[ConfigManager] exception in removeMachine method: " << e.what() << std::endl << std::flush;
            }
        }
        topic = temp;

        auto logic = getLogicConfig();
        logic.machines.erase(mid);
        
        return updateLogicConfig(logic) && updateMqttConfig(mqtt);
    }

    bool ConfigManager::addError(const unsigned int code, const std::string& des) {
        auto logic = getLogicConfig();
        auto& codes = logic.code;
        if (auto search = codes.find(code); search != codes.end()) {
            return false;
        } else {
            codes[code] = des;
        }

        return updateLogicConfig(logic);
    }

    bool ConfigManager::remError(const unsigned int code) {
        auto logic = getLogicConfig();
        auto& codes = logic.code;
        std::unordered_map<unsigned int, std::string> temp;
        for (auto& each: codes) {
            if (each.first != code) {
                temp [each.first] = each.second;
            }
        }
        codes = temp;
        return updateLogicConfig(logic);
    }

    bool ConfigManager::addEx(const unsigned int code) {
        auto logic = getLogicConfig();
        logic.disabled_codes.push_back(code);
        return updateLogicConfig(logic);
    }

    bool ConfigManager::remEx(const unsigned int code) {
        auto logic = getLogicConfig();
        auto& d_codes = logic.disabled_codes;
        std::vector<unsigned int> temp;
        for (auto each: d_codes) {
            if (each != code) {
                temp.push_back(each);
            }
        }
        d_codes = temp;
        return updateLogicConfig(logic);
    }

}   //  namespace Settings
