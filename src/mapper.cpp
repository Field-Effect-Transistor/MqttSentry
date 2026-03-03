//  src/mapper.cpp
#include "mapper.hpp"

#include "utils.hpp"

void Mapper::map(const std::string& id, const MachineState& toMap, std::function<void(Metric)> sink) {
    Metric metric;
    metric.path = _cm.resolvePath(id) + ".state.step_pos";
    metric.value = toMap.state;
    metric.ts = stringToUnix(toMap.ts);

    sink(metric);
}

void Mapper::map(const std::string& id, const MachineTimeLightECO& toMap, std::function<void(Metric)> sink) {
    Metric metric;
    auto path = _cm.resolvePath(id) + ".time_light_ECO.";
    metric.ts = stringToUnix(toMap.ts);
    
    metric.path = path + "time_on_ECO";
    metric.value = toMap.time_on_eco;
    sink(metric);

    metric.path = path + "time_on_light";
    metric.value = toMap.time_on_light;
    sink(metric);
}

void Mapper::map(const std::string& id, const MachineInNotFilter& toMap, std::function<void(Metric)> sink) {
    Metric metric;
    auto path = _cm.resolvePath(id) + ".in_not_fil.";
    metric.ts = stringToUnix(toMap.ts);

    metric.path = path + "in_holod_nf";
    metric.value = toMap.in_holod;
    sink(metric);

    metric.path = path + "in_holod_ver_nf";
    metric.value = toMap.in_holod_ver;
    sink(metric);

    metric.path = path + "in_night_nf";
    metric.value = toMap.in_night;
    sink(metric);

    metric.path = path + "in_pres_nf_NF";
    metric.value = toMap.in_pres_NF;
    sink(metric);

    metric.path = path + "in_svitlo_nf";
    metric.value = toMap.in_svitlo;
    sink(metric);
}

void Mapper::map(const std::string& id, const MachineInFilter& toMap, std::function<void(Metric)> sink) {
    Metric metric;
    auto path = _cm.resolvePath(id) + ".in_filter.";
    metric.ts = stringToUnix(toMap.ts);

    metric.path = path + "in_holod_f";
    metric.value = toMap.in_holod;
    sink(metric);

    metric.path = path + "in_holod_ver_f";
    metric.value = toMap.in_holod_ver;
    sink(metric);

    metric.path = path + "in_night_f";
    metric.value = toMap.in_night;
    sink(metric);

    metric.path = path + "in_pres_f_NF";
    metric.value = toMap.in_pres_NF;
    sink(metric);

    metric.path = path + "in_svitlo_f";
    metric.value = toMap.in_svitlo;
    sink(metric);
}

void Mapper::map(const std::string& id, const MachineOut& toMap, std::function<void(Metric)> sink) {
    Metric metric;
    auto path = _cm.resolvePath(id) + ".out.";
    metric.ts = stringToUnix(toMap.ts);

    metric.path = path + "out_ECO1";
    metric.value = toMap.out_ECO1;
    sink(metric);

    metric.path = path + "out_ECO2";
    metric.value = toMap.out_ECO2;
    sink(metric);

    metric.path = path + "out_RB_blok";
    metric.value = toMap.out_RB_blok;
    sink(metric);

    metric.path = path + "out_air";
    metric.value = toMap.out_air;
    sink(metric);

    metric.path = path + "out_alarm_sig";
    metric.value = toMap.out_alarm_sig;
    sink(metric);

    metric.path = path + "out_osn_klap_NF";
    metric.value = toMap.out_osn_klap_NF;
    sink(metric);
}

void Mapper::map(const std::string& id, const MachineAlarmKod& toMap, std::function<void(Metric)> sink) {
    Metric metric;
    metric.path = _cm.resolvePath(id) + ".alarm_kod.alarm_kod";
    metric.value = toMap.alarm_kod;
    metric.ts = stringToUnix(toMap.ts);

    sink(metric);
}
    