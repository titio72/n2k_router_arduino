#ifndef _SPEED_THROUGH_WATER_H
#define _SPEED_THROUGH_WATER_H

#include <SpeedSensorInterrupt.h>
#include <SpeedSensor.h>
#include "Agents.hpp"
#include "MsClock.h"
#ifndef NATIVE
#include <esp_timer.h>
#else
#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif
#ifndef esp_timer_handle_t
#define esp_timer_handle_t void*
#endif
#endif

class Context;

class SpeedThroughWater
{
public:
    SpeedThroughWater(int pin);
    ~SpeedThroughWater();

    unsigned long get_sample_age() const { return speed_sensor.get_sample_age(); }

    // Used for testing only! The signal is normally captured by interrupts in the SpeedSensorInterrupt class
    void signal() { /*speed_sensor.signal(); */ }

    AB_AGENT

private:
    //SpeedSensorInterrupt speed_sensor;
    SpeedSensor speed_sensor;
    bool enabled;
    double adjustment_factor = 1.0;
    unsigned long last_read = 0;
    MsClock ms_clock;

    esp_timer_handle_t timer_handle = NULL;
    static void IRAM_ATTR timer_callback(void *arg);
};

#endif