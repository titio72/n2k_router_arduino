#ifndef LEDS_H
#define LEDS_H

#include "Utils.h"

enum LEDS
{
    LED_PWR = 0,
    LED_GPS = 1,
    LED_N2K = 2,
};
#define NUM_LEDS 3

class Leds {

public:
    Leds();
    ~Leds();

    AB_AGENT

    void blink(LEDS led, unsigned long ms, unsigned long period_on, bool error = false);

    void switchLed(LEDS led, bool is_on, bool error = false)
    {
        if (is_on)
            on(led, error);
        else
            off(led);
    }

    void on(LEDS led, bool error = false);
    void off(LEDS led);

private:
    bool init;

    uint8_t led_state[NUM_LEDS];

    unsigned long blink_time[NUM_LEDS];
    unsigned long blink_period[NUM_LEDS];
    bool blink_error[NUM_LEDS];
    int pins[NUM_LEDS];
    bool rgb[NUM_LEDS];
    bool enabled;

    void set_on(int led, bool on, bool error = false);
};


#endif