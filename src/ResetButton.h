#ifndef RESET_BUTTON_H
#define RESET_BUTTON_H

#include "Agents.hpp"

/**
 * Reboots the device when a push button (wired between RESET_PIN and GND, active low)
 * is held for RESET_HOLD_USEC.
 * A button that is already held when the agent is enabled is ignored until released,
 * so a stuck or floating line cannot cause a reboot loop.
 */
class ResetButton
{
public:
    typedef bool (*PinReader)(int pin); // true when the button is pressed
    typedef void (*Restarter)();
    typedef void (*BeforeRestart)();

    /**
     * @param reader / restarter hardware hooks, replaced by mocks in tests
     */
    ResetButton(int pin, unsigned long hold_usec = RESET_HOLD_USEC, PinReader reader = nullptr, Restarter restarter = nullptr);

    /** Called just before the board restarts, e.g. to persist state that would otherwise be lost. */
    void set_before_restart(BeforeRestart callback) { before_restart = callback; }

    AB_AGENT

private:
    int pin;
    unsigned long hold_usec;
    PinReader read_pin;
    Restarter restart;
    BeforeRestart before_restart = nullptr;

    bool enabled;
    bool armed;    // button seen released since enabled
    bool pressed;  // press in progress
    unsigned long press_start;

    void do_restart();
};

#endif
