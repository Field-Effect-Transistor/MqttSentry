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

    using OnMSCallback = std::function<void(const std::string, const MachineState)>;
    using OnAlertCallback = std::function<void(const AlertEvents)>;
    using OnLightCallback = std::function<void(const std::string, const MachineLight)>;
    using MqttClient = boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket, std::monostate, boost::mqtt5::logger>;
    
    /**
     * @param cm    посилання на конфіг менедежер
     * @param ioc   посилання на асинхронний контекст вводу/виводу, потрібен для роботи таймерів та роботи mqtt клієнта
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

    /**
     * @brief Встановлює обробник для критичних тривог.
     * @param callback Функція, яка буде викликана при отриманні коду помилки.
     */
    void setOnAlert(OnAlertCallback callback) { 
        _onAlert = std::move(callback); 
    }

    /**
     * @brief Встановлює обробник для оновлення поточного стану машини (позиція/таймштамп).
     * @param callback Функція, що буде викликана при отриманні оновленого стану машини
     */
    void setOnMachineStateCallback(OnMSCallback callback) { 
        _onMS = std::move(callback); 
    }

    /**
     * @brief Встановлює обробник для оновлення даних лічильників (ECO/Light).
     * @param callback Функція, що буде викликана при отриманні нових даних
     */
    void setOnLightCallback(OnLightCallback callback) { 
        _onLight = std::move(callback); 
    }
    private:
    Settings::ConfigManager& _cm;
    boost::asio::io_context& _ioc;
    std::shared_ptr<MqttClient> _client;
    std::vector<std::shared_ptr<TopicWatchdog>> _watchdogs;
    OnAlertCallback _onAlert;
    OnMSCallback _onMS;
    OnLightCallback _onLight;

    std::shared_ptr<boost::asio::steady_timer> _retryTimer;

    /**
     * @brief внутрішня функція-обробник відповідей
     */
    void _recieveLoop();
};

