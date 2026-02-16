//  include/topicWatchdog.hpp
#pragma once

#include <memory>
#include <boost/asio/steady_timer.hpp>
#include <tgbot/tgbot.h>
#include <functional>

#include "config.hpp"

/**
 * @brief Події, генеровані вотчдогом
 */
enum class WatchdogEvents {
    Online,  // Пристрій знову надіслав дані після таймауту
    Offline  // Дані не надходили протягом визначеного часу
};

/**
 * @brief Тип функції зворотного виклику для сповіщення про зміну стану пристрою.
 */
using WatchdogCallback = std::function<void(const WatchdogEvents)>;

/**
 * @brief Відстежує серцебиття конкретного пристрою через MQTT.
 * 
 * Клас використовує асинхронний таймер, std::shared_ptr
 */
class TopicWatchdog : public std::enable_shared_from_this<TopicWatchdog> {
private:
    std::shared_ptr<boost::asio::steady_timer> _timer;
    std::string _hmi_id;
    Settings::ConfigManager& _cm;
    unsigned int _timeouts;
    WatchdogCallback _callback;    

public:
    /**
     * @param ioc Контекст вводу-виводу для роботи таймера.
     * @param hmi_id Унікальний ідентифікатор машини (з топіка).
     * @param cm Посилання на менеджер конфігурації для отримання актуальних таймаутів.
     * @param callback Функція, що буде викликана при зміні статусу.
     */
    TopicWatchdog(
        boost::asio::io_context& ioc,
        const std::string& hmi_id,
        Settings::ConfigManager& cm,
        WatchdogCallback callback
    );

    ~TopicWatchdog() = default;

    /**
     * @brief Скинути вотчдог. Викликається при кожному вхідному повідомленні від пристрою.
     * Якщо пристрій був Offline, переводить його в Online.
     */
    void pet();

    /**
     * @brief Запускає або перезапускає асинхронне очікування таймауту.
     */
    void start_timer();

    /**
     * @brief Повертає ID пристрою, за яким закріплений цей ватчдог.
     */
    inline const std::string& get_hmi_id() const { return _hmi_id; };

private:
    /**
     * @brief Внутрішній обробник спрацювання таймера.
     */
    void on_timeout();
};