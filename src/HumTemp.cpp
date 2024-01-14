#include "Constants.h"
#include "HumTemp.h"
#include "N2K.h"
#include "Utils.h"
#include "Conf.h"
#ifdef ESP32_ARCH
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
  DHT->setup(DHTPIN, (ctx.conf.dht11_dht22 == CONF_DHT11) ? DHTesp::DHT11 : DHTesp::DHT22);
  ctx.cache.humidity = N2kDoubleNA;
  ctx.cache.temperature = N2kDoubleNA;
}

void HumTemp::read_temp_hum(ulong ms)
{
  static unsigned long last_dht_time = 0;

  if (enabled && ctx.conf.use_dht11)
  {
    if ((ms - last_dht_time) > DHT->getMinimumSamplingPeriod())
    {
      TempAndHumidity th = DHT->getTempAndHumidity();
      ctx.cache.humidity = th.humidity;
      ctx.cache.temperature = th.temperature;
      DHT->getTempAndHumidity();
    }
  }
}
#else
HumTemp::HumTemp(Context _ctx): enabled(false), ctx(_ctx)
{
}

HumTemp::~HumTemp()
{
}

void HumTemp::setup()
{
  ctx.cache.humidity = N2kDoubleNA;
  ctx.cache.temperature = N2kDoubleNA;
}


void HumTemp::read_temp_hum(ulong ms)
{
  ctx.cache.humidity = N2kDoubleNA;
  ctx.cache.temperature = N2kDoubleNA;
}
#endif

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
  }
}

void HumTemp::disable()
{
  if (enabled)
  {
    enabled = false;
  }
}
