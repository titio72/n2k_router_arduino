#include "MeteoDHT.h"
#include "N2K.h"
#include "Utils.h"
#include "Data.h"
#include "Conf.h"
#include <Log.h>
#ifndef NATIVE
#include <DHTesp.h>
#endif
#define DHT_LOG_TAG "DHT"

MeteoDHT::MeteoDHT(int _pin, DHT_MODEL _model, uint8_t ix) : enabled(false), meteo_index(ix), dht(), last_read_time(0), pin(_pin), model(_model)
{
  #ifndef NATIVE
  dht = new DHTesp;
  #endif
}

MeteoDHT::~MeteoDHT()
{
  disable();
  #ifndef NATIVE
  delete dht;
  #endif
}

void MeteoDHT::setup(Context &ctx)
{
  Log::tracex(DHT_LOG_TAG, "Setup", "Type {%d} Pin {%d}", model, pin);
}

void MeteoDHT::read_temp_hum(unsigned long micros, MeteoData &data)
{
  #ifndef NATIVE
  if (check_elapsed(micros, last_read_time, dht->getMinimumSamplingPeriod() * 1000L))
  {
    if (enabled)
    {
      TempAndHumidity th = dht->getTempAndHumidity();
      data.humidity = th.humidity;
      data.temperature = th.temperature;
      // Log::tracex(DHT_LOG_TAG, "Read", "Temp {%.1fC} Hum {%.1f%%}\n", data.temperature, data.humidity);
    }
    else
    {
      data.humidity = NAN;
      data.temperature = NAN;
    }
  }
  #else
  data.humidity = NAN;
  data.temperature = NAN;
  #endif
}

void MeteoDHT::loop(unsigned long micros, Context &ctx)
{
  switch (meteo_index)
  {
  case 0:
    read_temp_hum(micros, ctx.data_cache.meteo_0);
    break;
  case 1:
    read_temp_hum(micros, ctx.data_cache.meteo_1);
    break;
  default:
    break;
  }
}

bool MeteoDHT::is_enabled()
{
  return enabled;
}

void MeteoDHT::enable()
{
  if (!enabled)
  {
    #ifndef NATIVE
    dht->setup(pin, model == DHT11 ? dht->DHT_MODEL_t::DHT11 : dht->DHT_MODEL_t::DHT22);
    enabled = dht->getStatus() == DHTesp::ERROR_NONE;
    #else
    enabled = true;
    #endif
    Log::tracex(DHT_LOG_TAG, "Enable", "Success {%d}", enabled);
  }
}

void MeteoDHT::disable()
{
  if (enabled)
  {
    enabled = false;
    Log::tracex(DHT_LOG_TAG, "Disable", "Succcess {%d}", !enabled);
  }
}
