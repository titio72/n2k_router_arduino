#ifndef _TEMPERATURE_H
#define _TEMPERATURE_H

struct WaterData;
struct Configuration;

#include "SpeedSensor.h"

class WaterTemperature
{
public:
    WaterTemperature(int pin);
    ~WaterTemperature();

    unsigned long get_sample_age() const { return last_read_time; }

    void setup();

    void read_data(WaterData &data, Configuration &conf, unsigned long milliseconds);

    void loop_micros(unsigned long now_micros);

private:
    int pin;
    double temperature;
    unsigned long last_read_time = 0;
    double adjustment_factor = 1.0;
};

#endif