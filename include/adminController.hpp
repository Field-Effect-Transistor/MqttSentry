//  include/adminController.hpp
#pragma once

#include "settings.hpp"
#include "types.hpp"

#include <tgbot/tgbot.h>
#include <cstdint>
#include <functional>
#include <map>

/**
 * @brief Контролер адміністративних функцій бота.
 * 
 * @details Відповідає за:
 * - Обробку команд (/admin, /start).
 * - Реєстрацію Inline-пошуку
 * - Керування станами багатоетапних діалогів (через _awaitingInput).
 * - Відображення та редагування конфігурації.
 */
class AdminController {
private:
    /**
     * @brief Структура для збереження контексту очікування вводу від користувача.
     */
    struct PendingAction {
        std::string key;   ///< Який параметр редагується
        std::string param; ///< Тимчасовий буфер для першого кроку
    };

    Settings::ConfigManager& _cm;
    TgBot::Bot& _bot;

    std::map<uint64_t, PendingAction> _awaitingInput;

    // Провайдери даних (Dependency Injection через Application)
    std::function<void(const std::string&, MachineState&)> _getMachineState;
    std::function<void(const std::string&, MachineLight&)> _getMachineLight;

    /**
     * @brief Перевіряє, чи має користувач права адміністратора.
     */
    bool _isAdmin(uint64_t userId);

public:
    AdminController(Settings::ConfigManager& cm, TgBot::Bot& bot)
        : _cm(cm), _bot(bot) {}

    /**
     * @brief Реєструє обробники текстових команд та CallbackQuery (кнопок).
     */
    void registerCommands();

    /**
     * @brief Реєструє обробник Inline-запитів (живий пошук по @username бота).
     */
    void registerInlineSearch();

    /** @name Налаштування провайдерів даних */
    /// @{
    void setMachineStateProvider(const std::function<void(const std::string&, MachineState&)>& func) { _getMachineState = func; }
    void setMachineLightProvider(const std::function<void(const std::string&, MachineLight&)>& func) { _getMachineLight = func; }
    /// @}
    
private:
    /** @name Рендеринг інтерфейсів (Inline Keyboards) */
    /// @{
    void _showAdminPannel(const uint64_t id) ;
    void _on_admin(const uint64_t chatId, const uint32_t messageId);
    void _on_admin_timeouts(const uint64_t chatId, const uint32_t messageId);
    void _on_admin_users(const uint64_t chatId, const uint32_t messageId);
    void _on_admin_machines(const uint64_t chatId, const uint32_t messageId);
    void _on_admin_errors(const uint64_t chatId, const uint32_t messageId);
    /// @}

    /**
     * @brief Загальний обробник для повідомлень, що не є командами.
     * Використовується для зчитування значень параметрів під час редагування.
     */
    void _onAnyMessage(TgBot::Message::Ptr message);
};
