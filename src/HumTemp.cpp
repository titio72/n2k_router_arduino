#include "HumTemp.h"
#include "N2K.h"
#include "Utils.h"
#include "Conf.h"
#include <Log.h>
#include <DHTesp.h>

HumTemp::HumTemp(const HumTemp& h): enabled(false), ctx(h.ctx), last_read_time(0)
{
  DHT = new DHTesp();
}

HumTemp::HumTemp(Context _ctx) : enabled(false), ctx(_ctx), last_read_time(0)
{
  DHT = new DHTesp();
}

HumTemp::~HumTemp()
{
  delete DHT;
}

void HumTemp::setup()
{
  Log::tracex("DHT", "Setup", "Type {%s} Pin {%d}", "DHT22", DHT_PIN);
  DHT->setup(DHT_PIN, DHTesp::DHT22);
  ctx.cache.humidity = NAN;
  ctx.cache.temperature = NAN;
}

void HumTemp::read_temp_hum(ulong millisecs)
{
  if (enabled && check_elapsed(millisecs, last_read_time, DHT->getMinimumSamplingPeriod()))
  {
    TempAndHumidity th = DHT->getTempAndHumidity();
    ctx.cache.humidity = th.humidity;
    ctx.cache.temperature = th.temperature;
    //Log::tracex("DHT", "Read", "Temp {%.1fC} Hum {%.1f%%}\n", ctx.cache.temperature, ctx.cache.humidity);
  }
}

void HumTemp::loop(unsigned long micros)
{
  read_temp_hum(micros/1000L);
}

bool HumTemp::is_enabled()
{
  return enabled;
}

void HumTemp::enable()
{
  if (!enabled)
  {
    enabled = true;
    Log::tracex("DHT", "Enable", "Success {%d}", enabled);
  }
}

void HumTemp::disable()
{
  if (enabled)
  {
    enabled = false;
    ctx.cache.humidity = NAN;
    ctx.cache.temperature = NAN;
    Log::tracex("DHT", "Disable", "Succcess {%d}", !enabled);
  }
}
