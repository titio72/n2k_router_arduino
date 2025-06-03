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

#define RGB_ON_ERROR 0,16,0
#define RGB_ON 16,0,0
#define RGB_OFF 0,0,0

EVODisplay::EVODisplay() : init(false), display(NULL), enabled(false)
{
    memset(blink_time, 0, NUM_LEDS * sizeof(unsigned long));
    memset(blink_period, 0, NUM_LEDS * sizeof(unsigned long));
    memset(led_state, LOW, NUM_LEDS * sizeof(uint8_t));
    memset(blink_error, false, NUM_LEDS * sizeof(bool));
    pins[LED_PWR] = LED_PIN;
    pins[LED_GPS] = LED_PIN_GPS;
    pins[LED_N2K] = LED_PIN_SPARE;
}

bool is_rgb(int led)
{
    #ifdef ESP32_C3
    return led == LED_N2K;
    #else
    return false;
    #endif
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

        for (int led = 0; led < 3; led++)
        {
            led_state[led] = LOW;
            blink_time[led] = 0;
            blink_period[led] = 0;
            if (pins[led] != -1)
            {
                if (is_rgb((LEDS)led))
                {
                    // nothing to initialize?
                }
                else
                {
                    pinMode(pins[led], OUTPUT);
                    digitalWrite(pins[led], LOW);
                }
                set_on(led, false);
            }
        }
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

void EVODisplay::set_on(int led, bool on, bool error)
{
    int pin = pins[led];
    if (pin!=-1)
    {
        if (is_rgb(led))
        {
            if (on && error)
            {
                neopixelWrite(pin, RGB_ON_ERROR);
            }
            else if (on && !error)
            {
                neopixelWrite(pin, RGB_ON);
            }
            else
            {
                neopixelWrite(pin, RGB_OFF);
            }
        }
        else
        {
            digitalWrite(pin, on?HIGH:LOW);
        }
    }
}

void EVODisplay::on(LEDS led, bool error)
{
    led_state[led] = HIGH;
    blink_time[led] = 0;
    blink_period[led] = 0;
    if (enabled)
    {
        set_on(led, true, error);
    }
}

void EVODisplay::off(LEDS led)
{
    led_state[led] = LOW;
    blink_time[led] = 0;
    blink_period[led] = 0;
    blink_error[led] = false;
    if (enabled)
    {
        set_on(led, false);
    }
}


void EVODisplay::blink(LEDS led, unsigned long now_micros, unsigned long period_on, bool error)
{
    if (enabled && blink_period[led]==0 && blink_time[led]==0 && pins[led]!=-1)
    {
        led_state[led] = HIGH;
        set_on(led, true, error);
        blink_time[led] = now_micros;
        blink_period[led] = period_on;
        blink_error[led] = error;
    }
}

void EVODisplay::loop(unsigned long micros)
{
    if (enabled)
    {
        for (int led = 0; led<3; led++)
        {
            if (blink_period[led]!=0 && blink_time[led]!=0 && check_elapsed(micros, blink_time[led], blink_period[led]))
            {
                led_state[led] = LOW;
                set_on(led, false);
                blink_period[led] = 0;
                blink_time[led] = 0;
            }
        }
    }
}

void EVODisplay::enable()
{
    enabled = true;
    for (int led = 0; led<3; led++)
    {
        set_on(led, (led_state[led]==HIGH));
    }
}

void EVODisplay::disable()
{
    enabled = false;
    for (int led = 0; led<3; led++)
    {
        set_on(led, false);
        blink_time[led] = 0;
        blink_period[led] = 0;
    }
}

bool EVODisplay::is_enabled()
{
    return enabled;
}