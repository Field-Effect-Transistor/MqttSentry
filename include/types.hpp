//  include/types.hpp
#pragma once

#include <string>

#include <queue>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <optional>

#include <nlohmann/json.hpp>

struct AlertEvents {
    enum class State {
        fine,
        online,
        offline,
        error,
        pump_on,
        pump_off,
        eco_malfunction
    } state;
    std::string machine_id;
    std::string message;
    std::string timestamp;
};

struct MachineState {
    unsigned short state;
    std::string ts;
};

struct MachineLight {
    unsigned long int time_on_eco;
    unsigned long int time_on_light;
    std::string ts;
};

struct MachineIn {
    bool in_holod;
    bool in_holod_ver;
    bool in_night;
    bool in_pres_NF;
    bool in_svitlo;
    std::string ts;
};

/**
 * @brief Десеріалізація JSON-об'єкта (nlohmann::json) у структуру MachineIn
 * 
 * @param j [in] JSON-об'єкт, у якого будуть вичитані дані
 * @param min [out]  структура, у яку дані будуть записані
 */
inline void from_json(const nlohmann::json& j, MachineIn& min) {
    int buff = j["in_holod_nf"][0];
    min.in_holod = buff;
    buff = j["in_holod_ver_nf"][0];
    min.in_holod_ver = buff;
    buff = j["in_night_nf"][0];
    min.in_night = buff;
    buff = j["in_pres_nf_NF"][0];
    min.in_pres_NF = buff;
    buff = j["in_svitlo_nf"][0];
    min.in_svitlo = buff;
    min.ts = j["ts"];
}

struct MachineOut {
    bool out_ECO1;
    bool out_ECO2;
    bool out_RB_blok;
    bool out_air;
    bool out_alarm_sig;
    bool out_osn_klap_NF;
    std::string ts;
};

/**
 * @brief Десеріалізація JSON-об'єкта (nlohmann::json) у структуру MachineOut
 * 
 * @param j [in] JSON-об'єкт, у якого будуть вичитані дані
 * @param min [out]  структура, у яку дані будуть записані
 */
inline void from_json(const nlohmann::json& j, MachineOut& mout) {
    int buff = j["out_ECO1"][0];
    mout.out_ECO1 = buff;
    buff = j["out_ECO2"][0];
    mout.out_ECO2 = buff;
    buff = j["out_RB_blok"][0];
    mout.out_RB_blok = buff;
    buff = j["out_air"][0];
    mout.out_air = buff;
    buff = j["out_alarm_sig"][0];
    mout.out_alarm_sig = buff;
    buff = j["out_osn_klap_NF"][0];
    mout.out_osn_klap_NF = buff;
    mout.ts = j["ts"];
}

//  ethalone threadsafe template queue container class
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

//  ethalone threadsafe template map container class
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

/**
 * @brief Структура для запису двох останніх відомих станів
 */
template<typename T>
struct History {
    T current;
    T previous;
    
    /**
    * @brief оновлює 2 останні значення
    */
    void update(const T& next) {
        previous = std::move(current);
        current = next;
    }
};
