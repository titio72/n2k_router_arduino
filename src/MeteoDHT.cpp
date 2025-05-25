#include "MeteoDHT.h"
#include "N2K.h"
#include "Utils.h"
#include "Data.h"
#include "Conf.h"
#include <Log.h>

#define DHT_LOG_TAG "DHT"

MeteoDHT::MeteoDHT(Context _ctx, int _pin, DHT_MODEL _model, MeteoData& _data) : enabled(false), ctx(_ctx), data(_data), dht(), last_read_time(0), pin(_pin), model(_model)
{}

MeteoDHT::~MeteoDHT()
{
  disable();
}

void MeteoDHT::setup()
{
  Log::tracex(DHT_LOG_TAG, "Setup", "Type {%d} Pin {%d}", model==DHT11?dht.DHT_MODEL_t::DHT11:dht.DHT_MODEL_t::DHT22, pin);
}

void MeteoDHT::read_temp_hum(unsigned long micros)
{
  if (enabled && check_elapsed(micros, last_read_time, dht.getMinimumSamplingPeriod() * 1000L))
  {
    TempAndHumidity th = dht.getTempAndHumidity();
    data.humidity = th.humidity;
    data.temperature = th.temperature;
    //Log::tracex(DHT_LOG_TAG, "Read", "Temp {%.1fC} Hum {%.1f%%}\n", data.temperature, data.humidity);
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
    dht.setup(pin, model==DHT11?dht.DHT_MODEL_t::DHT11:dht.DHT_MODEL_t::DHT22);
    enabled = dht.getStatus() == DHTesp::ERROR_NONE;
    data.humidity = NAN;
    data.temperature = NAN;
    Log::tracex(DHT_LOG_TAG, "Enable", "Success {%d}", enabled);
  }
}

void MeteoDHT::disable()
{
  if (enabled)
  {
    enabled = false;
    data.humidity = NAN;
    data.temperature = NAN;
    Log::tracex(DHT_LOG_TAG, "Disable", "Succcess {%d}", !enabled);
  }
}
