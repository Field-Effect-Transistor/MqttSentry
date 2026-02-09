//  include/types.hpp
#pragma once

#include <string>

#include <queue>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
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

struct MachineState {
    unsigned short state;
    std::string ts;
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

template<typename K, typename V>
class ThreadSafeMap {
private:
    std::unordered_map<K, V> _map;
    mutable std::shared_mutex _mutex;

public:
    void set(const K& key, const V& value) {
        std::unique_lock lock(_mutex);
        _map[key] = value;
    }

    std::optional<V> get(const K& key) const {
        std::shared_lock lock(_mutex);
        auto it = _map.find(key);
        if (it != _map.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool erase(const K& key) {
        std::unique_lock lock(_mutex);
        return _map.erase(key) > 0;
    }

    bool contains(const K& key) const {
        std::shared_lock lock(_mutex);
        return _map.find(key) != _map.end();
    }

};
