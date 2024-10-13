#include "Display.h"
#include "Log.h"

#include <Arduino.h>

#if (DO_DISPLAY == 1)
#include <Adafruit_SSD1306.h>
#include "TwoWireProvider.h"
#endif

#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 32    // OLED display height, in pixels


EVODisplay::EVODisplay() : init(false), display(NULL), blink_time(0), blink_period(0), enabled(false), led_state(LOW)
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

        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN, LOW);
        led_state = LOW;
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
        display->write(text, strlen(text));
        display->display();
    }
#endif
}

void EVODisplay::blink(unsigned long micros, unsigned long period_on)
{
    if (enabled && blink_period==0 && blink_time==0)
    {
        led_state = HIGH;
        digitalWrite(LED_PIN, HIGH);
        blink_time = micros;
        blink_period = period_on;
    }
}

void EVODisplay::loop(unsigned long micros)
{
    if (enabled && blink_period!=0 && blink_time!=0 && check_elapsed(micros, blink_time, blink_period))
    {
        led_state = LOW;
        digitalWrite(LED_PIN, LOW);
        blink_period = 0;
        blink_time = 0;
    }
}

void EVODisplay::enable()
{
    enabled = true;
}

void EVODisplay::disable()
{
    enabled = false;
}

bool EVODisplay::is_enabled()
{
    return enabled;
}