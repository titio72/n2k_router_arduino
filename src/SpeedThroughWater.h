#ifndef _SPEED_THROUGH_WATER_H
#define _SPEED_THROUGH_WATER_H

struct water_data;
struct configuration;

#include <SpeedSensorInterrupt.h>
#include "Agents.hpp"

class Context;

class SpeedThroughWater
{
public:
    SpeedThroughWater(int pin);
    ~SpeedThroughWater();

    unsigned long get_sample_age() const { return speed_sensor.get_sample_age(); }

    // Used for testing only! The signal is normally captured by interrupts in the SpeedSensorInterrupt class
    void signal() { speed_sensor.signal(); }

    AB_AGENT

private:
    SpeedSensorInterrupt speed_sensor;
    bool enabled;
    double adjustment_factor = 1.0;
    unsigned long last_read = 0;
};

#endif