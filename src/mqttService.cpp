//  src/mqttService.cpp
#include "mqttService.hpp"

#include <stdexcept>

inline std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

MqttService::MqttService(
    Settings::ConfigManager& cm,
    boost::asio::io_context& ioc
): _cm(cm), _ioc(ioc) {
    //  creating mqtt client
    _client = std::make_shared<boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket>>(ioc);
}

void MqttService::start() {
    //  prepere topics to subs
    auto s_topics = *_cm.get<std::vector<std::string>>("mqtt.topic");
    std::vector<boost::mqtt5::subscribe_topic> topics;
    topics.reserve(s_topics.size());
    _watchdogs.clear();
    for (const auto& t : s_topics) {
        //  creating watchdogs + givin them callback function so they notify on machine dis- / connections
        auto hmi_id = split(t, '/').at(1);
        auto watchdog = std::make_shared<TopicWatchdog>(_ioc, hmi_id, _cm, [this, hmi_id](const WatchdogEvents event){
            if (event == WatchdogEvents::Online) {
                _onAlert({hmi_id, "Machine is back online", "NOW"});
            } else {
                _onAlert({hmi_id, "Machine is offline!!!", "NOW"});
            }
        });
        _watchdogs.push_back(watchdog);
        watchdog->start_timer();

        topics.push_back({t});  //  preparing topics for subs
    }

    //  settin up client
    auto mqttConfig = _cm.getMqttConfig();
    _client->brokers(mqttConfig.broker, mqttConfig.port)
        .credentials(
            mqttConfig.client_id,
            mqttConfig.client_name,
            mqttConfig.pwd
        );

    _client->async_run([](boost::mqtt5::error_code ec) {
        std::cerr << "[MqttService] Stopped. Reason: " << ec.message() << std::endl;
    }); 
    
    _client->async_subscribe(
        topics, boost::mqtt5::subscribe_props {},
        [](boost::mqtt5::error_code ec, std::vector<boost::mqtt5::reason_code>, boost::mqtt5::suback_props) {
            if (!ec)
                std::cout << "[MqttService] Successfully subscribed via MQTT!" << std::endl;
            else
                std::cerr << "[MqttService] Subscribing error: " << ec.message() << std::endl;
        }
    );

    //  start recieve loop
    recieveLoop();
    _ioc.run();
}

void MqttService::recieveLoop() {
    _client->async_receive(
        [this](
        boost::mqtt5::error_code ec, 
        std::string full_topic, 
        std::string payload, 
        boost::mqtt5::publish_props
    ) {
        if (ec == boost::asio::error::operation_aborted) return;

        if(ec) {
            std::cerr << "[MqttService] RecieveLoop error: " << ec.message() << std::endl;
            this->recieveLoop();
            return;
        }

        //  extracting id and theme (state, alarm_kod, out, etc.) from topic
        auto topics = split(full_topic, '/');
        if (topics.size() != 3) {
            std::cerr << "[MqttService] RecieveLoop warning: from " << full_topic << " topic recieved\n";
        }
        auto& id = topics.at(1);
        auto& theme = topics.at(2);

        std::shared_ptr<TopicWatchdog> watchdog;
        {   //  looking for watchdog 
            size_t size = _watchdogs.size(), i = 0;
            while (i < size) {
                if(_watchdogs.at(i)->get_hmi_id() == id) {
                    watchdog = _watchdogs.at(i);
                    break;
                }
            }
            if (i == size) {
                std::cerr << "[MqttService] RecieveLoop warning: from unregistered " << full_topic << " topic recieved\n";
                recieveLoop();
                return;
            }
        }
        watchdog->pet();

        if(theme == "alatm_kod") {
            unsigned int kod;
            std::string ts = "unknown";

            try {
                nlohmann::json data = nlohmann::json::parse(payload);
                ts = data["ts"];
                std::vector<unsigned int> kod_v = data["alarm_kod"];
                kod = kod_v.at(0);

                if (kod) {
                    auto codes = _cm.getLogicConfig().code;
                    _onAlert({id, codes.at(kod), ts});
                }

            } catch (const std::out_of_range& e) {
                _onAlert({id, "Unknown error with code " + kod, ts});
            } catch (const std::exception& e) {
                std::cerr << "[MqttService] RecieveLoop Error: " << e.what() << std::endl;
            }
        }
        
        recieveLoop();
    });
}

void MqttService::stop() {
    std::cout << "[MqttService] Stopping\n";
    _watchdogs.clear();
    if(_client) {
        _client->async_disconnect(
            [](boost::mqtt5::error_code ec){
                if(!ec) {
                    std::cerr << "[MqttService] Disconnect error: " << ec.message() << std::endl;
                } else {
                    std::cout << "[MqttService] Disconnected\n";
                }
            }       
        );
    }
}
