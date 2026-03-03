//  src/graphiteService.cpp
#include "graphiteService.hpp"

#include <memory>
#include <iostream>

namespace asio = boost::asio;

GraphiteService::GraphiteService(
    const std::string&  host,
    const std::string&  port,
    asio::io_context&   ioc
):  _ioc(ioc),
    _socket(ioc),
    _resolver(ioc),
    _timer(_ioc),
    _host(host),
    _port(port),
    _is_connecting(false),
    _is_writing(false)
{
    std::cout << "[GraphiteService] Created\n";
    _buff.reserve(512);
    _do_connect();
}

GraphiteService::~GraphiteService() {
    std::cout << "[GraphiteService] Destructed\n";
}

void GraphiteService::_do_connect() {
    if (_is_connecting)
        return;
    _is_connecting = true;

    std::cout << "[GraphiteService] _do_connect called\n";
    _resolver.async_resolve(_host, _port, [this](auto ec, auto results){
        if (!ec) {
            asio::async_connect(_socket, results, [this](auto ec, auto endpoint) {
                _is_connecting = false;
                if (!ec) {
                    std::cout << "[GraphiteService] Connected to " << _host << ":" << _port << std::endl;
                    if (!_queue.empty()) {
                        _do_write();
                    }
                } else {
                    std::cerr << "[GraphiteService] Connection failed via " << ec.message() << std::endl << std::flush;
                    _timer.expires_after(std::chrono::seconds(10));
                    _timer.async_wait([this](...) {
                        _do_connect();
                    });
                }
                
            });
        } else {
            std::cerr << "[GraphiteService] Host resolving failed via " << ec.message() << std::endl << std::flush;
            _is_connecting = false;
        }
    });
}

void GraphiteService::_do_write() {
    if (_is_writing || _queue.empty())
        return;
    _is_writing = true;
    std::cout << "[GraphiteService] _do_write called\n";
    if (_queue.empty())
        std::cout << "[GraphiteService] _queue empty\n";

    while (_queue.empty() || _buff.size() < 1024) {
        auto metric = _queue.try_pop();
        if (metric == std::nullopt)
            break;
        _buff += metric->path + ' ' + std::to_string(metric->value) + ' ' + std::to_string(metric->ts) + '\n';
    }

    auto write_buff = std::make_shared<std::string>(std::move(_buff));

    asio::async_write(_socket, asio::buffer(*write_buff), [this, write_buff](auto ec, ...) {
        _is_writing = false;
        if (!ec) {
            _buff.clear();
            if (!_queue.empty())
                _do_write();
        } else {
            _socket.close();
            _do_connect();
        }
    });
    
}

void GraphiteService::push(Metric& m) {
    _queue.push(m);
    if (!_is_writing || _queue.empty()) {
        _do_write();
    }
}
