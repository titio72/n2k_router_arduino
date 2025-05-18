#include "MeteoDHT.h"
#include "N2K.h"
#include "Utils.h"
#include "Data.h"
#include "Conf.h"
#include <Log.h>
#include <DHTesp.h>

#define DHT_LOG_TAG "DHT"
#ifndef DHT_MODEL
#define DHT_MODEL DHT22
#endif

MeteoDHT::MeteoDHT(Context _ctx) : enabled(false), ctx(_ctx), last_read_time(0)
{
  DHT = new DHTesp();
}

MeteoDHT::~MeteoDHT()
{
  delete DHT;
}

void MeteoDHT::setup()
{
  Log::tracex(DHT_LOG_TAG, "Setup", "Type {%d} Pin {%d}", DHT->DHT_MODEL_t::DHT_MODEL, DHT_PIN);
}

void MeteoDHT::read_temp_hum(unsigned long micros)
{
  if (enabled && check_elapsed(micros, last_read_time, DHT->getMinimumSamplingPeriod() * 1000L))
  {
    TempAndHumidity th = DHT->getTempAndHumidity();
    ctx.cache.humidity_1 = th.humidity;
    ctx.cache.temperature_1 = th.temperature;
    //Log::tracex(DHT_LOG_TAG, "Read", "Temp {%.1fC} Hum {%.1f%%}\n", ctx.cache.temperature, ctx.cache.humidity);
  }
}

void MeteoDHT::loop(unsigned long micros)
{
  read_temp_hum(micros);
}

bool MeteoDHT::is_enabled()
{
  return enabled;
}

void MeteoDHT::enable()
{
  if (!enabled)
  {
    DHT->setup(DHT_PIN, DHT->DHT_MODEL_t::DHT_MODEL);
    enabled = DHT->getStatus() == DHTesp::ERROR_NONE;
    ctx.cache.humidity_1 = NAN;
    ctx.cache.temperature_1 = NAN;
    Log::tracex(DHT_LOG_TAG, "Enable", "Success {%d}", enabled);
  }
}

void MeteoDHT::disable()
{
  if (enabled)
  {
    enabled = false;
    ctx.cache.humidity_1 = NAN;
    ctx.cache.temperature_1 = NAN;
    Log::tracex(DHT_LOG_TAG, "Disable", "Succcess {%d}", !enabled);
  }
}
