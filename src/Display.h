#ifndef DISPLAY_H
#define DISPLAY_H

#include "Utils.h"

class Adafruit_SSD1306;

class EVODisplay {

public:
    EVODisplay();
    ~EVODisplay();

    AB_AGENT

    void draw_text(const char* text, ...);

    void blink(unsigned long ms, unsigned long period_on);

private:
    bool init;
    Adafruit_SSD1306* display;

    uint8_t led_state;

    unsigned long blink_time;
    unsigned long blink_period;
    bool enabled;
};


#endif