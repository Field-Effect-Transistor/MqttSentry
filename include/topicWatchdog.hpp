//  include/topicWatchdog.hpp
#pragma once

#include <memory>
#include <boost/asio/steady_timer.hpp>
#include <tgbot/tgbot.h>

#include "config.hpp"

class TopicWatchdog : public std::enable_shared_from_this<TopicWatchdog> {
    private:
    std::shared_ptr<boost::asio::steady_timer> _timer;
    std::string _topic;
    Settings::ConfigManager& _cm;
    TgBot::Bot& _bot;
    unsigned int _timeouts;

    public:
    TopicWatchdog(
        boost::asio::io_context& ioc,
        const std::string& topic,
        Settings::ConfigManager& cm,
        TgBot::Bot& bot
    );

    ~TopicWatchdog(){}

    void pet();
    void start_timer();
    inline const std::string& getTopic() { return _topic; };

    //static inline std::vector<std::string> split(const std::string& s, char delimiter);

    private:
    void on_timeout();
};
