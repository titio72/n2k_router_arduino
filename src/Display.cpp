#include "Display.h"
#include <Log.h>

#include <Arduino.h>

#if (DO_DISPLAY == 1)
#include <Adafruit_SSD1306.h>
#include "TwoWireProvider.h"
#endif

#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 32    // OLED display height, in pixels


EVODisplay::EVODisplay() : init(false), display(NULL), enabled(false)
{
}

EVODisplay::~EVODisplay()
{
#if (DO_DISPLAY == 1)
    if (display)
        delete display;
#endif
}

void EVODisplay::setup()
{
    if (!init)
    {
#if (DO_DISPLAY == 1)
        display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, TwoWireProvider::get_two_wire(), OLED_RESET);
        init = display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
        if (init)
            display->clearDisplay();
#else
        init = true;
#endif
        Log::trace("[DS] Initialized {%d}\n", init);
    }
}

void EVODisplay::draw_text(const char *text, ...)
{
#if (DO_DISPLAY == 1)
    if (enabled && init && text)
    {
        va_list args;
        char t[64];
        va_start(args, text);
        vsnprintf(t, 63, text, args);
        va_end(args);
        display->clearDisplay();
        display->setTextSize(2);              // Normal 1:1 pixel scale
        display->setTextColor(SSD1306_WHITE); // Draw white text
        display->setCursor(0, 0);             // Start at top-left corner
        display->cp437(true);                 // Use full 256 char 'Code Page 437' font
        display->write(t, strlen(t));
        display->display();
    }
#endif
}

void EVODisplay::loop(unsigned long micros)
{
}

void EVODisplay::enable()
{
    if (!enabled && init)
    {
        enabled = true;
        Log::tracex("DS", "Enable", "Success {%d}", enabled);
    }

}

void EVODisplay::disable()
{
    if (enabled)
    {
        #if (DO_DISPLAY == 1)
        if (display)
            display->clearDisplay();
        #endif
        enabled = false;
        Log::tracex("DS", "Disable", "Success {%d}", !enabled);
    }
}

bool EVODisplay::is_enabled()
{
    return enabled;
}