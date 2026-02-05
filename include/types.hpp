//  include/types.hpp
#pragma once

#include <string>

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

struct AlertEvents {
    enum class State {
        online,
        offline,
        error
    } state;
    std::string machine_id;
    std::string message;
    std::string timestamp;
};

template<typename U>
class ThreadSafeQueue {
    std::queue<U> _queue;
    std::condition_variable _cv;
    mutable std::mutex _mutex;

    public:
    void push(const U& value) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(value);
        _cv.notify_one();
    }

    U pop() {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [this]{
            return !_queue.empty();
        });

        auto value = _queue.front();
        _queue.pop();
        return value;
    }

    std::optional<U> try_pop() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_queue.empty()) {
            return std::nullopt;
        }

        auto value = _queue.front();
        _queue.pop();
        return value;
    }

};