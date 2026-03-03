//  include/graphiteService.hpp
#pragma once

#include <boost/asio.hpp>
#include <atomic>

#include "types.hpp"

class GraphiteService {
public:
    void push(Metric& m);

    GraphiteService(
        const std::string&          host,
        const std::string&          port,
        boost::asio::io_context&    ioc
    );    
    ~GraphiteService();

private:
    boost::asio::io_context&        _ioc;
    boost::asio::ip::tcp::socket    _socket;
    boost::asio::ip::tcp::resolver  _resolver;
    boost::asio::steady_timer       _timer;

    std::string _host;
    std::string _port;

    std::string _buff;

    void _do_connect();
    void _do_write();

    std::atomic<bool>   _is_connecting;
    std::atomic<bool>   _is_writing;    
    ThreadSafeQueue<Metric> _queue;

};
