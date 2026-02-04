//  include/mqttService.hpp
#pragma once

#include <functional>

#include <boost/mqtt5/mqtt_client.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/mqtt5/logger.hpp>

#include "config.hpp"
#include "types.hpp"
#include "topicWatchdog.hpp"

class MqttService {
    public:

    using OnAlertCallback = std::function<void(const AlertEvents)>;
    using MqttClient = boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket, std::monostate, boost::mqtt5::logger>;
    
    MqttService(
        Settings::ConfigManager& cm,
        boost::asio::io_context& ioc
    );
    ~MqttService() { stop(); } 

    void start();
    void stop();

    void setOnAlert(OnAlertCallback onAlert) { _onAlert = onAlert; };

    private:
    Settings::ConfigManager& _cm;
    boost::asio::io_context& _ioc;
    std::shared_ptr<MqttClient> _client;
    std::vector<std::shared_ptr<TopicWatchdog>> _watchdogs;
    OnAlertCallback _onAlert;

    std::shared_ptr<boost::asio::steady_timer> _retryTimer;

    //void onMessageReceived(const std::string& topic, const std::string& payload);
    void recieveLoop();
    //std::optional<AlertEvents> parser(const std::vector<std::string>& message);
    //void subscribeToTopics();
};

