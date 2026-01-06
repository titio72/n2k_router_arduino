#include "Leds.h"
#include <Log.h>
#include <Utils.h>
#ifndef DISABLE_LEDS
#include <Arduino.h>
#endif
#define RGB_ON_ERROR 0,16,0
#define RGB_ON 16,0,0
#define RGB_OFF 0,0,0

#ifndef LED_RGB_N2K
#define LED_RGB_N2K 0
#endif
#ifndef LED_RGB_GPS
#define LED_RGB_GPS 0
#endif
#ifndef LED_RGB
#define LED_RGB 0
#endif

//#define DISABLE_LEDS

Leds::Leds() : init(false), enabled(false)
{
    #ifndef DISABLE_LEDS
    memset(blink_time, 0, NUM_LEDS * sizeof(unsigned long));
    memset(blink_period, 0, NUM_LEDS * sizeof(unsigned long));
    memset(led_state, LOW, NUM_LEDS * sizeof(uint8_t));
    memset(blink_error, false, NUM_LEDS * sizeof(bool));
    pins[LED_PWR] = LED_PIN;
    pins[LED_GPS] = LED_PIN_GPS;
    pins[LED_N2K] = LED_PIN_N2K;
    rgb[LED_PWR] = LED_RGB;
    rgb[LED_GPS] = LED_RGB_GPS;
    rgb[LED_N2K] = LED_RGB_N2K;
    #endif
}

Leds::~Leds()
{
}

void Leds::setup(Context &ctx)
{
    #ifndef DISABLE_LEDS
    if (!init)
    {
        init = true;
        Log::trace("[LED] Setup {%d}\n", init);

        for (int led = 0; led < 3; led++)
        {
            led_state[led] = LOW;
            blink_time[led] = 0;
            blink_period[led] = 0;
            if (pins[led] != -1)
            {
                if (rgb[led])
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
    #endif
}

void Leds::set_on(int led, bool on, bool error)
{
    #ifndef DISABLE_LEDS
    int pin = pins[led];
    if (pin!=-1)
    {
        if (rgb[led])
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
    #endif
}

void Leds::on(LEDS led, bool error)
{
    #ifndef DISABLE_LEDS
    led_state[led] = HIGH;
    blink_time[led] = 0;
    blink_period[led] = 0;
    if (enabled)
    {
        set_on(led, true, error);
    }
    #endif
}

void Leds::off(LEDS led)
{
    #ifndef DISABLE_LEDS
    led_state[led] = LOW;
    blink_time[led] = 0;
    blink_period[led] = 0;
    blink_error[led] = false;
    if (enabled)
    {
        set_on(led, false);
    }
    #endif
}


void Leds::blink(LEDS led, unsigned long now_micros, unsigned long period_on, bool error)
{
    #ifndef DISABLE_LEDS
    if (enabled && blink_period[led]==0 && blink_time[led]==0 && pins[led]!=-1)
    {
        led_state[led] = HIGH;
        set_on(led, true, error);
        blink_time[led] = now_micros;
        blink_period[led] = period_on;
        blink_error[led] = error;
    }
    #endif
}

void Leds::loop(unsigned long micros, Context &ctx)
{
    #ifndef DISABLE_LEDS
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
    #endif
}

void Leds::enable()
{
    #ifndef DISABLE_LEDS
    if (!enabled)
    {
        enabled = true;
        for (int led = 0; led<3; led++)
        {
            set_on(led, (led_state[led]==HIGH));
        }
        Log::tracex("LED", "Enable", "Success {%d}", enabled);
    }
    #endif
}

void Leds::disable()
{
    #ifndef DISABLE_LEDS
    if (enabled)
    {
        enabled = false;
        for (int led = 0; led<3; led++)
        {
            set_on(led, false);
            blink_time[led] = 0;
            blink_period[led] = 0;
        }
        Log::tracex("LED", "Disable", "Success {%d}", !enabled);
    }
    #endif
}

bool Leds::is_enabled()
{
    return enabled;
}