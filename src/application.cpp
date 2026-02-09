//  src/application.cpp
#include "application.hpp"

Application::Application(const std::string& configFilePath)
    :_cm(configFilePath),
    _tg(_cm),
    _mqtt(_cm, _ioc),
    _signals(_ioc),
    _running(true)
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

    _tg.set_getMachineState([this](const std::string& id, MachineState& ms) {
        if (_msMap.get(id) != std::nullopt)
            ms = *_msMap.get(id);
        else {
            ms = {0, "Немає даних"};
        }
    });
    _workerThread = std::thread(&Application::_workerLoop, this);
    _tgThread = std::thread([this](){
        _tg.runLongPoll();
    });

    //  starting mqtt
    _mqtt.setOnAlert([this](const AlertEvents alert){
        std::cout << "[ThreadSafeQueue] New value pushed " << alert.machine_id
            << '|' << alert.message
            << '|' << alert.timestamp << std::endl << std::flush;
        _alertQueue.push(alert);
    });
    _mqtt.setOnMSCallback([this](const std::string& id, const MachineState ms) {
        try {
            _msMap.set(id, ms);
        } catch (const std::exception& e) {
            std::cerr << "[App] " << e.what() << std::endl << std::flush;
        }
        
    });
    _mqtt.start();

    std::cout << "[App] App running...\n" << std::flush;
    _ioc.run();
    std::cout << "[App] Main Loop finished\n" << std::flush;
}

void Application::stop() {
    if (!_running.exchange(false)) {
        return;
    }

    _tg.stop();
    _mqtt.stop();
    _ioc.stop();
    _alertQueue.push({});

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
    std::cout << "[Worker] Thread started!" << std::endl << std::flush;
    
    while(_running) {
        try {
            auto alert = _alertQueue.pop(); 
            
            if (alert.machine_id.empty() && alert.message.empty()) break;

            std::cout << "[Worker] Processing alert from " << alert.machine_id << "..." << std::endl;
            
            _tg.sendAlert(alert);
            
            std::cout << "[Worker] Alert sent successfully!" << std::endl << std::flush;

        } catch (const std::exception& e) {
            std::cerr << "[Worker] Failed to send alert: " << e.what() << std::endl << std::flush;
        }
    }
    std::cout << "[Worker] Thread finished." << std::endl << std::flush;
}
