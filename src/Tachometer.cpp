#include "Tachometer.h"
#include "N2K_router.h"
#include "Utils.h"
#include "Conf.h"
#include "Log.h"

#include <Arduino.h>
#include <vector>

#define RPM_LOG_TAG "RPM"

// frequency calculation
#define PERIOD 500000L
#define PERIOD_H 2000000L
#define FREQ_SMOOTHING_BUFFER_SIZE 2

// simulator
#define UP_CYCLES 9
#define DOWN_CYCLES 5
int sim_counter = 0;
int sim_state = LOW;
int sim_cycles = DOWN_CYCLES;

#ifndef TACHO_TIMER_N
#define TACHO_TIMER_N 0
#endif
#define TACHO_TIMER_DIVIDER 80
#define TACHO_TIMER_ALARM_VALUE 100
#ifndef ENGINE_RPM_SIM_PIN
#define ENGINE_RPM_SIM_PIN -1
#endif
hw_timer_t* timer = NULL;

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
        if (_tach[i]) _tach[i]->read_signal();
    }
}

void init_timer()
{
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
}

double add_and_get_freq(double freq, double* freqs, int &ix)
{
    freqs[ix] = freq;
    ix = (ix + 1) % FREQ_SMOOTHING_BUFFER_SIZE;
    double sum = 0;
    for (int i = 0; i<FREQ_SMOOTHING_BUFFER_SIZE; i++) sum += freqs[i];
    return sum / FREQ_SMOOTHING_BUFFER_SIZE;
}

Tachometer::Tachometer(Context _ctx, uint8_t _pin, uint8_t _poles, double _rpm_ratio, double _rpm_adjustment, uint8_t _timer_n)
    :enabled(false), ctx(_ctx), pin(_pin), poles(_poles), rpm_ratio(_rpm_ratio), rpm_adjustment(_rpm_adjustment),
    last_read(0), last_read_eng_h(0), freq_buffer(NULL), freq_buffer_ix(0), is_setup(false), engine_time(0), counter(0), state(LOW)
{
    freq_buffer = new double[FREQ_SMOOTHING_BUFFER_SIZE];
    memset(freq_buffer, 0, FREQ_SMOOTHING_BUFFER_SIZE * sizeof(double));
}

Tachometer::~Tachometer()
{
    disable();
    delete freq_buffer;
    if (timer) timerEnd(timer);
}

inline void Tachometer::read_signal()
{
    int new_state = digitalRead(pin);
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
        ctx.cache.rpm = 0xFFFF;
        Log::tracex(RPM_LOG_TAG, "Disable", "Success {1}");
        enabled = false;
    }
}

void Tachometer::setup()
{
    if (!is_setup)
    {
        Log::tracex(RPM_LOG_TAG, "Setup", "Pin {%d} Sim {%d}", pin, ENGINE_RPM_SIM_PIN);
        pinMode(pin, INPUT_PULLDOWN);
        init_timer();
        is_setup = true;
        set_engine_time(ctx.conf.get_engine_hours());
        set_adjustment(ctx.conf.get_rpm_adjustment());
    }
}

void Tachometer::loop(unsigned long micros)
{
    unsigned long dT = check_elapsed(micros, last_read, PERIOD);
    if (enabled && dT)
    {
        if (dT > 0) // skip the case where the time goes back to 0
        {
            // calc frequency in Hz
            double freq = add_and_get_freq(((double)counter / (double)dT / 2.0) * 1000000.0, freq_buffer, freq_buffer_ix);
            int rpm = (int)(rpm_adjustment * 60.0 * rpm_ratio * freq / poles);
            //Log::trace("Freq {%.3fHz} Counter {%lu} RPM {%d} D-Time {%lu}\n", freq, counter, rpm, dT);
            counter = 0;
            if (rpm>50 /* engine on */)
            {
                set_engine_time(engine_time + (uint64_t)(dT / 1000), true); // milliseconds
            }
            ctx.cache.rpm = rpm;
            ctx.n2k.sendEngineRPM(0, rpm);
        }
    }
    dT = check_elapsed(micros, last_read_eng_h, PERIOD_H);
    if (enabled && dT)
    {
        if (dT > 0) // skip the case where the time goes back to 0
        {
            // send down engine time in seconds
            uint32_t ts = engine_time / 1000L;
            ctx.n2k.sendEngineHours(0, ts);
        }
    }
}

void Tachometer::calibrate(int rpm)
{
    if (rpm && ctx.cache.rpm)
    {
        double current_rpm = ctx.cache.rpm / rpm_adjustment;
        double new_adj = (double)rpm / current_rpm;
        Log::tracex(RPM_LOG_TAG, "Set RPM adjustment", "RPM {%d} 2RPM {%d} Adj {%.2f} 2Adj {%.2f}", current_rpm, rpm, rpm_adjustment, new_adj);
        set_adjustment(new_adj, true);
    }
}

double Tachometer::get_adjustment()
{
    return rpm_adjustment;
}

void Tachometer::set_adjustment(double adj, bool save)
{
    if (adj>0.1 && adj<5000.0)
    {
      rpm_adjustment = adj;
      if (save) ctx.conf.save_rpm_adjustment(adj);
    }
}

void Tachometer::dumpStats()
{
    uint32_t ts = engine_time / 1000L;
    uint32_t h = ts / 3600;
    uint32_t m = (ts % 3600) / 60;
    uint32_t s = (ts % 60);
    Log::tracex(RPM_LOG_TAG, "Stats", "RPM {%d} Engine Hours {%lu:%02lu:%02lu %lu%03d}", ctx.cache.rpm, h, m, s, (uint32_t)(engine_time/1000),(uint16_t)(engine_time%1000));
}

bool Tachometer::is_enabled()
{
    return enabled;
}

void Tachometer::set_engine_time(uint64_t t, bool save)
{
    engine_time = t;
    ctx.cache.engine_time = engine_time;
    if (save) ctx.conf.save_engine_hours(engine_time);
}

unsigned long Tachometer::get_engine_time()
{
    return engine_time;
}