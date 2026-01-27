//  src/main.cpp

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>

#include <tgbot/tgbot.h>

#include <boost/mqtt5/mqtt_client.hpp>
#include <boost/mqtt5/types.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>

using namespace std;
using namespace TgBot;

//#include "dotenv.hpp"
//#include <fstream>

#include "config.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "[ERROR] config filename wasnt specified\n";
        exit(1);
    }

    Settings::ConfigManager cm = std::string(argv[1]);

    Bot bot(*cm.get<std::string>("tg.token"));
    bot.getEvents().onCommand("start", [&bot, &cm](Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Hi!");
        cm.addUser(std::to_string(message->from->id));
    });
    bot.getEvents().onAnyMessage([&bot](Message::Ptr message) {
        printf("User @%s wrote %s\n", message->from->username.c_str(),  message->text.c_str());
        if (StringTools::startsWith(message->text, "/start")) {
            return;
        }
        bot.getApi().sendMessage(message->chat->id, "Your message is: " + message->text);
    });

    signal(SIGINT, [](int s) {
        printf("\nSIGINT got\n");
        exit(0);
    });

    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        bot.getApi().deleteWebhook();

        TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    } catch (exception& e) {
        printf("error: %s\n", e.what());
    }

    return 0;
}

/*
int main() {
    Bot bot(getenv("token"));
    bot.getEvents().onCommand("start", [&bot](Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Hi!");
    });
    bot.getEvents().onAnyMessage([&bot](Message::Ptr message) {
        printf("User wrote %s\n", message->text.c_str());
        if (StringTools::startsWith(message->text, "/start")) {
            return;
        }
        bot.getApi().sendMessage(message->chat->id, "Your message is: " + message->text);
    });

    signal(SIGINT, [](int s) {
        printf("SIGINT got\n");
        exit(0);
    });

    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        bot.getApi().deleteWebhook();

        TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    } catch (exception& e) {
        printf("error: %s\n", e.what());
    }

    return 0;
}

int main() {
    std::ifstream envFile(".env");
    load_env(envFile);

	boost::asio::io_context ioc;

	boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket> c(ioc);

    char    *mqtt_port  = getenv("mqtt-port"),
            *mqtt_client_name = getenv("mqtt-client-name"),
            *mqtt_pwd = getenv("mqtt-pwd");
	c.brokers(getenv("mqtt-broker"), !(mqtt_port) ? atoi(mqtt_port) : 1883)
		.credentials(getenv("client-id"),
                    !mqtt_client_name ? mqtt_client_name : "",
                    !mqtt_pwd ? mqtt_pwd : ""            
        ).async_run(boost::asio::detached);

	c.async_publish<boost::mqtt5::qos_e::at_most_once>(
		getenv("mqtt-topic"), "Hello world!",
		boost::mqtt5::retain_e::no, boost::mqtt5::publish_props {},
		[&c](boost::mqtt5::error_code ec) {
			std::cout << ec.message() << std::endl;
			c.async_disconnect(boost::asio::detached);
		}
	);
	
	ioc.run();
}
*/