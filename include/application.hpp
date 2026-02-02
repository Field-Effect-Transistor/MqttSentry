//  include/appliation.hpp
#pragma once

#include "mqttService.hpp"
#include "tgService.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <thread>
#include <atomic>

class Application {
    Settings::ConfigManager _cm;
    
    //  thread save queue
    ThreadSafeQueue<AlertEvents> _queue;

    //  tg (consumer)
    std::thread _tgThread;
    TgService _tg;
    std::thread _workerThread;
    void _workerLoop();

    //  mqtt (producer)
    boost::asio::io_context _ioc;
    MqttService _mqtt;

    boost::asio::signal_set _signals;       //  for Sign INT and TERM


    std::atomic<bool> _running;

    public:
    Application(const std::string& configFilePath);
    ~Application();

    void run();
    void stop();
};
