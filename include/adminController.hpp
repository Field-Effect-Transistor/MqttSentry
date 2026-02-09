//  include/adminController.hpp
#pragma once

#include "config.hpp"
#include "types.hpp"

#include <tgbot/tgbot.h>
#include <cstdint>

class AdminController {
    Settings::ConfigManager& _cm;
    TgBot::Bot& _bot;
    std::map<uint64_t, std::string> _awaitingInput;

    bool _isAdmin(uint64_t userId);

    public:
    AdminController(
        Settings::ConfigManager& cm,
        TgBot::Bot& bot
    ): _cm(cm), _bot(bot) {}

    void registerCommands();
    
    private:
    void _showAdminPannel(const uint64_t id);

    void _on_admin(const uint64_t chatId, const uint32_t messageId);
    void _on_admin_timeouts(const uint64_t chatId, const uint32_t messageId);
    void _on_admin_users(const uint64_t chatId, const uint32_t messageId);

    void _onAnyMessage(TgBot::Message::Ptr message);
};
