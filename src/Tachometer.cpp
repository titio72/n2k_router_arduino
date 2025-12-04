#include "Tachometer.h"
#include "N2K_router.h"
#include "Utils.h"
#include "Conf.h"
#include "Log.h"

#include <vector>

#ifndef NATIVE
#include <Arduino.h>
#endif
#ifndef LOW
#define LOW 0x00
#define HIGH 0x01
#endif

#define RPM_LOG_TAG "RPM"

// frequency calculation
#define PERIOD 500000L // Period for RPM calculation
#define PERIOD_H 2000000L // Period for sending engine hours
#define FREQ_SMOOTHING_BUFFER_SIZE 3 // Smooth over the last N periods

#pragma region simulator
#define UP_CYCLES 9
#define DOWN_CYCLES 5
int sim_counter = 0;
int sim_state = LOW;
int sim_cycles = DOWN_CYCLES;


#ifndef ENGINE_RPM_SIM_PIN
#define ENGINE_RPM_SIM_PIN -1
#endif
#pragma endregion simulator

#pragma region timers_management
#ifndef TACHO_TIMER_N
#define TACHO_TIMER_N 0
#endif
#define TACHO_TIMER_DIVIDER 80
#define TACHO_TIMER_ALARM_VALUE 100

std::vector<Tachometer*> _tach = {};

void add_tacho(Tachometer* tachometer)
{
    for (int i = 0; i<_tach.size(); i++)
    {
        if (_tach[i]==NULL)
        {
            _tach[i] = tachometer;
            return;
        }
    }
    _tach.push_back(tachometer);
}

void remove_tacho(Tachometer* tachometer)
{
    for (int i = 0; i<_tach.size(); i++)
    {
        if (_tach[i]==tachometer) _tach[i] = NULL;
    }
}

bool contains_tacho(const Tachometer* tachometer)
{
        for (int i = 0; i<_tach.size(); i++)
    {
        if (_tach[i]==tachometer) return true;
    }
    return false;
}

#ifndef NATIVE
hw_timer_t* timer = NULL;

void IRAM_ATTR on_timer()
{
    #if (ENGINE_RPM_SIM_PIN >= 0)
    sim_counter++;
    if (sim_counter == sim_cycles)
    {
        sim_counter = 0;
        sim_state = sim_state == LOW ? HIGH : LOW;
        digitalWrite(ENGINE_RPM_SIM_PIN, sim_state);
        sim_cycles = (sim_state == HIGH) ? UP_CYCLES : DOWN_CYCLES;
    }
    #endif
    //-----------------------------------------------------
    for (int i = 0; i<_tach.size(); i++)
    {
        if (_tach[i])
        {
            int new_state = digitalRead(_tach[i]->get_pin());
            _tach[i]->read_signal(new_state);
        }
    }
}
#endif

void init_timer()
{
    #ifndef PIO_UNIT_TESTING
    #ifndef NATIVE
    if (timer==NULL)
    {
        #if (ENGINE_RPM_SIM_PIN >= 0)
        pinMode(ENGINE_RPM_SIM_PIN, OUTPUT);
        digitalWrite(ENGINE_RPM_SIM_PIN, LOW);
        #endif

        timer = timerBegin(TACHO_TIMER_N, TACHO_TIMER_DIVIDER, true); // 10Khz
        timerAttachInterrupt(timer, on_timer, true);
        timerAlarmWrite(timer, TACHO_TIMER_ALARM_VALUE, true);
        timerAlarmEnable(timer);
        Log::tracex("Tacho", "Set timer", "Timer {%d} initialized at {%d Hz}", TACHO_TIMER_N, getCpuFrequencyMhz() * 1000L * TACHO_TIMER_ALARM_VALUE / TACHO_TIMER_DIVIDER);
    }
    #endif
    #endif
}

#pragma endregion timers_management

double add_and_get_freq(double freq, double* freqs, int &ix)
{
    freqs[ix] = freq;
    ix = (ix + 1) % FREQ_SMOOTHING_BUFFER_SIZE;
    double sum = 0;
    for (int i = 0; i<FREQ_SMOOTHING_BUFFER_SIZE; i++) sum += freqs[i]; // for small buffers, it is more efficient to sum, rather than subtract and add
    return sum / FREQ_SMOOTHING_BUFFER_SIZE;
}

Tachometer::Tachometer(uint8_t _pin, EngineHours *eng, uint8_t _poles, double _rpm_ratio, double _rpm_adjustment, uint8_t _timer_n)
    :enabled(false), pin(_pin), poles(_poles), rpm_ratio(_rpm_ratio), current_rpm(0), engine_hours_svc(eng),
    last_read(0), last_read_eng_h(0), freq_buffer(NULL), freq_buffer_ix(0), is_setup(false), current_engine_time(0), counter(0), state(LOW)
{
    freq_buffer = new double[FREQ_SMOOTHING_BUFFER_SIZE];
    memset(freq_buffer, 0, FREQ_SMOOTHING_BUFFER_SIZE * sizeof(double));
}

Tachometer::~Tachometer()
{
    disable();
    delete freq_buffer;
    #ifndef PIO_UNIT_TESTING
    #ifndef NATIVE
    if (timer) timerEnd(timer);
    #endif
    #endif
}

bool Tachometer::is_tacho_registered() const
{
    return contains_tacho(this);
}

void Tachometer::read_signal(int new_state)
{
    if (new_state != state)
    {
        state = new_state;
        counter++;
    }
}

void Tachometer::enable()
{
    if (!enabled && is_setup)
    {
        add_tacho(this);
        enabled = true;
        Log::tracex(RPM_LOG_TAG, "Enable", "Success {%d}", enabled);
    }
}

void Tachometer::disable()
{
    if (enabled)
    {
        remove_tacho(this);
        Log::tracex(RPM_LOG_TAG, "Disable", "Success {1}");
        enabled = false;
    }
}

void Tachometer::setup(Context &ctx)
{
    if (!is_setup)
    {
        Log::tracex(RPM_LOG_TAG, "Setup", "Pin {%d} Sim {%d}", pin, ENGINE_RPM_SIM_PIN);
        #ifndef PIO_UNIT_TESTING
        #ifndef NATIVE
        pinMode(pin, INPUT);
        init_timer();
        #endif
        #endif
        is_setup = true;
    }
}

void Tachometer::loop(unsigned long micros, Context &ctx)
{
    unsigned long dT = check_elapsed(micros, last_read, PERIOD);
    if (dT)
    {
        if (enabled)
        {
            if (dT > 0) // skip the case where the time goes back to 0
            {
                // calc frequency in Hz
                double freq = add_and_get_freq(((double)counter / (double)dT / 2.0) * 1000000.0, freq_buffer, freq_buffer_ix);
                int rpm = (int)(ctx.conf.get_rpm_adjustment() * 60.0 * rpm_ratio * freq / poles);
                //Log::trace("Freq {%.3fHz} Counter {%lu} RPM {%d} D-Time {%lu}\n", freq, counter, rpm, dT);
                counter = 0;
                if (rpm>200 /* engine on */ && engine_hours_svc)
                {
                    current_engine_time = engine_hours_svc->get_engine_hours() + (uint64_t)(dT / 1000);
                    engine_hours_svc->save_engine_hours(current_engine_time); // milliseconds
                }
                ctx.data_cache.engine.rpm = rpm;
                ctx.data_cache.engine.engine_time = current_engine_time;
                ctx.n2k.sendEngineRPM(0, rpm);
                current_rpm = rpm;
            }
        }
        else
        {
            ctx.data_cache.engine.rpm = 0;
            ctx.data_cache.engine.engine_time = 0;
        }
    }
    dT = check_elapsed(micros, last_read_eng_h, PERIOD_H);
    if (enabled && dT)
    {
        if (dT > 0) // skip the case where the time goes back to 0
        {
            // send down engine time in seconds
            uint32_t ts = engine_hours_svc->get_engine_hours() / 1000L;
            ctx.n2k.sendEngineHours(0, ts);
        }
    }
}

void Tachometer::dumpStats()
{
    uint32_t ts = current_engine_time / 1000L;
    uint32_t h = ts / 3600;
    uint32_t m = (ts % 3600) / 60;
    uint32_t s = (ts % 60);
    Log::tracex(RPM_LOG_TAG, "Stats", "RPM {%d} Engine Hours {%lu:%02lu:%02lu %lu%03d}", current_rpm, h, m, s, (uint32_t)(current_engine_time/1000),(uint16_t)(current_engine_time%1000));
}

bool Tachometer::is_enabled()
{
    return enabled;
}
