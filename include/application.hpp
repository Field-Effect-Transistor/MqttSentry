//  include/appliation.hpp
#pragma once

#include "mqttService.hpp"
#include "tgService.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <thread>
#include <atomic>


/**
 * @brief Головний клас додатка, що оркеструє роботу всіх сервісів.
 * 
 * @details Реалізує патерн "Producer-Consumer":
 * - MqttService (Producer): отримує дані з мережі та кладе в чергу.
 * - WorkerThread (Consumer): забирає дані з черги та відправляє через TgService.
 * Також керує життєвим циклом потоків та обробкою системних сигналів.
 */
class Application {
private:
    /// Менеджер конфігурації (завантажується першим)
    Settings::ConfigManager _cm;
    
    /** @name Спільне сховище даних (Thread-Safe) */
    /// @{
    ThreadSafeQueue<AlertEvents> _alertQueue;           ///< Черга подій для відправки в Telegram
    ThreadSafeMap<std::string, AlertEvents>  _alertMap; ///< Останні тривоги за пристроями
    ThreadSafeMap<std::string, MachineState> _msMap;    ///< Поточні стани HMI
    ThreadSafeMap<std::string, MachineLight> _lightMap; ///< Дані лічильників (ECO/Light)
    ThreadSafeMap<std::string, History<MachineIn>> _mIn;         ///< Дані з входу машини (2 останні значення)
    ThreadSafeMap<std::string, History<MachineOut>> _mOut;       ///< Дані з виходу машини (2 останні значення)
    /// @}

    /** @name Telegram компоненти */
    /// @{
    std::thread _tgThread;    ///< Потік для LongPoll (блокуючий)
    TgService   _tg;          ///< Сервіс взаємодії з Telegram API
    std::thread _workerThread;///< Потік обробника черги (Consumer)
    
    /** @brief Цикл роботи потіка-воркера */
    void _workerLoop();
    /// @}

    /** @name MQTT та Мережеві компоненти */
    /// @{
    boost::asio::io_context _ioc;      ///< Контекст вводу-виводу
    MqttService _mqtt;                 ///< Сервіс взаємодії з MQTT брокером
    boost::asio::signal_set _signals;  ///< Обробник системних сигналів (SIGINT/SIGTERM)
    /// @}

    std::atomic<bool> _running; ///< Прапорець активності додатка

public:
    /**
     * @brief Конструктор додатка.
     * @param configFilePath Шлях до файлу налаштувань JSON.
     */
    Application(const std::string& configFilePath);

    /**
     * @brief Граціозно зупиняє всі сервіси та приєднує потоки.
     */
    ~Application();

    /**
     * @brief Основна точка входу. Запускає всі потоки та блокує main.
     */
    void run();

    /**
     * @brief Ініціює процес зупинки програми.
     */
    void stop();
};

