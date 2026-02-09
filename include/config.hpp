//  include/config.hpp
#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <cstdint>

#include <nlohmann/json.hpp>


namespace Settings {
    struct tg {
        std::string token;
        std::vector<uint64_t> users;
        std::vector<uint64_t> admins;
    };
    void to_json(nlohmann::json& j, const tg& t);
    void from_json(const nlohmann::json& j, tg& t);

    struct mqtt {
        std::string broker = "localhost";
        unsigned short port = 1883;

        std::string client_id = "cpp_client";
        std::string client_name = "";
        std::string pwd = "";

        std::vector<std::string> topic{"topic"};
    };
    void to_json(nlohmann::json& j, const mqtt& m);
    void from_json(const nlohmann::json& j, mqtt& m);

    struct logic {
        time_t timeout = 60;
        unsigned int timeout_limit = 3;
        std::vector<unsigned int> disabled_codes{0};
        std::unordered_map<unsigned int, std::string> code{
            {0,     "FINE"},
            {100,   "WE COOKED"}
        };
        std::unordered_map<std::string, std::string> machines{
            {"B83DF67CE1D5", "Testing Machine"}
        };
    };
    void to_json(nlohmann::json& j, const logic& l);
    void from_json(const nlohmann::json& j, logic& l);

    class ConfigManager {
    private:
        bool _loaded = false;
        tg   _tg;
        mqtt _mqtt;
        logic _logic;
        std::string _configFileName;

        mutable std::mutex _mutex;

        inline bool _load();   //  without mutex
        inline bool _write();  //  without mutex
        
    public:
        ConfigManager(const std::string& configFileName);
        ~ConfigManager() {};

        //  deprecated setters n getters
        //template<typename U>
        //bool update(const std::string& key, const U& value);
        //template<typename U>
        //std::optional<U> get(const std::string& key);

        tg      getTgConfig()   const { std::lock_guard<std::mutex> lock(_mutex);   return _tg; };
        mqtt    getMqttConfig() const { std::lock_guard<std::mutex> lock(_mutex);   return _mqtt; };
        logic   getLogicConfig() const { std::lock_guard<std::mutex> lock(_mutex); return _logic; };

        bool updateTgConfig(const tg& t);
        bool updateMqttConfig(const mqtt& m);
        bool updateLogicConfig(const logic& l);

        //  User management methodes
        bool addUser(const uint64_t u);
        bool removeUser(const uint64_t u);
        bool userExist(const uint64_t u);
    };
}// namespace Settings
