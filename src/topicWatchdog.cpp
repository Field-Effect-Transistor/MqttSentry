//  src/topicWatchdog.cpp
#include "topicWatchdog.hpp"

#include <stdexcept>
#include <nlohmann/json.hpp>
#include <sstream>

TopicWatchdog::TopicWatchdog(
    boost::asio::io_context& ioc,
    const std::string& hmi_id,
    Settings::ConfigManager& cm,
    WatchdogCallback callback,
    ConnectionProvider cp
) : _hmi_id(hmi_id),  _cm(cm), _timeouts(0), _callback(callback), _cp(cp) {
    _timer = std::make_shared<boost::asio::steady_timer>(ioc);
    std::cout << "[TopicWatchdog:" << hmi_id << "] created\n";
}

void TopicWatchdog::pet() {
    _timer->cancel();
    if (_timeouts) {
        _callback(WatchdogEvents::Online);
        _timeouts = 0;
    }

    start_timer();
}

void TopicWatchdog::start_timer() {
    _timer->expires_after(std::chrono::seconds(_cm.getLogicConfig().timeout));

    auto self = shared_from_this();
    _timer->async_wait([self](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted) {
            return;
        }

        if (!ec) {
            self->on_timeout();
        }
    });
}

void TopicWatchdog::on_timeout() {
    if(_cp) {
        start_timer();
        return;
    }

    auto timeout_limit = _cm.getLogicConfig().timeout_limit;
    std::cout << "TopicWatchdog::on_timeout() "<< _timeouts << '/' << timeout_limit << std::endl;

    if (_timeouts++ < timeout_limit) {
        _callback(WatchdogEvents::Offline);
        start_timer();
    }
}

void TopicWatchdog::stop() {
    if (_timer) {
        _timer->cancel();
    }
}
