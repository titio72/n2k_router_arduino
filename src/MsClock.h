#ifndef _MS_CLOCK_H
#define _MS_CLOCK_H

#include <stdint.h>

/**
 * Milliseconds derived from a free-running 32-bit microsecond counter.
 *
 * micros() wraps every 2^32 us (~71.6 min), and micros() / 1000 then jumps back from 4294967 to 0, which is
 * not a multiple of 2^32, so subtracting two of those values across the wrap gives a huge bogus interval.
 * This clock accumulates wrap-safe microsecond deltas instead, so the millisecond value is continuous
 * (it wraps only at 2^32 ms, and unsigned subtraction handles that correctly).
 * update() must be called at least once per wrap period (~71 min).
 */
class MsClock
{
public:
    uint32_t update(uint32_t now_micros)
    {
        if (!started)
        {
            started = true;
            ms = now_micros / 1000;
            rem = now_micros % 1000;
        }
        else
        {
            rem += (uint32_t)(now_micros - last); // modular subtraction: correct across the wrap
            ms += rem / 1000;
            rem %= 1000;
        }
        last = now_micros;
        return ms;
    }

private:
    bool started = false;
    uint32_t last = 0;
    uint32_t rem = 0;
    uint32_t ms = 0;
};

#endif
