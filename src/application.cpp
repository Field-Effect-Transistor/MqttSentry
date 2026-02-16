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

    //  STARTING TELEGRAM SERVICE
    _tg.set_getMachineState([this](const std::string& id, MachineState& ms) {
        if (auto _ms = _msMap.get(id); _ms != std::nullopt)
            ms = *_ms;
        else {
            ms = {0, "Немає даних"};
        }
    });
    _tg.set_getMachineLight([this](const std::string& id, MachineLight& ml) {
        if (auto _l = _lightMap.get(id); _l != std::nullopt) {
            ml = *_l;
        } else {
            ml = {0, 0, "Немає даних"};
        }
    });
    _workerThread = std::thread(&Application::_workerLoop, this);
    _tgThread = std::thread([this](){
        _tg.runLongPoll();
    });

    //  STARTING MQTT SERVICE
    _mqtt.setOnAlert([this](const AlertEvents alert){
        if (auto prev = _alertMap.get(alert.machine_id); prev == std::nullopt || prev->state != alert.state) {
            _alertQueue.push(alert);
        }
        _alertMap.set(alert.machine_id, alert);
    });
    _mqtt.setOnMSCallback([this](const std::string& id, const MachineState ms) {
        auto prev = _msMap.get(id);
        if (prev.has_value()) {
            if (prev->state == 0 && (ms.state == 1 || ms.state == 2)) {
                _alertQueue.push({
                    AlertEvents::State::pump_on,
                    _cm.resolveHmiName(id),
                    "Увімкнено зимовий режим",
                    ms.ts
                });
            } else if (ms.state == 0 && (prev->state == 1 || prev->state == 2)) {
                    _alertQueue.push({
                    AlertEvents::State::pump_off,
                    _cm.resolveHmiName(id),
                    "Вимкнено зимовий режим",
                    ms.ts
                });
            }
        }
        try {
            _msMap.set(id, ms);
        } catch (const std::exception& e) {
            std::cerr << "[App] " << e.what() << std::endl << std::flush;
        }
        
    });
    _mqtt.setOnLightCallback([this](const std::string id, const MachineLight l){
        try {
            _lightMap.set(id,l);
        } catch(const std::exception& e) {
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
