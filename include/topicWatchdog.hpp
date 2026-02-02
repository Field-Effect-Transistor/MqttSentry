//  include/topicWatchdog.hpp
#pragma once

#include <memory>
#include <boost/asio/steady_timer.hpp>
#include <tgbot/tgbot.h>
#include <functional>

#include "config.hpp"

enum class WatchdogEvents {
    Online,
    Offline
};

using WatchdogCallback = std::function<void(const WatchdogEvents)>;

class TopicWatchdog : public std::enable_shared_from_this<TopicWatchdog> {
    private:
    std::shared_ptr<boost::asio::steady_timer> _timer;
    std::string _hmi_id;
    Settings::ConfigManager& _cm;
    unsigned int _timeouts;
    WatchdogCallback _callback;    

    public:
    TopicWatchdog(
        boost::asio::io_context& ioc,
        const std::string& hmi_id,
        Settings::ConfigManager& cm,
        WatchdogCallback callback
    );

    ~TopicWatchdog(){}

    void pet();
    void start_timer();
    inline const std::string& get_hmi_id() const { return _hmi_id; };

    private:
    void on_timeout();
};
