#ifndef NATIVE
#include <Arduino.h>
#endif
#include "SpeedSensorAnalog.h"
#include "Utils.h"

static const int USE_PERIOD = 0; // 0=use counts, 1=use period

SpeedSensorAnalog::SpeedSensorAnalog(int p) : pin(p), counter(0), state(LOW)
{
}

SpeedSensorAnalog::~SpeedSensorAnalog()
{
}

bool SpeedSensorAnalog::read_data(unsigned long milliseconds, double &frequency, int& counter_out)
{
    counter_out = counter;

    unsigned long dt = milliseconds - last_read_time;
    last_read_time = milliseconds;

    if (dt > 50 && pin >= 0) // arbitrary 50ms interval between two readings (it should be 250ms)
    {
        if (USE_PERIOD)
        {
            transition_period_smoothed = transition_period * alpha + transition_period_smoothed * (1.0 - alpha);
            frequency = (transition_period_smoothed > 2000)  ? (1000000.0 / transition_period_smoothed) : 0; // in Hz
            //transition_period = 50000000L;
            counter = 0;
            cycles_counter = 0;
        }
        else
        {   
            smooth_counter = (double)counter * alpha + smooth_counter * (1.0 - alpha);
            frequency = smooth_counter * 1000.0 / (double)dt / 2.0; // in Hz
            
            counter = 0;
            cycles_counter = 0;
        }
        return true;
    }
    else
    {
        return false;
    }
}

// the time is in micros! called from an ISR every 1ms
void SpeedSensorAnalog::loop_micros(unsigned long t)
{

    #ifndef NATIVE
    if (pin >= 0)
    {
        cycles_counter++;
        uint16_t value = analogRead(pin); // read the value to clear the interrupt, the actual value is read in read_signal() which is called from the main loop to avoid doing analogRead in an ISR
        int new_state = (value > 3500) ? HIGH : ((value<3500) ? LOW : state); // add some hysteresis to avoid bouncing around the threshold value
        if (new_state != state)
        {
            read_signal(new_state, t);
        }
    }
    #endif
}

void SpeedSensorAnalog::read_signal(int new_state, unsigned long t_micros)
{
    if (t_micros - last_transition_time >= 100)
    {
        counter++;
        state = new_state;
        transition_period = t_micros - last_transition_time;
        last_transition_time = t_micros;
    }
}

void SpeedSensorAnalog::setup()
{
    #ifndef NATIVE
    if (pin >= 0)
        pinMode(pin, INPUT);
    #endif
}
