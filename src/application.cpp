//  src/application.cpp
#include "application.hpp"

Application::Application(const std::string& configFilePath)
    :_cm(configFilePath),
    _tg(_cm),
    _signals(_ioc),
    _mqtt(_cm, _ioc)

{
    _signals.add(SIGINT);
    _signals.add(SIGTERM);
    _signals.async_wait(
        [this](boost::system::error_code ec, int signal_number) {
            if (!ec) {
                std::cout << "\n[App] Signal " << signal_number 
                          << " received. Initiating graceful shutdown...\n";
                
                this->stop();
            }
        }
    );

}

Application::~Application() {
    stop();
}

void Application::run() {
    //  starting tg
    _workerThread = std::thread(&Application::_workerLoop, this);
    _tgThread = std::thread([this](){
        _tg.runLongPoll();
    });

    //  starting mqtt
    _mqtt.setOnAlert([this](const AlertEvents alert){
        _queue.push(alert);
    });
    _mqtt.start();

    std::cout << "[App] App running...\n";
    _ioc.run();
    std::cout << "[App] Main Loop finished\n";
}

void Application::stop() {
    if (!_running.exchange(false)) {
        return;
    }

    _tg.stop();
    _mqtt.stop();
    _ioc.stop();
    _queue.push({});

    if (_workerThread.joinable()) {
        _workerThread.join();
        std::cout << "[App] Worker thread stopped.\n";
    }

    if (_tgThread.joinable()) {
        _tgThread.join(); 
        std::cout << "[App] Telegram thread stopped.\n";
    }

}

void Application::_workerLoop() {
    while(_running) {
        try {
            _tg.sendAlert(_queue.pop());
        } catch (const std::exception& e) {
            std::cerr << "[App] Failed to sent alert: " << e.what() << std::endl;
        }
    }
}