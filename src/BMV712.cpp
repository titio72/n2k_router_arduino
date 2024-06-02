#include "BMV712.h"
#include "Utils.h"
#include "Log.h"
#include "N2K_router.h"

#define CAPACITY 280.0
#define INSTANCE 0
#define INSTANCE_E 1
#define VEDIRECT_BAUD_RATE 19200

BMV712::BMV712(Context& _c, Port& _p):
    ctx(_c), p(_p), enabled(false), delta_time(0), bmv(BMV_FIELDS, BMV_N_FIELDS)
{
}

BMV712::~BMV712()
{
}

void BMV712::on_partial(const char* line)
{
    static size_t l_checkusm = strlen("Checksum x");
    if (strlen(line)==l_checkusm && startswith("Checksum", line))
    {
      static N2KSid n2ksid;
      unsigned char sid = n2ksid.getNew();
      // message is complete
      double soc = 0.0;
      double voltage = 0.0;
      double voltage1 = 0.0;
      double current = 0.0;
      double temperature = 0.0;
      double ttg = 0.0;
      bmv.get_number_value(voltage, 0.001, BMV_VOLTAGE); // convert in V from mV
      bmv.get_number_value(voltage1, 0.001, BMV_VOLTAGE_1); // convert in V from mV
      bmv.get_number_value(current, 0.001, BMV_CURRENT); // convert in A from mA
      bmv.get_number_value(soc, 0.1, BMV_SOC); // convert in percentage from 1000ths
      bmv.get_number_value(temperature, 1, BMV_TEMPERATURE); // celsius
      //Log::trace("[BMV] Read values: SOC {%.2f%} V0 {%.2f V} V1 {%.2f V} Current {%.2f A}\n", soc, voltage, voltage1, current);
      ctx.n2k.sendBattery(sid, voltage, current, temperature, INSTANCE);
      ctx.n2k.sendBattery(sid, voltage1, N2kDoubleNA, N2kDoubleNA, INSTANCE_E);
      ctx.n2k.sendBatteryStatus(sid, soc, CAPACITY, ttg, INSTANCE);
      ctx.cache.voltage = voltage;
    }
}

double BMV712::getVoltage()
{
  double v = 0.0;
  bmv.get_number_value(v, 0.001, BMV_VOLTAGE); // convert in V from mV
  return v;
}


void BMV712::on_line_read(const char *line)
{
  if (startswith("PID", line))
  {
    bmv.reset();
    bmv.load_VEDirect_key_value(line, _millis());
  }
  else
  {
    bmv.load_VEDirect_key_value(line, _millis());
  }
}

void BMV712::loop(unsigned long ms)
{
    if (enabled)
    {
        p.listen(250);
    }
}

void BMV712::setup()
{
  p.set_handler(this);
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
  }
}

void BMV712::disable()
{
  if (enabled)
  {
    enabled = false;
    p.close();
  }
}