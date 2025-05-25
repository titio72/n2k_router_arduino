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

MeteoBME::MeteoBME(Context _ctx, int _address, MeteoData& _data) : enabled(false), ctx(_ctx), data(_data), last_read(0), bme(NULL), address(_address)
{}

MeteoBME::~MeteoBME()
{
  if (bme) delete bme;
}

void MeteoBME::enable()
{
  if (!enabled)
  {
    Log::tracex(BME_LOG_TAG, "Enabling", "address {0x%x}", address);
    enabled = bme->begin(address, TwoWireProvider::get_two_wire()); // set back to 0x76!!!!!!
    if (enabled)
    {
      bme->setSampling(
        Adafruit_BME280::MODE_NORMAL,
        Adafruit_BME280::SAMPLING_X4,
        Adafruit_BME280::SAMPLING_X4,
        Adafruit_BME280::SAMPLING_X4,
        Adafruit_BME280::FILTER_X2,
        Adafruit_BME280::STANDBY_MS_500);

      data.pressure = NAN;
      data.temperature = NAN;
      data.humidity = NAN;
    }
    Log::tracex(BME_LOG_TAG, "Enable", "Success {%d}", enabled);
  }
}

void MeteoBME::disable()
{
  if (enabled)
  {
    data.pressure = NAN;
    data.temperature = NAN;
    data.humidity = NAN;
    enabled = false;
    Log::tracex(BME_LOG_TAG, "Disable", "Succsess {%d}", !enabled);
  }
}

void MeteoBME::read(unsigned long ms)
{
  if (enabled)
  {
    data.pressure = bme->readPressure();
    data.temperature = bme->readTemperature();
    data.humidity = bme->readHumidity();
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
  }
}

bool MeteoBME::is_enabled()
{
  return enabled;
}