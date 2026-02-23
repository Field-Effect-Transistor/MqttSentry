#pragma once

#include <boost/mqtt5/logger.hpp>
#include <functional>

/**
 * @brief "Балакучий" клас-логер mqtt клієнта, що дає можливість відслідковувати статус з'єднання
 */
class MqttConnectionMonitor: boost::mqtt5::logger {
public:
    using Callback = std::function<void(void)>;
    using log_level = boost::mqtt5::log_level;
    using error_code = boost::system::error_code;
    using reason_code = boost::mqtt5::reason_code;
    using connack_props = boost::mqtt5::connack_props;

    MqttConnectionMonitor(
        Callback onConnection,
        Callback onDisconnection,
        log_level level = log_level::debug
    ):  logger(level),
        _onConnection(std::move(onConnection)),
        _onDisconnection(std::move(onDisconnection))
    {}

    void at_resolve(
        error_code ec,
        std::string_view host,
        std::string_view port,
        const boost::mqtt5::asio::ip::tcp::resolver::results_type& eps
    ) {
        boost::mqtt5::logger::at_resolve(ec, host, port, eps);
        if (ec) {
            std::cerr << "Brocker gone\n" << std::flush;
            _onDisconnection();
        }
    }

    void at_connack(
        reason_code rc,
        bool session_present,
        const connack_props& ca_props
    ) {
        boost::mqtt5::logger::at_connack(rc, session_present, ca_props);
        if (!rc) {
            _onConnection();
        }
    }



private:
    Callback _onConnection;
    Callback _onDisconnection;
};
