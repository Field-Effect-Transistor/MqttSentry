//  src/adminController.cpp
#include "adminController.hpp"

#include <algorithm>

bool AdminController::_isAdmin(uint64_t userId) {
    auto config = _cm.getTgConfig();
    auto& admins = config.admins;
    return std::find(admins.begin(), admins.end(), userId) != admins.end();
}

void AdminController::registerCommands() {
    //  onStart
    _bot.getEvents().onCommand("start", [this](TgBot::Message::Ptr message) {
        try {
            auto id = message->from->id;
            if (!_cm.userExist(id)) {
                std::string toSend = "New User Candidate: <code><b>" + std::to_string(id) + "</b></code>\n";
                _bot.getApi().sendMessage(id, "k");
                auto config = _cm.getTgConfig();
                auto& admins = config.admins;
                for (auto admin: admins) {
                    _bot.getApi().sendMessage(admin, toSend, nullptr, nullptr, nullptr, "HTML");
                }
                std::cout << "[AdminController] " << toSend;
            } else {
                _bot.getApi().sendMessage(id, "Не кнопкодав");
            }
        } catch (const std::exception& e) {
            std::cerr << "[AdminController] Exception in on start lambda: " << e.what() << std::endl;
        }
    });

    _bot.getEvents().onCommand("admin", [this](TgBot::Message::Ptr message) {
        try {
            auto id = message->from->id;
            if(_isAdmin(id)) {
                _showAdminPannel(id);                
            } else {
                _bot.getApi().sendMessage(id, "Недостатньо прав");
            }
        } catch (const std::exception& e) {
            std::cerr << "[AdminController] Exception in on admin lambda" << e.what() << std::endl;
        }

    });

    _bot.getEvents().onAnyMessage([this](TgBot::Message::Ptr message) {
        _onAnyMessage(message);
    });

    _bot.getEvents().onCallbackQuery([this](TgBot::CallbackQuery::Ptr query) {
        if (!_isAdmin(query->from->id)) return;
        uint64_t chatId = query->message->chat->id;
        uint32_t msgId = query->message->messageId;

        //  Admin
        if (query->data == "admin") {
            _on_admin(chatId, msgId);
        } else

        //  Timeouts
        if (query->data == "admin_timeouts") {
            _on_admin_timeouts(chatId, msgId);
        } else if (query->data == "admin_timeouts_change") {
            _awaitingInput[query->from->id] = "logic.timeout";
            _bot.getApi().sendMessage(chatId, "📝 Введіть нове значення <b>часу таймаута</b> (сек):", nullptr, nullptr, nullptr, "HTML");
        } else if (query->data == "admin_timeouts_limit_change") {
            _awaitingInput[query->from->id] = "logic.timeout_limit";
            _bot.getApi().sendMessage(chatId, "📝 Введіть нову <b>кількість таймаутів</b> для сповіщення:", nullptr, nullptr, nullptr, "HTML");
        } else
        
        //  Users
        if (query->data == "admin_users") {
            _on_admin_users(chatId, msgId);
        } else if (query->data == "admin_users_peek") {
            std::string toSend;
            auto tg = _cm.getTgConfig();
            auto& users = tg.users;
            for (auto user: users) {
                toSend += std::to_string(user) + '\n';
            }
            _bot.getApi().sendMessage(query->from->id, toSend);
        } else if (query->data == "admin_users_add") {
            _awaitingInput[query->from->id] = "tg.users.add";
            _bot.getApi().sendMessage(chatId, "📝 Введіть <b>id нового користувача</b>:", nullptr, nullptr, nullptr, "HTML");
        } else if (query->data == "admin_users_rem") {
            _awaitingInput[query->from->id] = "tg.users.rem";
            _bot.getApi().sendMessage(chatId, "📝 Введіть <b>id користувача</b>, якого треба прибрати:", nullptr, nullptr, nullptr, "HTML");
        } 
        
        _bot.getApi().answerCallbackQuery(query->id);
    });
}

using Keyboard = TgBot::InlineKeyboardMarkup;
using Button = TgBot::InlineKeyboardButton; 
void AdminController::_showAdminPannel(const uint64_t id) {
    Keyboard::Ptr keyboard(new Keyboard);
    Button::Ptr btnError(new Button()), btnTimeouts(new Button), btnMachines(new Button), btnUsers(new Button);

    std::vector<Button::Ptr> row1;
    std::vector<Button::Ptr> row2;
    std::vector<Button::Ptr> row3;
    std::vector<Button::Ptr> row4;

    btnError->text = "⚠️ Помилки";
    btnError->callbackData = "admin_errors";
    row1.push_back(btnError);

    btnTimeouts->text = "⏲️ Таймаути";
    btnTimeouts->callbackData = "admin_timeouts";
    row2.push_back(btnTimeouts);

    btnMachines->text = "🤖 Роботи";
    btnMachines->callbackData = "admin_machines";
    row3.push_back(btnMachines);

    btnUsers->text = "👤 Користувачі";
    btnUsers->callbackData = "admin_users";
    row4.push_back(btnUsers);

    keyboard->inlineKeyboard.push_back(row1);
    keyboard->inlineKeyboard.push_back(row2);
    keyboard->inlineKeyboard.push_back(row3);
    keyboard->inlineKeyboard.push_back(row4);
    
    _bot.getApi().sendMessage(id, "🛠 <b>Адмін-панель керування</b>\nОберіть потрібну дію:", nullptr, nullptr, keyboard, "HTML");
}


void AdminController::_on_admin(const uint64_t chatId, const uint32_t messageId) {
    Keyboard::Ptr keyboard(new Keyboard);
    Button::Ptr btnError(new Button()), btnTimeouts(new Button), btnMachines(new Button), btnUsers(new Button);

    std::vector<Button::Ptr> row1;
    std::vector<Button::Ptr> row2;
    std::vector<Button::Ptr> row3;
    std::vector<Button::Ptr> row4;

    btnError->text = "⚠️ Помилки";
    btnError->callbackData = "admin_errors";
    row1.push_back(btnError);

    btnTimeouts->text = "⏲️ Таймаути";
    btnTimeouts->callbackData = "admin_timeouts";
    row2.push_back(btnTimeouts);

    btnMachines->text = "🤖 Роботи";
    btnMachines->callbackData = "admin_machines";
    row3.push_back(btnMachines);

    btnUsers->text = "👤 Користувачі";
    btnUsers->callbackData = "admin_users";
    row4.push_back(btnUsers);

    keyboard->inlineKeyboard.push_back(row1);
    keyboard->inlineKeyboard.push_back(row2);
    keyboard->inlineKeyboard.push_back(row3);
    keyboard->inlineKeyboard.push_back(row4);
    
    _bot.getApi().editMessageText(
        "🛠 <b>Адмін-панель керування</b>\nОберіть потрібну дію:", 
        chatId, messageId, "", "HTML", nullptr, keyboard
    );
}

void AdminController::_on_admin_timeouts(const uint64_t chatId, const uint32_t messageId) {
    Keyboard::Ptr keyboard(new Keyboard);
    Button::Ptr btnChangeTimeout(new Button), btnChangeTimeoutLimit(new Button), btnBack(new Button);

    std::vector<Button::Ptr> row1;
    std::vector<Button::Ptr> row2;
    std::vector<Button::Ptr> row3;

    btnChangeTimeout->text = "Змінити час таймаута";
    btnChangeTimeout->callbackData = "admin_timeouts_change";
    row1.push_back(btnChangeTimeout);

    btnChangeTimeoutLimit->text = "Змінити кількість таймаутів з оповіщенням";
    btnChangeTimeoutLimit->callbackData = "admin_timeouts_limit_change";
    row2.push_back(btnChangeTimeoutLimit);

    btnBack->text = "Назад";
    btnBack->callbackData = "admin";
    row3.push_back(btnBack);

    keyboard->inlineKeyboard.push_back(row1);
    keyboard->inlineKeyboard.push_back(row2);
    keyboard->inlineKeyboard.push_back(row3);
    
    auto logic = _cm.getLogicConfig();
    std::string toSend = "⏲️ <b>Поточні налаштування таймаутів: </b>\n Час таймаута: " + std::to_string(logic.timeout) +
        "\n Кількість таймаутів з оповіщенням: " + std::to_string(logic.timeout_limit);
    _bot.getApi().editMessageText(
        toSend, chatId, messageId, "", "HTML", nullptr, keyboard
    );
}

void AdminController::_on_admin_users(const uint64_t chatId, const uint32_t messageId) {
    Keyboard::Ptr keyboard(new Keyboard);
    Button::Ptr btnPeek(new Button), btnAdd(new Button), btnRem(new Button), btnBack(new Button);

    std::vector<Button::Ptr> row1;
    std::vector<Button::Ptr> row2;
    std::vector<Button::Ptr> row3;
    std::vector<Button::Ptr> row4;

    btnPeek->text = "Переглянути";
    btnPeek->callbackData = "admin_users_peek";
    row1.push_back(btnPeek);

    btnAdd->text = "Додати";
    btnAdd->callbackData = "admin_users_add";
    row2.push_back(btnAdd);

    btnRem->text = "Видалити";
    btnRem->callbackData = "admin_users_rem";
    row3.push_back(btnRem);

    btnBack->text = "Назад";
    btnBack->callbackData = "admin";
    row4.push_back(btnBack);

    keyboard->inlineKeyboard.push_back(row1);
    keyboard->inlineKeyboard.push_back(row2);
    keyboard->inlineKeyboard.push_back(row3);
    keyboard->inlineKeyboard.push_back(row4);

    _bot.getApi().editMessageText(
        "👤 Користувачі", chatId, messageId, "", "HTML", nullptr, keyboard
    );
}

void AdminController::_onAnyMessage(TgBot::Message::Ptr message) {
    if (!_isAdmin(message->from->id)) return;

    auto it = _awaitingInput.find(message->from->id);
    if (it != _awaitingInput.end()) {
        std::string key = it->second;
        std::string text = message->text;

        if (text == "Відміна" || text == "/cancel") {
            _awaitingInput.erase(it);
            _bot.getApi().sendMessage(message->chat->id, "❌ Редагування скасовано.");
            return;
        }

        try {
            bool success = false;

            //  LOGIC
            if (key == "logic.timeout") {
                time_t val = std::stoll(text);
                if (val < 5) throw std::runtime_error("Занадто мале значення");
                auto logic = _cm.getLogicConfig();
                logic.timeout = val;
                success = _cm.updateLogicConfig(logic);
            } else if (key == "logic.timeout_limit") {
                unsigned int val = std::stoul(text);
                auto logic = _cm.getLogicConfig();
                logic.timeout_limit = val;
                success = _cm.updateLogicConfig(logic);
            } else
            
            //  TG
            if (key == "tg.users.add") {
                try {
                    success = _cm.addUser(std::stoull(text));
                } catch (const std::exception& e) {
                    _bot.getApi().sendMessage(message->chat->id, "❌ Помилка: ID має бути числом.");
                    std::cerr << e.what() << std::endl;
                }
            } else if (key == "tg.users.rem") {
                success = _cm.removeUser(std::stoull(text));
            }

            if (success) {
                _bot.getApi().sendMessage(message->chat->id, "✅ Значення <b>" + key + "</b> успішно оновлено!", nullptr, nullptr, nullptr, "HTML");
                _awaitingInput.erase(it);
                _showAdminPannel(message->chat->id); 
            }
        } catch (...) {
            _bot.getApi().sendMessage(message->chat->id, "⚠️ Помилка: введіть коректне додатне число.");
        }
    }
}
