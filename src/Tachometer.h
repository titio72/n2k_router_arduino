#ifndef _Tachometer_H
#define _Tachometer_H

#include "Utils.h"
#include "Context.h"

class Adafruit_BMP280;
class TachometerPrivate;

class Tachometer
{
public:
    Tachometer(Context ctx, EngineData& data, uint8_t pin, uint8_t poles = 12, double ratio = 1.5, double adjustment = 1.0, uint8_t timer_n = 0);
    ~Tachometer();

    void dumpStats();

    // called by the timer!
    void read_signal();

    void set_engine_time(uint64_t t, bool save = false);
    unsigned long get_engine_time();

    void calibrate(int rpm);

    double get_adjustment();
    void set_adjustment(double adj, bool save = false);

    AB_AGENT

private:
    bool enabled;
    Context ctx;
    EngineData& data;
    uint8_t pin;
    uint8_t poles;
    double rpm_ratio;
    double rpm_adjustment;

    unsigned long last_read;
    unsigned long last_read_eng_h;
    double *freq_buffer;
    int freq_buffer_ix;
    bool is_setup;

    unsigned long counter;
    int state;

    // engine time in seconds
    uint64_t engine_time;
};

#endif