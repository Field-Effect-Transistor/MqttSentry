//  src/main.cpp

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <thread>
#include <iostream>
#include <charconv>
#include <sstream>

#include <tgbot/tgbot.h>

#include <boost/mqtt5/mqtt_client.hpp>
#include <boost/mqtt5/types.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/experimental/parallel_group.hpp>

#include "config.hpp"
#include "topicWatchdog.hpp"

using namespace std;
using namespace TgBot;

boost::asio::io_context ioc;

std::vector<std::string> split(const std::string& s, char delimiter);
void run_mqtt_monitor(
    Settings::ConfigManager& cm,
    Bot& bot
);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "[ERROR] config filename wasnt specified\n";
        return 1;
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
            try{
                run_mqtt_monitor(cm, bot);
            } catch(...) {
                std::cerr<< "\n[ERROR] crash mqtt_thread\n";
            }
            
        });

        bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
            if (StringTools::startsWith(message->text, "/start")) {
                std::cout << "[WARN] NEW USER CANDIDATE: " << message->from->id << std::endl;
                return;
            }
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

    std::cout << "App finished\n";

    return 0;
}

void run_mqtt_monitor(
    Settings::ConfigManager& cm,
    Bot& bot
) {
    auto c = std::make_shared<boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket>>(ioc);

    std::vector<std::shared_ptr<TopicWatchdog>> watchdogs;

    auto s_topics = *cm.get<std::vector<std::string>>("mqtt.topic");
    std::vector<boost::mqtt5::subscribe_topic> topics;
    topics.reserve(s_topics.size());
    for (const auto& t : s_topics) {
        auto watchdog = std::make_shared<TopicWatchdog>(ioc, split(t, '/').at(1), cm, bot);
        watchdogs.push_back(watchdog);
        watchdog->start_timer();

        topics.push_back({t});
    }

    c->brokers(*cm.get<std::string>("mqtt.broker"), *cm.get<unsigned short>("mqtt.port"))
        .credentials(
            *cm.get<std::string>("mqtt.client_id"),
            *cm.get<std::string>("mqtt.client_name"),
            *cm.get<std::string>("mqtt.pwd")
        ).async_run([](boost::mqtt5::error_code ec) {
         
            std::cout << "[MQTT Client] Stopped. Reason: " << ec.message() << std::endl;
        });    
    
    c->async_subscribe(
            topics, boost::mqtt5::subscribe_props {},
            [](boost::mqtt5::error_code ec, std::vector<boost::mqtt5::reason_code>, boost::mqtt5::suback_props) {
                if (!ec) std::cout << "Successfully subscribed via MQTT!" << std::endl;
        }
    );
    

    std::function<void()> loop;
    loop = [&c, watchdogs, &loop, &cm, &bot]{
        c->async_receive(
            [c, watchdogs, &loop, &cm, &bot](
                boost::mqtt5::error_code ec, 
                std::string full_topic, 
                std::string payload, 
                boost::mqtt5::publish_props
            ) {
                if (!ec) {
                    std::cout << payload << std::endl;
                    auto topics = split(full_topic, '/');
                    if (topics.size() != 3) {
                        std::cerr << "[WARN] " + full_topic + " should look like NordFrost/MACHINE_ID/topic\n";
                    } else {
                        auto& id = topics.at(1);
                        auto& topic = topics.at(2);

                        size_t i = 0;
                        while (i < watchdogs.size()) {
                            if (watchdogs.at(i)->get_hmi_id() == id)
                                break;
                            ++i;
                        }
                        auto& watchdog = watchdogs.at(i);
                        watchdog->pet();
                        
                        //! dont forget to change alatm_kod
                        if (topic == "alatm_kod") {
                            unsigned int kod;
                            std::string ts;
                            try {
                                nlohmann::json data = nlohmann::json::parse(payload);
                                ts = data["ts"];
                                //unsigned int kod = data["alarm_kod"];
                                std::vector<unsigned int> kod_v = data["alarm_kod"];
                                kod = kod_v.at(0);

                                if (kod) {
                                    watchdog->_is_alarm = true;
                                    std::stringstream ss;
                                    ss  << "[ALARM] on " << id << ' ' << cm.get<std::unordered_map<unsigned int,std::string>>("logic.code")->at(kod) << std::endl 
                                        << "timestamp: " << ts << std::endl;
                                    std::string s = ss.str();
                                    
                                    auto users = *cm.get<std::vector<std::string>>("tg.users");
                                    for (auto it = users.begin(); it != users.end(); ++it) {
                                        bot.getApi().sendMessage(std::stoul(*it), s);
                                    }            
                                    std::cout << s;
                                } else if (watchdog->_is_alarm) {
                                    watchdog->_is_alarm = false;
                                    auto users = *cm.get<std::vector<std::string>>("tg.users");
                                    for (auto it = users.begin(); it != users.end(); ++it) {
                                        bot.getApi().sendMessage(
                                            std::stoul(*it),
                                            "[ALARM] Release on " + id + "\n timestamp: " + ts + '\n' 
                                        );
                                    }

                                }
                            } catch (const std::out_of_range& e) {
                                auto users = *cm.get<std::vector<std::string>>("tg.users");
                                for (auto it = users.begin(); it != users.end(); ++it) {
                                    bot.getApi().sendMessage(
                                        std::stoul(*it),
                                        "[ALARM] on " + id + " with unknown code: " + std::to_string(kod) + "\n timestamp: " + ts + '\n' 
                                    );
                                }
                            } catch (const std::exception& e) {
                                std::cerr << "[ERROR] on " << full_topic << ":" << e.what() << std::endl;
                            }
                        }
                    }

                } 
                
                loop();
            }
        );
    };

    loop();
    ioc.run();
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}
