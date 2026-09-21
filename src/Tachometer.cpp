#include "Tachometer.h"
#include "N2K_router.h"
#include "Utils.h"
#include "Conf.h"
#include <Log.h>

#define RPM_LOG_TAG "RPM"

// frequency calculation
#define PERIOD 1000000L // Period for RPM calculation
#define PERIOD_H 2000000L // Period for sending engine hours
#define ENGINE_STOP_FLUSH_MIN_MS 5000 // do not write on engine stop for less than this much unsaved time
#define PERIOD_ENGINE_HOURS_WRITE 60000000L // Period for writing engine hours to persistence. Every minutes, so not to hammer the EEPROM

void IRAM_ATTR Tachometer::timer_callback(void *arg)
{
    Tachometer *self = static_cast<Tachometer *>(arg);
    self->speed_sensor.loop_micros(_micros());
}

Tachometer::Tachometer(uint8_t _pin, EngineHours *eng, uint8_t _poles, double _rpm_ratio, double _rpm_adjustment)
    :enabled(false), poles(_poles), rpm_ratio(_rpm_ratio), current_rpm(0), engine_hours_svc(eng),
    last_read(0), last_read_eng_h(0), last_engine_hours_write_time(0), is_setup(false), current_engine_time(0), engine_was_on(false), last_persisted_engine_time(0), speed_sensor(_pin), timer_handle(NULL)
{
}

Tachometer::~Tachometer()
{
    enabled = false;
    if (timer_handle)
    {
        #ifndef NATIVE
        esp_timer_stop(timer_handle);
        esp_timer_delete(timer_handle);
        timer_handle = NULL;
        #endif
    }
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
        #ifndef NATIVE
        esp_timer_create_args_t timer_args = {};
        timer_args.callback = &Tachometer::timer_callback;
        timer_args.arg = this;
        timer_args.dispatch_method = ESP_TIMER_TASK;
        timer_args.name = "tacho_loop";
        esp_timer_create(&timer_args, &timer_handle);
        esp_timer_start_periodic(timer_handle, 1000); // 1000 µs = 1 ms
        #endif
        Log::tracex(RPM_LOG_TAG, "Enable", "Success {%d}", enabled);
    }
}

void Tachometer::disable(Context &ctx)
{
    if (enabled)
    {
        flush(); // the up-to-a-minute accumulated since the last periodic save
        Log::tracex(RPM_LOG_TAG, "Disable", "Success {1}");
        enabled = false;
        engine_was_on = false;
        if (timer_handle)
        {
            #ifndef NATIVE
            esp_timer_stop(timer_handle);
            esp_timer_delete(timer_handle);
            #endif
            timer_handle = NULL;
        }
    }
}

bool Tachometer::flush(uint64_t min_unsaved_ms)
{
    if (!engine_hours_svc)
        return false;
    // someone else (BLE 'H') changed the stored value and the loop has not adopted it yet: theirs wins
    if (engine_hours_svc->get_engine_hours() != last_persisted_engine_time)
        return false;
    if (current_engine_time <= last_persisted_engine_time)
        return false; // nothing new
    if (current_engine_time - last_persisted_engine_time < min_unsaved_ms)
        return false;
    if (!engine_hours_svc->save_engine_hours(current_engine_time))
        return false;
    last_persisted_engine_time = current_engine_time;
    return true;
}

void Tachometer::setup(Context &ctx)
{
    if (!is_setup)
    {
        Log::tracex(RPM_LOG_TAG, "Setup", "Pin {%d}", speed_sensor.get_pin());
        speed_sensor.setup();
        is_setup = true;
        current_engine_time = engine_hours_svc->get_engine_hours();
        last_persisted_engine_time = current_engine_time;
        ctx.data_cache.engine.engine_time = current_engine_time;
        Log::tracex(RPM_LOG_TAG, "Setup", "Engine Hours loaded {%lu.%03d}", (uint32_t)(current_engine_time / 1000), (uint16_t)(current_engine_time % 1000));
    }
}

static bool is_engine_on(int rpm)
{
    return rpm > 200;
}

void Tachometer::read_signal()
{
#ifdef NATIVE
    _test_signal_state = (_test_signal_state == LOW) ? HIGH : LOW;
    _test_signal_time += 5000; // 5ms per transition, above 2ms debounce threshold
    speed_sensor.read_signal(_test_signal_state, _test_signal_time);
#endif
}

//#define CRUDE_TEST_RPM

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
        // get frequency in Hz
        
        #ifdef CRUDE_TEST_RPM
        double freq = 125; //Hz for crude test RPM
        #else
        bool ok = speed_sensor.read_data(ms_clock.update(micros), fr, cnt); // SpeedSensor wants a continuous millisecond clock
        double freq = ok ? fr : 0.0; // smoothed frequency in Hz from SpeedSensor (alpha set via BLE 'a' command)
        #endif
        int rpm = (int)(ctx.conf.get_rpm_adjustment() * 60.0 * rpm_ratio * freq / poles);


        //Log::tracex(RPM_LOG_TAG, "Loop", "Ok {%d} Cnt {%d} Freq {%.3fHz} RPM {%d} D-Time {%lu} ET {%lu.%03d} Max {%d} Min {%d}", ok, cnt, freq, rpm, dT, (uint32_t)(current_engine_time / 1000), (uint16_t)(current_engine_time % 1000), speed_sensor.max, speed_sensor.min);
        // engine_time is running state: the service only holds the last *persisted* value (written once a
        // minute), so re-reading it every tick would discard the time accumulated since. Adopt the service
        // value only when someone else changed it (e.g. the BLE 'H' command).
        if (engine_hours_svc && engine_hours_svc->get_engine_hours() != last_persisted_engine_time)
        {
            current_engine_time = engine_hours_svc->get_engine_hours();
            last_persisted_engine_time = current_engine_time;
        }
        bool engine_on = is_engine_on(rpm);
        if (engine_was_on && !engine_on)
        {
            // Engine just stopped: persist now. The boat's power is often cut right after, and the periodic save
            // alone would lose up to a minute. The threshold keeps a speed hovering around the on/off limit
            // from turning into a flash write per second.
            flush(ENGINE_STOP_FLUSH_MIN_MS);
        }
        engine_was_on = engine_on;
        if (engine_on)
        {
            current_engine_time += (uint64_t)(((double)dT / 1000.0)+0.5); // engine time in milliseconds, add 0.5 to round to the nearest millisecond
            if (check_elapsed(micros, last_engine_hours_write_time, PERIOD_ENGINE_HOURS_WRITE) > 0)
            {
                if (engine_hours_svc)
                {
                    engine_hours_svc->save_engine_hours(current_engine_time); // milliseconds
                    last_persisted_engine_time = current_engine_time;
                }
            }
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
