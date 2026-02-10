//  src/adminController.cpp
#include "adminController.hpp"

#include <algorithm>

#include "utils.hpp"

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
            _awaitingInput[query->from->id].key = "logic.timeout";
            _bot.getApi().sendMessage(chatId, "📝 Введіть нове значення <b>часу таймаута</b> (сек):", nullptr, nullptr, nullptr, "HTML");
        } else if (query->data == "admin_timeouts_limit_change") {
            _awaitingInput[query->from->id].key = "logic.timeout_limit";
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
                toSend += "<code>" + std::to_string(user) + "</code>\n";
            }
            _bot.getApi().sendMessage(query->from->id, toSend, nullptr, nullptr, nullptr, "HTML");
        } else if (query->data == "admin_users_add") {
            _awaitingInput[query->from->id].key = "tg.users.add";
            _bot.getApi().sendMessage(chatId, "📝 Введіть <b>id нового користувача</b>:", nullptr, nullptr, nullptr, "HTML");
        } else if (query->data == "admin_users_rem") {
            _awaitingInput[query->from->id].key = "tg.users.rem";
            _bot.getApi().sendMessage(chatId, "📝 Введіть <b>id користувача</b>, якого треба прибрати:", nullptr, nullptr, nullptr, "HTML");
        } else

        //  Machines
        if (query->data == "admin_machines") {
            _on_admin_machines(chatId, msgId);
        } else if (query->data == "admin_machines_peek") {
            std::string toSend;
            auto mqtt = _cm.getMqttConfig();
            auto logic = _cm.getLogicConfig();
            for (auto& topic: mqtt.topic) {
                try {
                    std::string id = split(topic, '/')[1];
                    if (auto search = logic.machines.find(id); search != logic.machines.end()) {
                        toSend += "<code>" + id + "</code> : " + search->second + '\n';
                    } else {
                        toSend += "<code>" + id + "</code>\n";
                    }
                } catch (const std::out_of_range& e) {
                    std::cerr << "[AdminController] out_of_range exception in registerCommand() on onCallbackQuerry " << e.what();
                }   
            }
            _bot.getApi().sendMessage(chatId, toSend, nullptr, nullptr, nullptr, "HTML");
        } else if (query->data == "admin_machines_state") {
            _awaitingInput[query->from->id].key = "machines.state";
            _bot.getApi().sendMessage(chatId, "📝 Введіть <b>ID машини</b>, яку потрібно оглянути:", nullptr, nullptr, nullptr, "HTML");
        } else if (query->data == "admin_machines_add") {
            _awaitingInput[query->from->id].key = "machines.add.id";
            _bot.getApi().sendMessage(chatId, "📝 Введіть <b>ID машини</b>, яку потрібно додати:", nullptr, nullptr, nullptr, "HTML");
        } else if (query->data == "admin_machines_rem") {
            _awaitingInput[query->from->id].key = "machines.rem";
            _bot.getApi().sendMessage(chatId, "📝 Введіть <b>ID машини</b>, яку потрібно видалити:", nullptr, nullptr, nullptr, "HTML");
        } else 

        if (query->data == "admin_restart") {
            std::cout << "[AdminController] App restarting...\n" << std::flush;
            _bot.getApi().sendMessage(chatId, "🚀 Система перезапускається. Почекайте ~15 секунд...");
            std::raise(SIGINT);
        }
        
        _bot.getApi().answerCallbackQuery(query->id);
    });
}

using Keyboard = TgBot::InlineKeyboardMarkup;
using Button = TgBot::InlineKeyboardButton; 
void AdminController::_showAdminPannel(const uint64_t id) {
    Keyboard::Ptr keyboard(new Keyboard);
    Button::Ptr btnError(new Button()), btnTimeouts(new Button), btnMachines(new Button), btnUsers(new Button), btnRestart(new Button);

    std::vector<Button::Ptr> row1, row2, row3, row4, row5;

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

    btnRestart->text = "🚀 Перезавантажити";
    btnRestart->callbackData = "admin_restart";
    row5.push_back(btnRestart);

    keyboard->inlineKeyboard.push_back(row1);
    keyboard->inlineKeyboard.push_back(row2);
    keyboard->inlineKeyboard.push_back(row3);
    keyboard->inlineKeyboard.push_back(row4);
    keyboard->inlineKeyboard.push_back(row5);
    
    _bot.getApi().sendMessage(id, "🛠 <b>Адмін-панель керування</b>\nОберіть потрібну дію:", nullptr, nullptr, keyboard, "HTML");
}


void AdminController::_on_admin(const uint64_t chatId, const uint32_t messageId) {
    Keyboard::Ptr keyboard(new Keyboard);
    Button::Ptr btnError(new Button()), btnTimeouts(new Button), btnMachines(new Button), btnUsers(new Button), btnRestart(new Button);

    std::vector<Button::Ptr> row1, row2, row3, row4, row5;

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

    btnRestart->text = "🚀 Перезавантажити";
    btnRestart->callbackData = "admin_restart";
    row5.push_back(btnRestart);

    keyboard->inlineKeyboard.push_back(row1);
    keyboard->inlineKeyboard.push_back(row2);
    keyboard->inlineKeyboard.push_back(row3);
    keyboard->inlineKeyboard.push_back(row4);
    keyboard->inlineKeyboard.push_back(row5);
    
    _bot.getApi().editMessageText(
        "🛠 <b>Адмін-панель керування</b>\nОберіть потрібну дію:", 
        chatId, messageId, "", "HTML", nullptr, keyboard
    );
}

void AdminController::_on_admin_timeouts(const uint64_t chatId, const uint32_t messageId) {
    Keyboard::Ptr keyboard(new Keyboard);
    Button::Ptr btnChangeTimeout(new Button), btnChangeTimeoutLimit(new Button), btnBack(new Button);

    std::vector<Button::Ptr> row1, row2, row3;

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
        "(c)\n Кількість таймаутів з оповіщенням: " + std::to_string(logic.timeout_limit);
    _bot.getApi().editMessageText(
        toSend, chatId, messageId, "", "HTML", nullptr, keyboard
    );
}

void AdminController::_on_admin_users(const uint64_t chatId, const uint32_t messageId) {
    Keyboard::Ptr keyboard(new Keyboard);
    Button::Ptr btnPeek(new Button), btnAdd(new Button), btnRem(new Button), btnBack(new Button);

    std::vector<Button::Ptr> row1, row2, row3, row4;

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

void AdminController::_on_admin_machines(const uint64_t chatId, const uint32_t messageId) {
    Keyboard::Ptr keyboard(new Keyboard);
    Button::Ptr btnPeek(new Button()), btnAdd(new Button), btnRem(new Button), btnState(new Button), btnBack(new Button);

    std::vector<Button::Ptr> row1, row2, row3, row4, row5;

    btnPeek->text = "Переглянути";
    btnPeek->callbackData = "admin_machines_peek";
    row1.push_back(btnPeek);

    btnAdd->text = "Додати";
    btnAdd->callbackData = "admin_machines_add";
    row2.push_back(btnAdd);

    btnRem->text = "Видалити";
    btnRem->callbackData = "admin_machines_rem";
    row3.push_back(btnRem);

    btnState->text = "Стан робота";
    //btnState->callbackData = "admin_machines_state";
    btnState->switchInlineQueryCurrentChat = "stateof ";
    row4.push_back(btnState);

    btnBack->text = "Назад";
    btnBack->callbackData = "admin";
    row5.push_back(btnBack);

    keyboard->inlineKeyboard.push_back(row1);
    keyboard->inlineKeyboard.push_back(row2);
    keyboard->inlineKeyboard.push_back(row3);
    keyboard->inlineKeyboard.push_back(row4);
    keyboard->inlineKeyboard.push_back(row5);

    _bot.getApi().editMessageText(
        "🤖 Роботи", chatId, messageId, "", "HTML", nullptr, keyboard
    );
}

void AdminController::_onAnyMessage(TgBot::Message::Ptr message) {
    if (!_isAdmin(message->from->id)) return;

    auto it = _awaitingInput.find(message->from->id);
    if (it != _awaitingInput.end()) {
        std::string key = it->second.key;
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
            } else

            //  Machines 
            if (key == "machines.state") {
                MachineState ms;
                _getMachineState(text, ms);
                _bot.getApi().sendMessage(message->chat->id, std::string("step_pos: ") + std::to_string(ms.state) + "\n ts: " + ms.ts );
                success = true;
            } else if (key == "machines.add.id") {
                it->second.param = message->text;
                it->second.key = "machines.add.pseudo";
                _bot.getApi().sendMessage(message->chat->id, "📝 Введіть <b>Підпис машини</b>:", nullptr, nullptr, nullptr, "HTML");
            } else if (key == "machines.add.pseudo") {
                success = _cm.addMachine(it->second.param, message->text);
            } else if (key == "machines.rem") {
                success = _cm.removeMachine(message->text);
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
    else {
        //  todo:
    }
}

void AdminController::registerInlineSearch() {
    _bot.getEvents().onInlineQuery([this](TgBot::InlineQuery::Ptr query) {
        try {
            std::string text = query->query;
            auto tokens = split(text,' ');
            for (auto token: tokens) {
                std::cout << token << std::endl << std::flush;
            }
            if (tokens.size() == 0 || tokens.size() > 2) {
                _bot.getApi().answerInlineQuery(query->id, {});
                return;
            }
            if (tokens.size() == 1){
                tokens.push_back("");
            }

            if (auto& command = tokens[0], &s_id = tokens[1]; command  == "stateof") {
                std::vector<TgBot::InlineQueryResult::Ptr> results;

                auto machines = _cm.getLogicConfig().machines;

                int id = 0;
                for (auto const& [m_id, name] : machines) {
                    if (s_id.empty() || m_id.find(s_id) != std::string::npos || name.find(s_id) != std::string::npos) {
                        
                        TgBot::InlineQueryResultArticle::Ptr val(new TgBot::InlineQueryResultArticle);
                        val->id = std::to_string(id++);
                        val->title = name;
                        val->description = "ID: " + m_id;
                        
                        TgBot::InputTextMessageContent::Ptr content(new TgBot::InputTextMessageContent);
                        content->messageText = "/stateof " + m_id;
                        val->inputMessageContent = content;

                        results.push_back(val);
                    }
                    if (results.size() > 20) break;
                }

                _bot.getApi().answerInlineQuery(query->id, results);
            } else {
                _bot.getApi().answerInlineQuery(query->id, {});
            }
        } catch(const std::exception& e) {
            std::cerr << "[AdminController] Exception in registerInlineSearch lambda " << e.what() << std::endl << std::flush;
        }
    }
    );
        
}
