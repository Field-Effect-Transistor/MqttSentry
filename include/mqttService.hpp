//  include/mqttService.hpp
#pragma once

#include <functional>

#include <boost/mqtt5/mqtt_client.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/mqtt5/logger.hpp>

#include "settings.hpp"
#include "types.hpp"
#include "topicWatchdog.hpp"

/**
 * @brief Керує Boost MQTT5 клієнтом, відстежує стан машин та їх серцебиття через watchdog
 */
class MqttService {
    public:

    using OnAlert = std::function<void(const AlertEvents)>;
    
    using OnMS =                std::function<void(const std::string, const MachineState)>;
    using OnMachineIn =         std::function<void(const std::string, const MachineIn)>;
    using OnMachineInFilter =   std::function<void(const std::string, const MachineInFilter)>;
    using OnMachineOut =        std::function<void(const std::string, const MachineOut)>;
    using OnMachineAlarm =      std::function<void(const std::string, const MachineAlarmKod)>;
    using OnMachineLight =      std::function<void(const std::string, const MachineLight)>;

    using ConnectionCallback = std::function<void(void)>;

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
    void setOnMachineIn(OnMachineIn callback) { _onMIn = std::move(callback); }
    void setOnMachineInFilter(OnMachineInFilter callback) { _onMInf = std::move(callback); }
    void setOnMachineLight(OnMachineLight callback) { _onMLight = std::move(callback); }
    void setOnMachineOut(OnMachineOut callback) { _onMOut = std::move(callback); }
    void setOnMachineAlarm(OnMachineAlarm callback) { _onMAlarm = std::move(callback); }
    
    void setOnConnection(ConnectionCallback callback) { _onConnection = std::move(callback);}
    void setOnDisconnection(ConnectionCallback callback) { _onDisconnection = std::move(callback);}

    inline bool isConnected() { return _isConnected; }

    private:
    Settings::ConfigManager& _cm;
    boost::asio::io_context& _ioc;
    std::shared_ptr<MqttClient> _client;
    std::vector<std::shared_ptr<TopicWatchdog>> _watchdogs;
    std::atomic<bool> _isConnected;

    //  callbacks
    OnAlert         _onAlert;   ///< обробник для критичних тривог.

    OnMS                _onMS;      ///< обробник для оновлення поточного стану машини (позиція/таймштамп).
    OnMachineIn         _onMIn;     ///< обробник для оновлення поточного входу на машину
    OnMachineInFilter   _onMInf;     ///< обробник для оновлення поточного входу на машину
    OnMachineOut        _onMOut;    ///< обробник для оновлення поточного виходу машини
    OnMachineLight      _onMLight;   ///< обробник для оновлення даних лічильників (ECO/Light)
    OnMachineAlarm      _onMAlarm;

    ConnectionCallback _onConnection;       ///< обробник появи доступу до брокера
    ConnectionCallback _onDisconnection;    ///< обробник зникнення доступу до брокера


    std::shared_ptr<boost::asio::steady_timer> _retryTimer;

    bool _isWaitingForResponse;
    std::shared_ptr<boost::asio::steady_timer> _pingTimer;
    std::shared_ptr<boost::asio::steady_timer> _responseTimer;
    

    void _subscribeToTopics();

    /**
     * @brief внутрішня функція-обробник відповідей
     */
    void _recieveLoop();
    void _sendPing();
    void _pingLoop();
};

