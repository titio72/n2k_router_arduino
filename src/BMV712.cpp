#include "BMV712.h"
#include "Utils.h"
#include "N2K_router.h"
#include "Data.h"
#include <Log.h>

#define CAPACITY 280.0
#define INSTANCE 0
#define INSTANCE_E 1
#define VEDIRECT_BAUD_RATE 19200

#define VE_LOG_PREFIX "VE"

BMV712::BMV712(Context& _c, Port& _p):
    ctx(_c), p(_p), enabled(false), delta_time(0), bmv(), last_read_time(0), checksum(0)
{
}

BMV712::~BMV712()
{
}

#define N2K_NAN -1e9

void reset_cache(Data& cache)
{
  cache.voltage = NAN;
  cache.current = NAN;
  cache.soc = NAN;
}

void BMV712::on_complete(VEDirectObject &obj)
{
    last_read_time = micros();
    static N2KSid n2ksid;
    unsigned char sid = n2ksid.getNew();
    // message is complete
    double soc = NAN;
    double voltage = NAN;
    double voltage1 = NAN;
    double current = NAN;
    double temperature = NAN;
    double ttg = NAN;
    bmv.get_number_value(voltage, 0.001, BMV_VOLTAGE); // convert in V from mV
    bmv.get_number_value(voltage1, 0.001, BMV_VOLTAGE_1); // convert in V from mV
    bmv.get_number_value(current, 0.001, BMV_CURRENT); // convert in A from mA
    bmv.get_number_value(soc, 0.1, BMV_SOC); // convert in percentage from 1000ths
    bmv.get_number_value(temperature, 1, BMV_TEMPERATURE); // celsius
    //Log::trace("[BMV] Read values: SOC {%.2f%} V0 {%.2f V} V1 {%.2f V} Current {%.2f A} Temp {%.2f}\n", soc, voltage, voltage1, current, temperature);
    if (!isnan(voltage)) ctx.n2k.sendBattery(sid, voltage, current, temperature, INSTANCE);
    if (!isnan(voltage1)) ctx.n2k.sendBattery(sid, voltage1, N2kDoubleNA, N2kDoubleNA, INSTANCE_E);
    if (!isnan(soc)) ctx.n2k.sendBatteryStatus(sid, soc, CAPACITY, ttg, INSTANCE);
    ctx.cache.voltage = voltage;
    ctx.cache.current = current;
    ctx.cache.soc = soc;
}

void BMV712::on_partial_x(const char* line, int len)
{
  bmv.on_partial(line, len);
}

double BMV712::getVoltage()
{
  double v = 0.0;
  bmv.get_number_value(v, 0.001, BMV_VOLTAGE); // convert in V from mV
  return v;
}

void BMV712::on_line_read(const char *line)
{
  bmv.on_line_read(line);
}

void BMV712::loop(unsigned long micros)
{
    if (last_read_time==0 || check_elapsed(micros, last_read_time, 10000000))
    {
      // Log::tracexx(VE_LOG_PREFIX, "Reset cache", "No activity detected for 10 seconds");
      // no activity for 10 seconds, reset values
      reset_cache(ctx.cache);
      bmv.reset();
    }

    if (enabled)
    {
        p.listen(250);
    }
}

void BMV712::setup()
{
  bmv.init(BMV_FIELDS, BMV_N_FIELDS);
  bmv.set_listener(this);
  p.set_handler(this);
  Log::tracex(VE_LOG_PREFIX, "Setup", "Setting port speed {%d}", VEDIRECT_BAUD_RATE);
  p.set_speed(VEDIRECT_BAUD_RATE);
}

bool BMV712::is_enabled()
{
    return enabled;
}

void BMV712::enable()
{
  if (!enabled)
  {
    enabled = true;
    Log::tracex(VE_LOG_PREFIX, "Enable", "Success {%d}", enabled);
  }
}

void BMV712::disable()
{
  if (enabled)
  {
    reset_cache(ctx.cache);
    bmv.reset();
    enabled = false;
    p.close();
    Log::tracex(VE_LOG_PREFIX, "Disable", "Success {%d}", !enabled);
  }
}