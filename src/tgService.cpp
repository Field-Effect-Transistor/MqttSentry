//  src/tgService.cpp
#include "tgService.hpp"

#include <stdexcept>
#include <thread>

TgService::TgService(Settings::ConfigManager& cm):
    _cm(cm),
    _bot(cm.getTgConfig().token)
{
    //  subscribe for message event
    _bot.getEvents().onAnyMessage([](TgBot::Message::Ptr message) {
        if (StringTools::startsWith(message->text, "/start")) {
            std::cout << "[WARN] NEW USER CANDIDATE: " << message->from->id << std::endl;
            return;
        }
    });

    printf("Bot username: @%s\n", _bot.getApi().getMe()->username.c_str());
    _bot.getApi().deleteWebhook();
}

void TgService::runLongPoll() {
    try {
        TgBot::TgLongPoll longPoll(_bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
            if(_exit) {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[TgService] Exception in TgService::run: " << e.what() << std::endl;
    }
}

void TgService::sendAlert(const AlertEvents& alert) {
    std::string toSend = "[MACHINE]: " + alert.machine_id
     + "\n[MESSAGE]: " + alert.message
     + "\n[TIMESTAMP]:" + alert.timestamp + '\n';
    
    try {
        auto users_s = _cm.getTgConfig().users;
        for(const auto& user : users_s) {
            _bot.getApi().sendMessage(std::stoul(user), toSend);
        }
    } catch (const std::exception& e) {
        std::cerr << "[TgService] Failed to send Alert: " << e.what() << std::endl;
    }
}
