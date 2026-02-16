#include "Tachometer.h"
#include "N2K_router.h"
#include "Utils.h"
#include "Conf.h"
#include <Log.h>

#define RPM_LOG_TAG "RPM"

// frequency calculation
#define PERIOD 1000000L // Period for RPM calculation
#define PERIOD_H 2000000L // Period for sending engine hours
#define FREQ_SMOOTHING_BUFFER_SIZE 3 // Smooth over the last N periods

double add_and_get_freq(double freq, double* freqs, int &ix)
{
    freqs[ix] = freq;
    ix = (ix + 1) % FREQ_SMOOTHING_BUFFER_SIZE;
    double sum = 0;
    for (int i = 0; i<FREQ_SMOOTHING_BUFFER_SIZE; i++) sum += freqs[i]; // for small buffers, it is more efficient to sum, rather than subtract and add
    return sum / FREQ_SMOOTHING_BUFFER_SIZE;
}

Tachometer::Tachometer(uint8_t _pin, EngineHours *eng, uint8_t _poles, double _rpm_ratio, double _rpm_adjustment)
    :enabled(false), poles(_poles), rpm_ratio(_rpm_ratio), current_rpm(0), engine_hours_svc(eng),
    last_read(0), last_read_eng_h(0), freq_buffer(NULL), freq_buffer_ix(0), is_setup(false), current_engine_time(0), speed_sensor(_pin)
{
    freq_buffer = new double[FREQ_SMOOTHING_BUFFER_SIZE];
    memset(freq_buffer, 0, FREQ_SMOOTHING_BUFFER_SIZE * sizeof(double));
}

Tachometer::~Tachometer()
{
    delete[] freq_buffer;
}

bool Tachometer::is_tacho_registered() const
{
    return true; //contains_tacho(speed_sensor);
}

void Tachometer::enable(Context &ctx)
{
    if (!enabled && is_setup)
    {
        enabled = true;
        Log::tracex(RPM_LOG_TAG, "Enable", "Success {%d}", enabled);
    }
}

void Tachometer::disable(Context &ctx)
{
    if (enabled)
    {
        Log::tracex(RPM_LOG_TAG, "Disable", "Success {1}");
        enabled = false;
    }
}

void Tachometer::setup(Context &ctx)
{
    if (!is_setup)
    {
        Log::tracex(RPM_LOG_TAG, "Setup", "Pin {%d}", speed_sensor.get_pin());
        speed_sensor.setup();
        is_setup = true;
        current_engine_time = engine_hours_svc->get_engine_hours();
        ctx.data_cache.engine.engine_time = current_engine_time;
        Log::tracex(RPM_LOG_TAG, "Setup", "Engine Hours loaded {%lu.%03d}", (uint32_t)(current_engine_time / 1000), (uint16_t)(current_engine_time % 1000));
    }
}

bool is_engine_on(int rpm)
{
    return rpm > 200;
}

void Tachometer::read_signal()
{
    speed_sensor.signal();
}

void Tachometer::loop(unsigned long micros, Context &ctx)
{
    if (!enabled)
    {
        ctx.data_cache.engine.rpm = 0;
        ctx.data_cache.engine.engine_time = 0;
        return;
    }
    unsigned long dT = check_elapsed(micros, last_read, PERIOD);
    if (dT > 0)
    {
        double fr = NAN;
        int cnt = 0;
        // calc frequency in Hzs
        bool ok = speed_sensor.read_data(micros / 1000L, fr, cnt); // dT is in micros, convert to millis
        // apply smoothing using a buffer
        double freq = fr / 2.0; //add_and_get_freq((fr / 2.0) * 1000000.0, freq_buffer, freq_buffer_ix);
        int rpm = (int)(ctx.conf.get_rpm_adjustment() * 60.0 * rpm_ratio * freq / poles);
        //Log::trace("Ok {%d} Cnt {%d} Freq {%.3fHz} RPM {%d} D-Time {%lu} ET {%lu}\n", ok, cnt, freq, rpm, dT, (unsigned long)(current_engine_time/1000L));

        current_engine_time = engine_hours_svc->get_engine_hours();
        if (is_engine_on(rpm))
        {
            current_engine_time = current_engine_time + (uint64_t)(dT / 1000);
            if (engine_hours_svc) engine_hours_svc->save_engine_hours(current_engine_time); // milliseconds
        }
        ctx.data_cache.engine.rpm = rpm;
        ctx.data_cache.engine.engine_time = current_engine_time;
        ctx.n2k.sendEngineRPM(0, rpm);
        current_rpm = rpm;
    }
    dT = check_elapsed(micros, last_read_eng_h, PERIOD_H);
    if (dT > 0)
    {
        // send down engine time in seconds
        uint32_t ts = current_engine_time / 1000L;
        ctx.n2k.sendEngineHours(0, ts);
    }
}

void Tachometer::dumpStats()
{
    uint32_t ts = current_engine_time / 1000L;
    uint16_t ms = current_engine_time % 1000L;
    uint32_t h = ts / 3600;
    uint32_t m = (ts % 3600) / 60;
    uint32_t s = (ts % 60);
    Log::tracex(RPM_LOG_TAG, "Stats", "RPM {%d} Engine Hours {%lu:%02lu:%02lu.%03d}", current_rpm, h, m, s, ms);
}

bool Tachometer::is_enabled()
{
    return enabled;
}
