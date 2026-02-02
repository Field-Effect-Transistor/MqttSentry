//  include/mqttService.hpp
#pragma once

#include <functional>

#include <boost/mqtt5/mqtt_client.hpp>
#include <boost/asio/io_context.hpp>

#include "config.hpp"
#include "types.hpp"
#include "topicWatchdog.hpp"

class MqttService {
    public:
    using OnAlertCallback = std::function<void(const AlertEvents)>;
    
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
    std::shared_ptr<boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket>> _client;
    std::vector<std::shared_ptr<TopicWatchdog>> _watchdogs;
    OnAlertCallback _onAlert;

    //void onMessageReceived(const std::string& topic, const std::string& payload);
    void recieveLoop();
    //std::optional<AlertEvents> parser(const std::vector<std::string>& message);
    //void subscribeToTopics();
};

