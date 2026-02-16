//  include/tgService.hpp
#pragma once

#include <tgbot/tgbot.h>

#include "config.hpp"
#include "types.hpp"
#include "adminController.hpp"

#include <memory>
#include <atomic>

class TgService {    
    public:
    TgService(Settings::ConfigManager& cm); 
    //~TgService();

    void runLongPoll();
    void sendAlert(const AlertEvents& alert);
    void stop() { _exit = true; };
    void set_getMachineState(const std::function<void(const std::string&, MachineState&)>& func) { _getMachineState = func; };
    void set_getMachineLight(const std::function<void(const std::string&, MachineLight&)>& func) { _getMachineLight = func; };

    private:
    Settings::ConfigManager& _cm;
    TgBot::Bot _bot;
    std::atomic<bool> _exit;
    AdminController _admin;
    std::function<void(const std::string&, MachineState&)> _getMachineState;
    std::function<void(const std::string&, MachineLight&)> _getMachineLight;
    //void onStart();
};

