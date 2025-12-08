#include "MeteoBME.h"
#include "N2K.h"
#include "Utils.h"
#include "Data.h"
#include "Conf.h"
#include "Log.h"

#define BME_LOG_TAG "BME"

#ifndef NATIVE
#include <Arduino.h>
#include "TwoWireProvider.h"
#include <Adafruit_BME280.h>
class MYBME : public BME280Internal
{
public:
  MYBME(int address) : addr(address) {}

  virtual bool start()
  {
    bool ok = b.begin(addr, TwoWireProvider::get_two_wire()); // set back to 0x76!!!!!!
    if (ok)
    {
      b.setSampling(
          Adafruit_BME280::MODE_NORMAL,
          Adafruit_BME280::SAMPLING_X4,
          Adafruit_BME280::SAMPLING_X4,
          Adafruit_BME280::SAMPLING_X4,
          Adafruit_BME280::FILTER_X2,
          Adafruit_BME280::STANDBY_MS_500);
    }
    return ok;
  }

  virtual void stop()
  {
  }

  virtual float readPressure()
  {
    return b.readPressure();
  }

  virtual float readTemperature()
  {
    return b.readTemperature();
  }

  virtual float readHumidity()
  {
    return b.readHumidity();
  }

private:
  int addr;
  Adafruit_BME280 b;
};
#endif

MeteoBME::MeteoBME(int _address, uint8_t ix, BME280Internal *impl)
    : enabled(false), last_read(0), bme(impl), address(_address), meteo_index(ix), internalStateOwned(false)
{
  #ifndef NATIVE
  if (bme==nullptr)
  {
    bme = new MYBME(address);
    internalStateOwned = true;
  }
  #endif
}

MeteoBME::~MeteoBME()
{
  if (bme)
    delete bme;
}

void reset(MeteoData &data)
{
  data.pressure = NAN;
  data.temperature = NAN;
  data.humidity = NAN;
}

void MeteoBME::enable()
{
  if (!enabled)
  {
    Log::tracex(BME_LOG_TAG, "Enabling", "address {0x%x}", address);
    enabled = bme->start();
    Log::tracex(BME_LOG_TAG, "Enable", "Success {%d}", enabled);
  }
}

void MeteoBME::disable()
{
  if (enabled)
  {
    enabled = false;
    Log::tracex(BME_LOG_TAG, "Disable", "Succsess {%d}", !enabled);
  }
}

void MeteoBME::read(unsigned long ms, MeteoData &data)
{
  if (enabled && bme)
  {
    data.pressure = bme->readPressure();
    data.temperature = bme->readTemperature();
    data.humidity = bme->readHumidity();
  }
  else
  {
    reset(data);
  }
}

void MeteoBME::setup(Context &ctx)
{
  Log::tracex(BME_LOG_TAG, "Setup");
}

void MeteoBME::loop(unsigned long ms, Context &ctx)
{
  if (check_elapsed(ms, last_read, 1000000))
  {
    switch (meteo_index)
    {
    case 0:
      read(ms, ctx.data_cache.meteo_0);
      break;
    case 1:
      read(ms, ctx.data_cache.meteo_1);
      break;
    default:
      break;
    }
  }
}

bool MeteoBME::is_enabled()
{
  return enabled;
}