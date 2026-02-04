//  src/tgService.cpp
#include "tgService.hpp"

#include <stdexcept>
#include <thread>

TgService::TgService(Settings::ConfigManager& cm):
    _cm(cm),
    _bot(cm.getTgConfig().token)
{
    //  subscribe for message event
    _bot.getEvents().onCommand("start", [this](TgBot::Message::Ptr message) {
        try {
            std::cerr << "[TgService] New User Candidate: " << message->from->id << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[TgService] Exception in TgService lambda onAnyMessage: " << e.what() << std::endl;
        }
        
    });

    printf("Bot username: @%s\n", _bot.getApi().getMe()->username.c_str());
    _bot.getApi().deleteWebhook();
}

void TgService::runLongPoll() {
    _exit = false;
    try {
        TgBot::TgLongPoll longPoll(_bot);
        while (true) {
            printf("[TgService] Long poll started\n");
            longPoll.start();
            if(_exit) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& e) {
        std::cerr << "[TgService] Exception in TgService::run: " << e.what() << std::endl;
    }
}

void TgService::sendAlert(const AlertEvents& alert) {
    std::string toSend = "[MACHINE]: " + alert.machine_id
     + "\n[MESSAGE]: " + alert.message
     + "\n[TIMESTAMP]:" + alert.timestamp + '\n';
    std::cout << "[TgBot] Alert sent: " << toSend;

    try {
        auto users_s = _cm.getTgConfig().users;
        for(const auto& user : users_s) {
            _bot.getApi().sendMessage(static_cast<int64_t>(std::stoull(user)), toSend);
        }
    } catch (const std::exception& e) {
        std::cerr << "[TgService] Failed to send Alert: " << e.what() << std::endl;
    }
}
