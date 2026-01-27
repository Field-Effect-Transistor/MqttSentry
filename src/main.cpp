//  src/main.cpp

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <thread>
#include <iostream>
#include <charconv>

#include <tgbot/tgbot.h>

#include <boost/mqtt5/mqtt_client.hpp>
#include <boost/mqtt5/types.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/experimental/parallel_group.hpp>

#include "config.hpp"

using namespace std;
using namespace TgBot;

boost::asio::io_context ioc;

void run_mqtt_monitor(
    Settings::ConfigManager& cm,
    Bot& bot
) {
    auto c = std::make_shared<boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket>>(ioc);
    auto timer = std::make_shared<boost::asio::steady_timer >(ioc);

    c->brokers(*cm.get<std::string>("mqtt.broker"), *cm.get<unsigned short>("mqtt.port"))
        .credentials(
            *cm.get<std::string>("mqtt.client_id"),
            *cm.get<std::string>("mqtt.client_name"),
            *cm.get<std::string>("mqtt.pwd")
        ).async_run(boost::asio::detached);

    c->async_subscribe(
        { *cm.get<std::string>("mqtt.topic") }, boost::mqtt5::subscribe_props {},
            [](boost::mqtt5::error_code ec, std::vector<boost::mqtt5::reason_code>, boost::mqtt5::suback_props) {
                if (!ec) std::cout << "Successfully subscribed via MQTT!" << std::endl;
        }
    );

    std::function<void()> start_monitoring;
    start_monitoring = [&]() {
        timer->expires_after(std::chrono::seconds(10));

        boost::asio::experimental::make_parallel_group(
            timer->async_wait(boost::asio::deferred),
            c->async_receive(boost::asio::deferred)
        ).async_wait(
            boost::asio::experimental::wait_for_one(),
            [&](
                std::array<std::size_t, 2> ord,  
                boost::mqtt5::error_code /*timer_ec*/,
                boost::mqtt5::error_code receive_ec, std::string topic, std::string payload, boost::mqtt5::publish_props
            ) {
                if (ord[0] == 0) { 
                    uint64_t value = 0;
                    std::cout << "[ALARM] Не на зв'язку" << std::endl;
                    auto users = *cm.get<std::vector<std::string>>("tg.users");
                    try {
                        for(auto it = users.begin(); it != users.end(); ++it) {
                            auto result = std::from_chars(it->data(), it->data() + it->size(), value);
                            if (result.ec != std::errc()) {
                                continue;
                            }
                            bot.getApi().sendMessage(value, "[ALARM] Не на зв'язку");
                        }
                    } catch (const TgBot::TgException& e) {
                        std::cerr << "[TG ERROR] Не вдалося відправити повідомлення: " << e.what() << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "[UNKNOWN ERROR] " << e.what() << std::endl;
                    }
                }
                else {
                    if (!receive_ec) {
                        std::cout << "[OK] " << topic << "Повідомлення отримано: " << payload << std::endl;
                    } else {
                        std::cout << "[ERROR] Помилка отримання: " << receive_ec.message() << std::endl;
                    }
                }

                start_monitoring();
            }
        );
    };
    
    start_monitoring();
    ioc.run();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "[ERROR] config filename wasnt specified\n";
        exit(1);
    }

    signal(SIGINT, [](int s) {
        printf("\nSIGINT got\n");
        ioc.stop();
        exit(0);
    });

    try {
        Settings::ConfigManager cm = std::string(argv[1]);
        Bot bot(*cm.get<std::string>("tg.token"));

        std::thread mqtt_thread([&cm, &bot] {
            run_mqtt_monitor(cm, bot);
        });

        printf("Bot username: @%s\n", bot.getApi().getMe()->username.c_str());
        bot.getApi().deleteWebhook();

        TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }

        if (mqtt_thread.joinable()) mqtt_thread.join();

    } catch (exception& e) {
        printf("error: %s\n", e.what());
    }

    return 0;
}
