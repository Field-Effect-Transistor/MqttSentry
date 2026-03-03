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

/**
 * @brief   Містить класи та структури для роботи з налаштуваннями (JSON, MQTT, Telegram).
 */
namespace Settings {
    /**
     * @brief Конфігурація Telegram-бота.
     */
    struct tg {
        std::string token;              ///< API токен авторизації
        std::vector<uint64_t> users;    ///< Підписники на сповіщення
        std::vector<uint64_t> admins;   ///< Адміністратори системи
    };
    /**
     * @brief Серіалізація структури tg у JSON-об'єкт через nlohmann::json
     * 
     * @param j [out] JSON-об'єкт, у який записуються дані.
     * @param t [in] Структура з налаштуваннями Telegram.
     */
    void to_json(nlohmann::json& j, const tg& t);
    /**
     * @brief Десереалізація JSON-об'єкта у tg через nlohmann::json
     * 
     * @param j [in] JSON-об'єкт, з якого дані зчитуються
     * @param t [out] Структура у яку дані записуються
     */
    void from_json(const nlohmann::json& j, tg& t);

    /**
     * @brief Конфігурація mqtt клієнта
     */
    struct mqtt {
        std::string broker = "localhost";       ///< dns брокера
        unsigned short port = 1883;             ///< порт брокера

        std::string client_id = "cpp_client";   ///< ідентифікатор клієнта
        std::string client_name = "";           ///< ім'я користувача (для брокерів з авторизацією)
        std::string pwd = "";                   ///< пароль користувача (для брокерів з авторизацією)

        std::vector<std::string> topic{"topic"};///< список (вектор) топіків виду "NordFrost/MACHINE_ID/#",
    };
    /**
     * @brief Серіалізація структури mqtt у JSON-об'єкт (nlohmann::json)
     * 
     * @param j [out] JSON-об'єк, у який записуютсья дані
     * @param m [in]  mqtt структура, з якої дані вичитуються
     */
    void to_json(nlohmann::json& j, const mqtt& m);
    /**
     * @brief Десеріалізація JSON  у об'єкт mqtt
     * 
     * @param j [in]  JSON-об'єкт, з якого дані вичитуються
     * @param m [out] структуар mqtt, яку записуються дані
     */
    void from_json(const nlohmann::json& j, mqtt& m);

    /**
     * @brief Конфігурація логіки роботи програми (кількість спрацювань вотчдога, коди помилок, тощо)
     */
    struct logic {
        time_t timeout = 60;            ///< час спрацювання таймера серцебиття
        unsigned int timeout_limit = 3; ///< кількість оповіщуваних спрацювань таймера

        std::vector<unsigned int> disabled_codes{0};        ///< список (вектор) кодів, що на них не має спрацьовувати оповіщення
        std::unordered_map<unsigned int, std::string> code{ ///< мапа кодів та їх розшифрувань
            {0,     "FINE"},
            {100,   "WE COOKED"}
        };  

        std::unordered_map<std::string, std::string> machines{  ///< мапа id машин, та їх псевдо
            {"B83DF67CE1D5", "Testing Machine"}
        };

        std::unordered_map<unsigned int, std::string> poses{    ///< мапа станів машини та пояснення
            {0, "IDLE"}
        };
    };
    /**
     * @brief Серіалізація структури logic у JSON-об'єкт (nlohmann::json)
     * 
     * @param j [out] JSON-об'єкт, у яуого будуть записані дані
     * @param l [in]  структура logic, з якої будуть вичитані дані
     */
    void to_json(nlohmann::json& j, const logic& l);
    /**
     * @brief Десеріалізація JSON-об'єкта у структуру logic
     * 
     * @param j [in]  JSON-об'єкт, з якого будуть зчитані дані
     * @param l [out] структура logic, у яку дані будуть записані
     */
    void from_json(const nlohmann::json& j, logic& l);

    struct graphite {
        std::string dns;
        uint16_t    port;
    };
    void to_json(nlohmann::json& j, const graphite& g);
    void from_json(const nlohmann::json& j, graphite& g);

    /**
     * @brief Керує конфігурацією застосунку, записує, читає конфігураційний файл, потокобезпечний
     */
    class ConfigManager {
    private:
        bool _loaded = false;
        tg   _tg;
        mqtt _mqtt;
        logic _logic;
        graphite _graphite;
        std::string _configFileName;

        mutable std::mutex _mutex;

        /**
         * @brief Потоконебезпечне читтання, потрібне для уникнення блокування потоків, при виклику публічних функцій читання/запису 
         */
        inline bool _load();

        /**
         * @brief Потоконебезпечний запис, потрібни1 для уникнення блокування потоків, при виклику публічних функцій читання/запису 
         */
        inline bool _write();
        
    public:
        ConfigManager(const std::string& configFileName);
        ~ConfigManager() {};

        tg      getTgConfig()   const { std::lock_guard<std::mutex> lock(_mutex);   return _tg; };
        mqtt    getMqttConfig() const { std::lock_guard<std::mutex> lock(_mutex);   return _mqtt; };
        logic   getLogicConfig() const { std::lock_guard<std::mutex> lock(_mutex); return _logic; };
        graphite getGraphiteConfig() const { std::lock_guard<std::mutex> lock(_mutex); return _graphite; };

        bool updateTgConfig(const tg& t);
        bool updateMqttConfig(const mqtt& m);
        bool updateLogicConfig(const logic& l);
        bool updateGraphiteConfig(const graphite& g);

        /** 
         * @name Керування користувачами 
         * @{ 
         */
        
        /// Додає ID користувача Telegram до списку розсилки сповіщень.
        /// @return true, якщо список оновлено та збережено на диск, або користувач уже існує.
        bool addUser(const uint64_t u);

        /// Видаляє користувача зі списку розсилки.
        /// @return true, якщо список оновлено та збережено на диск.
        bool removeUser(const uint64_t u);

        /// Перевіряє, чи зареєстрований користувач у системі.
        bool userExist(const uint64_t u);
        /** @} */

        /** 
         * @name Керування обладнанням (HMI)
         * @{ 
         */
        
        /// Повертає зрозумілу назву машини за її ID
        /// Якщо назву не знайдено, повертає вхідний hmi_id.
        std::string resolveHmiName(const std::string& hmi_id);

        std::string resolvePath(const std::string& hmi_id);

        /// Додає нову машину або оновлює існуючий псевдонім (псевдо).
        bool addMachine(const std::string& mid, const std::string& pseudo);

        /// Видаляє машину та її псевдонім з конфігурації.
        bool removeMachine(const std::string& mid);
        /** @} */

        /** 
         * @name Керування словником помилок та фільтрацією
         * @{ 
         */
        
        /// Додає опис помилки (des) для конкретного коду (code) у словник.
        /// @return true, якщо успіщно додано нову помилку, та записано на диск
        bool addError(const unsigned int code, const std::string& des);

        /// Видаляє опис помилки зі словника.
        bool remError(const unsigned int code);

        /**
         * @brief Додає код помилки до списку виключень (mute/ignore).
         * Сповіщення за цим кодом надсилатися не будуть.
         */
        bool addEx(const unsigned int code);

        /// Видаляє код помилки зі списку виключень (unmute).
        bool remEx(const unsigned int code);
        /** @} */
        };
}// namespace Settings
