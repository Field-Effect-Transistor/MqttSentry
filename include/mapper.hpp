//  include/mapper.hpp
#pragma once

#include "types.hpp"
#include "settings.hpp"

#include <functional>

class Mapper {
    Settings::ConfigManager&    _cm;

public:
    Mapper(Settings::ConfigManager& cm): _cm(cm){};

    void map(const std::string& id, const MachineState& ms, std::function<void(Metric)> sink);
    void map(const std::string& id, const MachineTimeLightECO& ml, std::function<void(Metric)> sink);
    void map(const std::string& id, const MachineInNotFilter& mIn, std::function<void(Metric)> sink);
    void map(const std::string& id, const MachineInFilter& mIn, std::function<void(Metric)> sink);
    void map(const std::string& id, const MachineOut& mOut, std::function<void(Metric)> sink);
    void map(const std::string& id, const MachineAlarmKod& ma, std::function<void(Metric)> sink);
};

