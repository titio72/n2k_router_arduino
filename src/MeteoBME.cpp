#include "MeteoBME.h"
#include "N2K.h"
#include "Utils.h"
#include "Data.h"
#include "Conf.h"
#include "Log.h"

#include <Arduino.h>
#include "TwoWireProvider.h"
#include <Adafruit_BME280.h>

#define BME_LOG_TAG "BME"

#ifndef BMP_ADDRESS
#define BMP_ADDRESS 0x76
#endif

MeteoBME::MeteoBME(Context _ctx) : enabled(false), ctx(_ctx), last_read(0), bme(NULL)
{
}

MeteoBME::~MeteoBME()
{
  if (bme) delete bme;
}

void MeteoBME::enable()
{
  if (!enabled)
  {
    Log::tracex(BME_LOG_TAG, "Enabling", "address {0x%x}", BMP_ADDRESS);
    enabled = bme->begin(BMP_ADDRESS, TwoWireProvider::get_two_wire()); // set back to 0x76!!!!!!
    if (enabled)
    {
      bme->setSampling(
        Adafruit_BME280::MODE_NORMAL,
        Adafruit_BME280::SAMPLING_X16,
        Adafruit_BME280::SAMPLING_X16,
        Adafruit_BME280::SAMPLING_X16,
        Adafruit_BME280::FILTER_X16,
        Adafruit_BME280::STANDBY_MS_125);

      ctx.cache.pressure_0 = NAN;
      ctx.cache.temperature_0 = NAN;
      ctx.cache.humidity_0 = NAN;
    }
    Log::tracex(BME_LOG_TAG, "Enable", "Success {%d}", enabled);
  }
}

void MeteoBME::disable()
{
  if (enabled)
  {
    ctx.cache.pressure_0 = NAN;
    ctx.cache.temperature_0 = NAN;
    ctx.cache.humidity_0 = NAN;
    enabled = false;
    Log::tracex(BME_LOG_TAG, "Disable", "Succsess {%d}", !enabled);
  }
}

void MeteoBME::read(unsigned long ms)
{
  if (enabled)
  {
    ctx.cache.pressure_0 = bme->readPressure();
    ctx.cache.temperature_0 = bme->readTemperature();
    ctx.cache.humidity_0 = bme->readHumidity();
  }
}

void MeteoBME::setup()
{
  Log::tracex(BME_LOG_TAG, "Setup");
  bme = new Adafruit_BME280();
}

void MeteoBME::loop(unsigned long ms)
{
  if (enabled && check_elapsed(ms, last_read, 1000000))
  {
    read(ms);
    //Serial.printf("Humidity: %.2f%%\n", bme->readHumidity());
  }
}

bool MeteoBME::is_enabled()
{
  return enabled;
}