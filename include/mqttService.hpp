//  include/mqttService.hpp
#pragma once

#include <functional>

#include <boost/mqtt5/mqtt_client.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/mqtt5/logger.hpp>

#include "config.hpp"
#include "types.hpp"
#include "topicWatchdog.hpp"

/**
 * @brief Керує Boost MQTT5 клієнтом, відстежує стан машин та їх серцебиття через watchdog
 */
class MqttService {
    public:

    using OnAlert =         std::function<void(const AlertEvents)>;
    using OnMS =            std::function<void(const std::string, const MachineState)>;
    using OnLight =         std::function<void(const std::string, const MachineLight)>;
    using OnMachineIn =     std::function<void(const std::string, const MachineIn)>;
    using OnMachineOut =    std::function<void(const std::string, const MachineOut)>;
    using MqttClient = boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket, std::monostate, boost::mqtt5::logger>;
    
    /**
     * @param cm    посилання на конфіг менедежер
     * @param ioc   посилання на асинхронний контекст вводу/виводу, потрібен для роботи таймерів та mqtt клієнта
     */
    MqttService(
        Settings::ConfigManager& cm,
        boost::asio::io_context& ioc
    );
    ~MqttService() { stop(); } 

    /**
     * @brief вибір топіків, підписка на брокера, запуск клієнта, запуск циклу-обробника
     */
    void start();
    void stop();

    void setOnAlert(OnAlert callback) { _onAlert = std::move(callback); }
    void setOnMachineState(OnMS callback) { _onMS = std::move(callback); }
    void setOnLight(OnLight callback) { _onLight = std::move(callback); }
    void setOnMachineIn(OnMachineIn callback) { _onMIn = std::move(callback); }
    void setOnMachineOut(OnMachineOut callback) { _onMOut = std::move(callback); }

    inline bool isConnected() { return _isConnected; }

    private:
    Settings::ConfigManager& _cm;
    boost::asio::io_context& _ioc;
    std::shared_ptr<MqttClient> _client;
    std::vector<std::shared_ptr<TopicWatchdog>> _watchdogs;
    OnAlert         _onAlert;   ///< обробник для критичних тривог.
    OnMS            _onMS;      ///< обробник для оновлення поточного стану машини (позиція/таймштамп).
    OnLight         _onLight;   ///< обробник для оновлення даних лічильників (ECO/Light)
    OnMachineIn     _onMIn;     ///< обробник для оновлення поточного входу на машину
    OnMachineOut    _onMOut;    ///< обробник для оновлення поточного виходу машини
    std::atomic<bool> _isConnected;

    std::shared_ptr<boost::asio::steady_timer> _retryTimer;

    void _subscribeToTopics();

    /**
     * @brief внутрішня функція-обробник відповідей
     */
    void _recieveLoop();
};

