#include "ResetButton.h"
#include <Log.h>
#ifndef NATIVE
#include <Arduino.h>
#endif

#define RESET_LOG_TAG "RESET"

static bool default_pin_reader(int pin)
{
#ifndef NATIVE
    return digitalRead(pin) == LOW;
#else
    return false;
#endif
}

static void default_restarter()
{
#ifndef NATIVE
    delay(50); // let the log drain
    ESP.restart();
#endif
}

ResetButton::ResetButton(int _pin, unsigned long _hold_usec, PinReader reader, Restarter restarter)
    : pin(_pin), hold_usec(_hold_usec),
      read_pin(reader ? reader : default_pin_reader), restart(restarter ? restarter : default_restarter),
      enabled(false), armed(false), pressed(false), press_start(0)
{
}

void ResetButton::setup(Context &ctx)
{
#ifndef NATIVE
    if (pin != -1)
    {
        pinMode(pin, INPUT_PULLUP);
    }
#endif
    Log::tracex(RESET_LOG_TAG, "Setup", "Pin {%d} Hold {%lu} micros", pin, hold_usec);
}

void ResetButton::enable(Context &ctx)
{
    if (!enabled && pin != -1)
    {
        enabled = true;
        armed = false;
        pressed = false;
        Log::tracex(RESET_LOG_TAG, "Enable", "Success {%d}", enabled);
    }
}

void ResetButton::disable(Context &ctx)
{
    if (enabled)
    {
        enabled = false;
        pressed = false;
        Log::tracex(RESET_LOG_TAG, "Disable", "Success {%d}", !enabled);
    }
}

bool ResetButton::is_enabled()
{
    return enabled;
}

void ResetButton::loop(unsigned long time_micros, Context &ctx)
{
    if (!enabled)
        return;

    if (!read_pin(pin))
    {
        pressed = false;
        armed = true;
    }
    else if (armed)
    {
        if (!pressed)
        {
            pressed = true;
            press_start = time_micros;
        }
        else if (time_micros - press_start >= hold_usec)
        {
            do_restart();
        }
    }
}

void ResetButton::do_restart()
{
    Log::tracex(RESET_LOG_TAG, "Restart", "Reset button held");
    pressed = false;
    armed = false;
    if (before_restart)
        before_restart();
    restart();
}
