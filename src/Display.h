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

private:
    bool init;
    Adafruit_SSD1306* display;

    bool enabled;
};

#endif