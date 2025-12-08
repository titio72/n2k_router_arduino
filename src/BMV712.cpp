#include "BMV712.h"
#include "Utils.h"
#include "N2K_router.h"
#include "Data.h"
#include <Log.h>

#define CAPACITY 280.0
#define INSTANCE 0
#define INSTANCE_E 1

#define VE_LOG_PREFIX "VE"

BMV712::BMV712(Port &_p) : p(_p), enabled(false), delta_time(0), bmv_vedirect(), last_read_time(0), checksum(0), read(false)
{
}

BMV712::~BMV712()
{
}

static const double N2K_NAN = -1e9;

void reset_cache(BatteryData *data)
{
  if (data)
  {
    data->voltage = NAN;
    data->current = NAN;
    data->soc = NAN;
    data->temperature = NAN;
    data->ttg = NAN;
  }
}

void BMV712::on_complete(VEDirectObject &obj)
{
  // message is complete
  double soc = NAN;
  double voltage = NAN;
  double voltage1 = NAN;
  double current = NAN;
  double temperature = NAN;
  double ttg = NAN;
  bmv_vedirect.get_number_value(voltage, 0.001, BMV_VOLTAGE);     // convert in V from mV
  bmv_vedirect.get_number_value(voltage1, 0.001, BMV_VOLTAGE_1);  // convert in V from mV
  bmv_vedirect.get_number_value(current, 0.001, BMV_CURRENT);     // convert in A from mA
  bmv_vedirect.get_number_value(soc, 0.001, BMV_SOC);             // convert in percentage from 1000ths
  bmv_vedirect.get_number_value(temperature, 1, BMV_TEMPERATURE); // celsius
  // Log::trace("[BMV] Read values: SOC {%.2f%} V0 {%.2f V} V1 {%.2f V} Current {%.2f A} Temp {%.2f}\n", soc, voltage, voltage1, current, temperature);
  data_svc.voltage = voltage;
  data_svc.current = current;
  data_svc.soc = soc;
  data_svc.temperature = temperature;
  data_svc.ttg = NAN;
  data_eng.voltage = voltage1;
  data_eng.current = NAN;
  data_eng.soc = NAN;
  data_eng.temperature = NAN;
  data_eng.ttg = NAN;
  read = true;
}

void BMV712::on_partial_x(const char *line, int len)
{
  bmv_vedirect.on_partial(line, len);
}

void BMV712::on_line_read(const char *line)
{
  bmv_vedirect.on_line_read(line);
}

void BMV712::loop(unsigned long micros, Context &ctx)
{
  //Serial.printf("Loop %ul\n", micros);
  if (last_read_time == 0 || check_elapsed(micros, last_read_time, 10000000L))
  {
    Log::tracex(VE_LOG_PREFIX, "Reset cache", "No activity detected for 10 seconds");
    // no activity for 10 seconds, reset values
    reset_cache(&data_eng);
    reset_cache(&data_svc);
    bmv_vedirect.reset();
    last_read_time = micros;
  }
  
  if (enabled)
  {
    //Serial.printf("Listen %ul\n", micros);
  
    p.listen(250); // read for a maximum of X milliseconds before returning to the main loop
    
    if (read)
    {
      last_read_time = micros;
      unsigned char sid = n2ksid.getNew();
      if (!isnan(data_svc.voltage))
        ctx.n2k.sendBattery(sid, data_svc.voltage, data_svc.current, data_svc.temperature, INSTANCE);
      if (!isnan(data_eng.voltage))
        ctx.n2k.sendBattery(sid, data_eng.voltage, N2kDoubleNA, N2kDoubleNA, INSTANCE_E);
      if (!isnan(data_svc.soc))
        ctx.n2k.sendBatteryStatus(sid, data_eng.soc * 100.0, CAPACITY, data_eng.ttg, INSTANCE);
      ctx.data_cache.battery_svc = data_svc;
      ctx.data_cache.battery_eng = data_eng;
    }
    read = false;
  }
  else
  {
    reset_cache(&ctx.data_cache.battery_svc);
    reset_cache(&ctx.data_cache.battery_eng);
  }
}

void BMV712::setup(Context &ctx)
{
  bmv_vedirect.init(BMV_FIELDS, BMV_N_FIELDS);
  bmv_vedirect.set_listener(this);
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
    bmv_vedirect.reset();
    enabled = false;
    reset_cache(&data_eng);
    reset_cache(&data_svc);
    p.close();
    Log::tracex(VE_LOG_PREFIX, "Disable", "Success {%d}", !enabled);
  }
}