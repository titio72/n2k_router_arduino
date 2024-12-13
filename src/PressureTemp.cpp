#include "PressureTemp.h"
#include "N2K.h"
#include "Utils.h"
#include "Conf.h"
#include "Log.h"

#include <Arduino.h>
#include "TwoWireProvider.h"
#include <Adafruit_BMP280.h>

#define BMP_LOG_TAG "BMP"

PressureTemp::PressureTemp(Context _ctx) : enabled(false), ctx(_ctx), last_read(0), bmp(NULL)
{
}

PressureTemp::~PressureTemp()
{
  if (bmp) delete bmp;
}

void PressureTemp::enable()
{
  if (!enabled)
  {
    enabled = bmp->begin(0x76, 0x60);
    Log::tracex(BMP_LOG_TAG, "Enable", "Success {%d}", enabled);
  }
}

void PressureTemp::disable()
{
  if (enabled)
  {
    ctx.cache.pressure = NAN;
    ctx.cache.temperature_el = NAN;
    enabled = false;
    Log::tracex(BMP_LOG_TAG, "Disable", "Succsess {%d}", !enabled);
  }
}

void PressureTemp::read_pressure(unsigned long ms)
{
  if (enabled)
  {
    ctx.cache.pressure = bmp->readPressure();
    ctx.cache.temperature_el = bmp->readTemperature();
  }
}

void PressureTemp::setup()
{
  Log::tracex(BMP_LOG_TAG, "Setup");
  bmp = new Adafruit_BMP280(TwoWireProvider::get_two_wire());
  ctx.cache.humidity = NAN;
  ctx.cache.temperature = NAN;
}

void PressureTemp::loop(unsigned long ms)
{
  if (enabled && check_elapsed(ms, last_read, 1000000))
  {
    read_pressure(ms);
    last_read = ms;
  }
}

bool PressureTemp::is_enabled()
{
  return enabled;
}