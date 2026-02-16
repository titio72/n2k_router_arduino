#include "MeteoDHT.h"
#include "N2K.h"
#include "Utils.h"
#include "Data.h"
#include "Conf.h"
#include <Log.h>

#ifndef NATIVE
#include <DHTesp.h>
class DHTInternalImpl : public DHTInternal
{
public:
  DHTInternalImpl(int _pin, MeteoDHT::DHT_MODEL _model, uint8_t _ix): pin(_pin), model(_model), ix(_ix) {}
  ~DHTInternalImpl() {}

  bool setup() {
    dht_esp.setup(pin, model == MeteoDHT::DHT_MODEL::DHT11 ? dht_esp.DHT_MODEL_t::DHT11 : dht_esp.DHT_MODEL_t::DHT22);
    return dht_esp.getStatus() == DHTesp::ERROR_NONE;
  }

  void getTempAndHumidity(double &temp, double &humidity) {
    TempAndHumidity th = dht_esp.getTempAndHumidity();
    temp = th.temperature;
    humidity = th.humidity;
  }

  unsigned long getMinimumSamplingPeriod() {
    return dht_esp.getMinimumSamplingPeriod();
  }

private:
  int pin;
  MeteoDHT::DHT_MODEL model;
  uint8_t ix;
  DHTesp dht_esp;
};
#endif
#define DHT_LOG_TAG "DHT"

MeteoDHT::MeteoDHT(int _pin, DHT_MODEL _model, uint8_t ix, DHTInternal *impl) : enabled(false), meteo_index(ix), dht(), last_read_time(0), pin(_pin), model(_model)
{
  if (impl != nullptr)
  {
    own = false;
    dht = impl;
  }
  else
  {
    #ifndef NATIVE
    own = true;
    dht = new DHTInternalImpl(pin, model, meteo_index);
    #else
    own = false;
    dht = nullptr;
    #endif
  }
}

MeteoDHT::~MeteoDHT()
{
  if (own) delete dht;
}

void MeteoDHT::setup(Context &ctx)
{
  Log::tracex(DHT_LOG_TAG, "Setup", "Type {%d} Pin {%d}", model, pin);
}

void MeteoDHT::read_temp_hum(unsigned long micros, MeteoData &data)
{
  if (dht && check_elapsed(micros, last_read_time, dht->getMinimumSamplingPeriod() * 1000L))
  {
    if (enabled)
    {
      dht->getTempAndHumidity(data.temperature, data.humidity);
      // Log::tracex(DHT_LOG_TAG, "Read", "Temp {%.1fC} Hum {%.1f%%}\n", data.temperature, data.humidity);
    }
    else
    {
      data.humidity = NAN;
      data.temperature = NAN;
    }
  }
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

void MeteoDHT::enable(Context &ctx)
{
  if (!enabled)
  {
    if (dht) enabled = dht->setup();
    else enabled = true;
    Log::tracex(DHT_LOG_TAG, "Enable", "Success {%d}", enabled);
  }
}

void MeteoDHT::disable(Context &ctx)
{
  if (enabled)
  {
    enabled = false;
    Log::tracex(DHT_LOG_TAG, "Disable", "Succcess {%d}", !enabled);
  }
}
