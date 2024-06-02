#include "HumTemp.h"
#include "N2K.h"
#include "Utils.h"
#include "Conf.h"
#include <Log.h>
#include "DHTesp.h"

HumTemp::HumTemp(Context _ctx) : enabled(false), ctx(_ctx)
{
  DHT = new DHTesp();
}

HumTemp::~HumTemp()
{
  delete DHT;
}

void HumTemp::setup()
{
  Log::trace("[DHT] Setup DHT22 on pin {%d}\n", DHT_PIN);
  DHT->setup(DHT_PIN, DHTesp::DHT22);
  ctx.cache.humidity = N2kDoubleNA;
  ctx.cache.temperature = N2kDoubleNA;
}

void HumTemp::read_temp_hum(ulong ms)
{
  static unsigned long last_dht_time = 0;

  if (enabled)
  {
    if ((ms - last_dht_time) > DHT->getMinimumSamplingPeriod())
    {
      last_dht_time = ms;
      TempAndHumidity th = DHT->getTempAndHumidity();
      ctx.cache.humidity = th.humidity;
      ctx.cache.temperature = th.temperature;
      //Log::trace("[DHT] Read {%.1fC} {%.1f%%}\n", ctx.cache.temperature, ctx.cache.humidity);
      //DHT->getTempAndHumidity();
    }
  }
}

void HumTemp::loop(unsigned long ms)
{
  read_temp_hum(ms);
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
    Log::trace("[DHT] Enabled {%d}\n", enabled);
  }
}

void HumTemp::disable()
{
  if (enabled)
  {
    enabled = false;
    Log::trace("[DHT] Disabled\n");
  }
}
