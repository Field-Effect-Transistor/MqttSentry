//  include/tgService.hpp
#pragma once

#include <tgbot/tgbot.h>

#include "settings.hpp"
#include "types.hpp"
#include "adminController.hpp"

#include <memory>
#include <atomic>

/**
 * @brief   Керує телеграм ботом
 */
class TgService {    
    public:
    /**
     * @param cm посилання на конфіг менеджер
     */
    TgService(Settings::ConfigManager& cm); 
    ~TgService() = default;

    /**
     * @brief функція-обробник тг запитів, блокуюча, для виходу з циклу див. stop()
     */
    void runLongPoll();

    /**
     * @brief форматована відправка сповіщень
     * 
     * @param alert посилання на структуру-контейнер інформації про оповіщення
     */
    void sendAlert(const AlertEvents& alert);

    /**
     * @brief відправка повідомлень
     */
    void send(const std::string& message);

    /**
     * @brief метод-зупинка обробника тг запитів, див. runLongPoll()
     */
    void stop() { _exit = true; };

    /**
     * @brief Всатновити функцію-провайдера для отримання поточного стану машини
     */
    void set_getMachineState(const std::function<void(const std::string&, MachineState&)>& func) { _getMachineState = func; };

    /**
     * @brief Всатновити функцію-провайдера для отримання поточного стану машини
     */
    void set_getMachineLight(const std::function<void(const std::string&, MachineLight&)>& func) { _getMachineLight = func; };

    bool isConnection()const {
        return _isConnection;
    }

    private:
    Settings::ConfigManager& _cm;
    TgBot::Bot _bot;
    std::atomic<bool> _exit;
    AdminController _admin;
    std::atomic<bool> _isConnection;
    
    std::function<void(const std::string&, MachineState&)> _getMachineState;
    std::function<void(const std::string&, MachineLight&)> _getMachineLight;
};

