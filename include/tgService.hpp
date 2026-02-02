//  include/tgService.hpp
#pragma once

#include <tgbot/tgbot.h>

#include "config.hpp"
#include "types.hpp"

#include <memory>
#include <atomic>

class TgService {    
    public:
    TgService(Settings::ConfigManager& cm); 
    //~TgService();

    void runLongPoll();
    void sendAlert(const AlertEvents& alert);
    void stop() { _exit = true; };

    private:
    Settings::ConfigManager& _cm;
    TgBot::Bot _bot;
    std::atomic<bool> _exit;

    //void onStart();
};

