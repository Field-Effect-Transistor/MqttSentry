//  src/mqttService.cpp
#include "mqttService.hpp"

#include <stdexcept>
#include <algorithm>

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
): _cm(cm), _ioc(ioc), _isConnected(false) {
    //  creating mqtt client
    _client = std::make_shared<MqttClient>(MqttClient(ioc, {}, boost::mqtt5::logger(boost::mqtt5::log_level::debug)));  

    //  settin up client
    boost::mqtt5::connect_props cprop;
    cprop[boost::mqtt5::prop::session_expiry_interval] = 300;
    _client->connect_properties(cprop);

    auto mqttConfig = _cm.getMqttConfig();
    _client->brokers(mqttConfig.broker, mqttConfig.port).credentials(
        mqttConfig.client_id,
        mqttConfig.client_name,
        mqttConfig.pwd
    );
    
    _retryTimer = std::make_shared<boost::asio::steady_timer>(ioc);
}

void MqttService::start() {
    try {
        _client->async_run([](boost::mqtt5::error_code ec) {
            std::cerr << "[MqttService] Stopped. Reason: " << ec.message() << std::endl;
        });

        this->_subscribeToTopics();
    } catch (const std::exception& e) {
        std::cerr << "[MqttService] start() Error: " << e.what() << std::endl << std::flush;
    }
    //  start recieve loop
    _recieveLoop();
    _ioc.run();
}

void MqttService::_recieveLoop() {
    _client->async_receive(
        [this](
        boost::mqtt5::error_code ec, 
        std::string full_topic, 
        std::string payload, 
        boost::mqtt5::publish_props
    ) {
        if (ec == boost::asio::error::operation_aborted) {
            this->_isConnected = false;
            return;
        }
        if(ec) {
            std::cerr << "[MqttService] Error: " << ec.message() << ". Retrying in 10s...\n";

            if (ec == boost::mqtt5::client::error::session_expired) {
                std::cout << "[MqttService] Session expired. Re-subscribing..." << std::endl;
                this->_subscribeToTopics();
            }
            
            _retryTimer->expires_after(std::chrono::seconds(10));
            _retryTimer->async_wait([this](const boost::system::error_code& timer_ec){
                if (!timer_ec) this->_recieveLoop();
            });
            return;
        }

        this->_isConnected = true;
        
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
                ++i;
            }
            if (i == size) {
                std::cerr << "[MqttService] RecieveLoop warning: from unregistered " << full_topic << " topic recieved\n";
                _recieveLoop();
                return;
            }
        }

        if (theme == "time_light_ECO") {
            try {
                nlohmann::json data = nlohmann::json::parse(payload);
                MachineLight light;
                light.ts = data["ts"];
                light.time_on_eco = data["d"]["time_on_ECO"][0];
                light.time_on_light = data["d"]["time_on_light"][0];
                _onLight(id, light);
            } catch (const std::exception& e) {
                std::cerr << "[MqttService] failed to parse time_light_ECO of" << _cm.resolveHmiName(id) << " with payload " << payload << "cuzz" << e.what() << std::endl << std::flush;
            }
        } else if (theme == "state") {
            watchdog->pet();
            try {
                nlohmann::json data = nlohmann::json::parse(payload);
                MachineState ms;
                ms.state = data["step_pos"][0];
                ms.ts = data["ts"];
                _onMS(id, ms);
            } catch (const std::exception& e) {
                std::cerr << "[MqttService] failed to parse state of " << _cm.resolveHmiName(id) << " with payload " << payload << "cuzz" << e.what() << std::endl;
            }
        } else if(theme == "alarm_kod") {
            unsigned int kod = 0;
            std::string ts = "unknown";

            try {
                nlohmann::json data = nlohmann::json::parse(payload);
                ts = data["ts"];
                std::vector<unsigned int> kod_v = data["alarm_kod"];
                kod = kod_v.at(0);

                auto logic = _cm.getLogicConfig();
                auto& disabled_codes = logic.disabled_codes;
                if (std::find(disabled_codes.begin(), disabled_codes.end(), kod) == disabled_codes.end()) {
                    auto codes = logic.code;
                    if (kod == 0) {
                        _onAlert({AlertEvents::State::fine, _cm.resolveHmiName(id), codes.at(kod), ts});
                    } else {
                        _onAlert({AlertEvents::State::error, _cm.resolveHmiName(id), codes.at(kod), ts});
                    }
                }

            } catch (const std::out_of_range& e) {
                _onAlert({AlertEvents::State::error, _cm.resolveHmiName(id), std::string("Unknown error with code ") + std::to_string(kod), ts});
            } catch (const std::exception& e) {
                std::cerr << "[MqttService] RecieveLoop Error: " << e.what() << std::endl;
            }
        } else if (theme == "in_not_fil") {
            try {
                MachineIn min = nlohmann::json::parse(payload);
                _onMIn(id, min);
            } catch(const std::exception& e) {
                std::cerr << "[MqttService] failed to parse \"in_not_fil\" of" << _cm.resolveHmiName(id) << " with payload " <<  payload << "cuzz" << e.what() << std::endl;
            }
        } else if (theme == "out") {
            try {
                MachineOut mout = nlohmann::json::parse(payload);
                _onMOut(id, mout);
            } catch(const std::exception& e) {
                std::cerr << "[MqttService] failed to parse \"out\" of" << _cm.resolveHmiName(id) << " with payload " <<  payload << "cuzz" << e.what() << std::endl;
            }
        }
        
        _recieveLoop();
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

void MqttService::_subscribeToTopics() {
    //  prepere topics to subs
    auto s_topics = _cm.getMqttConfig().topic;
    std::vector<boost::mqtt5::subscribe_topic> topics;
    topics.reserve(s_topics.size());
    for (auto& w : _watchdogs) {
        w->stop();
    }
    _watchdogs.clear();
    for (const auto& t : s_topics) {
        //  creating watchdogs + givin them callback function so they notify on machine dis- / connections
        auto hmi_id = split(t, '/').at(1);
        auto watchdog = std::make_shared<TopicWatchdog>(
            _ioc,
            hmi_id,
            _cm,
            [this, hmi_id](const WatchdogEvents event){
                if (event == WatchdogEvents::Online) {
                    _onAlert({AlertEvents::State::online, _cm.resolveHmiName(hmi_id), "<b>Зв'язок:</b> знову на зв'язку", "NOW"});
                } else {
                    _onAlert({AlertEvents::State::offline, _cm.resolveHmiName(hmi_id), "<b>Зв'язок:</b> не на зв'язку", "NOW"});
                }
            },
            [this]() {  return this->isConnected(); }
        );
        _watchdogs.push_back(watchdog);
        watchdog->start_timer();

        topics.push_back({t});  //  preparing topics for subs
    }
    
    _client->async_subscribe(
        topics, boost::mqtt5::subscribe_props {},
        [this](boost::mqtt5::error_code ec, std::vector<boost::mqtt5::reason_code>, boost::mqtt5::suback_props) {
            if (!ec) {
                std::cout << "[MqttService] Successfully subscribed via MQTT!" << std::endl;
                this->_isConnected = true;
            }
            else
                std::cerr << "[MqttService] Subscribing error: " << ec.message() << std::endl;
        }
    );
}
