#ifndef _SPEED_SENSOR_ANALOG_H
#define _SPEED_SENSOR_ANALOG_H

#include <stdint.h>

class SpeedSensorAnalog
{
public:
    SpeedSensorAnalog(int pin);
    ~SpeedSensorAnalog();

    unsigned long get_sample_age() const { return last_read_time; }

    void setup();

    int get_counter() const { return counter; }

    bool read_data(unsigned long milliseconds, double &frequency, int &counter_out);

    void loop_micros(unsigned long now_micros);

    void set_alpha(double a) { alpha = a; }
    double get_alpha() const { return alpha; }

    int get_pin() const { return pin; }

    // used for tests
    void read_signal(int state, unsigned long t_micros = 0);

private:
    unsigned long last_read_time = 0;

    unsigned long last_transition_time = 0;
    unsigned long transition_period = 0;
    double transition_period_smoothed = 0.0;

    unsigned long cycles_counter = 0;
    unsigned long counter = 0;
    double smooth_counter = 0.0;
    int state = LOW;

    double alpha = 1.0;

    int pin;
};

#endif
