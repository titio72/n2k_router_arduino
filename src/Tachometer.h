#ifndef _Tachometer_H
#define _Tachometer_H

#include "Utils.h"
#include "Agents.hpp"
#include <SpeedSensorInterrupt.h>

class EngineHours;
class SpeedSensor;

class Tachometer
{
public:
    Tachometer(uint8_t pin, EngineHours *eng_hours, uint8_t poles = 12, double ratio = 1.5, double adjustment = 1.0);
    ~Tachometer();

    void dumpStats();

    // Used for testing only! The signal is normally captured by interrupts in the SpeedSensorInterrupt class
    void read_signal();

    AB_AGENT

    // for testing purposes
    bool is_tacho_registered() const;

private:
    bool enabled;

    EngineHours *engine_hours_svc;

    uint8_t poles;
    double rpm_ratio;

    int current_rpm;
    uint64_t current_engine_time;

    unsigned long last_read;
    unsigned long last_read_eng_h;

    SpeedSensorInterrupt speed_sensor;

    double *freq_buffer;
    int freq_buffer_ix;
    bool is_setup;
};

#endif