#ifndef DISPLAY_H
#define DISPLAY_H

#include "Utils.h"

class Adafruit_SSD1306;

enum LEDS
{
    LED_PWR = 0,
    LED_GPS = 1,
    LED_N2K = 2,
};

class EVODisplay {

public:
    EVODisplay();
    ~EVODisplay();

    AB_AGENT

    void draw_text(const char* text, ...);

    void blink(LEDS led, unsigned long ms, unsigned long period_on);

    void on(LEDS led);
    void off(LEDS led);

private:
    bool init;
    Adafruit_SSD1306* display;

    uint8_t led_state[3];

    unsigned long blink_time[3];
    unsigned long blink_period[3];
    int pins[3];
    bool enabled;

    void set_on(int led, bool on);
};


#endif