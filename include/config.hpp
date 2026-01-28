//  include/config.hpp

#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <mutex>

#include <nlohmann/json.hpp>


namespace Settings {
    struct tg {
        std::string token;
        std::vector<std::string> users;
    };
    void to_json(nlohmann::json& j, const tg& t);
    void from_json(const nlohmann::json& j, tg& t);

    struct mqtt {
        std::string broker = "localhost";
        unsigned short port = 1883;

        std::string client_id = "cpp_client";
        std::string client_name = "";
        std::string pwd = "";

        std::string topic = "topic";
    };
    void to_json(nlohmann::json& j, const mqtt& m);
    void from_json(const nlohmann::json& j, mqtt& m);

    struct logic {
        time_t timeout = 30;
        std::unordered_map<unsigned int, std::string> code{
            {0,     "FINE"},
            {100,   "WE COOKED"}
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

        template<typename U>
        bool update(const std::string& key, const U& value);
        template<typename U>
        std::optional<U> get(const std::string& key);

        bool addUser(const std::string& u);
        bool removeUser(const std::string& u);
        bool userExist(const std::string& u);
    };
}
