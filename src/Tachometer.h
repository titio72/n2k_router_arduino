#ifndef _Tachometer_H
#define _Tachometer_H

#include "Utils.h"
#include "Agents.hpp"

class EngineHours;

class Tachometer
{
public:
    Tachometer(uint8_t pin, EngineHours *eng_hours, uint8_t poles = 12, double ratio = 1.5, double adjustment = 1.0, uint8_t timer_n = 0);
    ~Tachometer();

    void dumpStats();

    // called by the timer!
    void read_signal(int state);

    AB_AGENT

    inline int get_pin() { return pin; }

    // for testing purposes
    bool is_tacho_registered() const;
    unsigned long get_counter() const { return counter; }


private:
    bool enabled;

    EngineHours *engine_hours_svc;

    uint8_t pin;
    uint8_t poles;
    double rpm_ratio;

    int current_rpm;
    uint64_t current_engine_time;

    unsigned long last_read;
    unsigned long last_read_eng_h;

    double *freq_buffer;
    int freq_buffer_ix;
    bool is_setup;

    unsigned long counter;
    int state;
};

#endif