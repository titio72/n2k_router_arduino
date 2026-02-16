#ifndef _TEMPERATURE_H
#define _TEMPERATURE_H

struct WaterData;
struct Configuration;

#include "SpeedSensor.h"
#include "Agents.hpp"
#include "Context.h"

class WaterTemperature
{
public:
    WaterTemperature(int pin);
    ~WaterTemperature();

    AB_AGENT

    void read_data(WaterData &data, Configuration &conf);

private:
    int pin = -1;
    double temperature = NAN;
    unsigned long last_read_time = 0;
    double adjustment_factor = 1.0;
    bool enabled = false;
};

#endif