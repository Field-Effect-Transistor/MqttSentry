//  include/topicWatchdog.hpp
#pragma once

#include <memory>
#include <boost/asio/steady_timer.hpp>
#include <tgbot/tgbot.h>

#include "config.hpp"

class TopicWatchdog : public std::enable_shared_from_this<TopicWatchdog> {
    private:
    std::shared_ptr<boost::asio::steady_timer> _timer;
    std::string _hmi_id;
    Settings::ConfigManager& _cm;
    TgBot::Bot& _bot;
    unsigned int _timeouts;

    public:
    bool _is_alarm;

    TopicWatchdog(
        boost::asio::io_context& ioc,
        const std::string& hmi_id,
        Settings::ConfigManager& cm,
        TgBot::Bot& bot
    );

    ~TopicWatchdog(){}

    void pet();
    void start_timer();
    inline const std::string& get_hmi_id() { return _hmi_id; };

    private:
    void on_timeout();
};
