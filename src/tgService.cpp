//  src/tgService.cpp
#include "tgService.hpp"

#include <stdexcept>
#include <thread>

#include "adminController.hpp"

inline std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

TgService::TgService(Settings::ConfigManager& cm):
    _cm(cm),
    _bot(cm.getTgConfig().token),
    _admin(_cm, _bot)
{
    _admin.set_getMachineState([this](const std::string& id, MachineState& ms) {
        _getMachineState(id, ms);
    });
    _admin.set_getMachineLight([this](const std::string& id, MachineLight& ml) {
        _getMachineLight(id, ml);
    });
    
    _admin.registerInlineSearch();
    _admin.registerCommands();
    printf("Bot username: @%s\n", _bot.getApi().getMe()->username.c_str());
    _bot.getApi().deleteWebhook();
}

void TgService::runLongPoll() {
    _exit = false;
    TgBot::TgLongPoll longPoll(_bot);

    while (!_exit) {
        try {
            printf("[TgService] Long poll started\n");
            longPoll.start();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } catch (const std::exception& e) {
            std::cerr << "[TgService] API/Network Error: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    printf("[TgService] Long poll finished\n");
}

void TgService::sendAlert(const AlertEvents& alert) {
    std::string toSend = alert.machine_id + " :\n";
    switch(alert.state) {
        case AlertEvents::State::fine:      toSend += "✅ "; break;
        case AlertEvents::State::online:    toSend += "📶 "; break;
        case AlertEvents::State::offline:   toSend += "🚫 "; break;
        case AlertEvents::State::error:     toSend += "🚨 "; break;
        case AlertEvents::State::pump_on:   toSend += "🟢 "; break; 
        case AlertEvents::State::pump_off:  toSend += "🔴 "; break; 
    }
    toSend += alert.message + "\n";
    if (alert.timestamp != "NOW") {
        toSend += "🗓️ " + alert.timestamp;
    }
    //std::cout << "[TgBot] Alert sent: " << toSend;

    try {
        auto users_s = _cm.getTgConfig().users;
        for(const auto& user : users_s) {
            _bot.getApi().sendMessage(user, toSend, nullptr, nullptr, nullptr, "HTML");
        }
    } catch (const std::exception& e) {
        std::cerr << "[TgService] Failed to send Alert: " << e.what() << std::endl;
    }
}
