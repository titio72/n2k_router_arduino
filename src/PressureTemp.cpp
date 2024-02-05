#include "Constants.h"
#include "PressureTemp.h"
#include "N2K.h"
#include "Utils.h"
#include "Conf.h"
#include "Log.h"

#ifdef ESP32_ARCH
#include <Arduino.h>
#include "TwoWireProvider.h"
#include <Adafruit_BMP280.h>

PressureTemp::PressureTemp(Context _ctx) : enabled(false), ctx(_ctx), last_read(0)
{
  bmp = NULL;
}

PressureTemp::~PressureTemp()
{
  disable();
  delete bmp;
}

void PressureTemp::enable()
{
  if (!enabled)
  {
    bmp = new Adafruit_BMP280(TwoWireProvider::get_two_wire());
    enabled = bmp->begin(0x76, 0x60);
    if (!enabled)
    {
      delete bmp;
    }
    Log::trace("[BMP] Enabled {%d}\n", enabled);
  }
}

void PressureTemp::disable()
{
  if (enabled)
  {
    Log::trace("[BMP] Disabled\n");
    bmp = NULL;
    enabled = false;
  }
}

void PressureTemp::read_pressure(unsigned long ms)
{
  ctx.cache.pressure = N2kDoubleNA;
  ctx.cache.temperature_el = N2kDoubleNA;
  if (enabled)
  {
    ctx.cache.pressure = bmp->readPressure();
    ctx.cache.temperature_el = bmp->readTemperature();
    //Log::trace("[BMP] Read pressure {%.1f}\n", ctx.cache.pressure/100.0);
  }
}
#else
PressureTemp::PressureTemp(Context _ctx) : enabled(false), ctx(_ctx)
{
}

PressureTemp::~PressureTemp()
{
}

void PressureTemp::enable()
{
  enabled = true;
}

void PressureTemp::disable()
{
  enabled = false;
}

void PressureTemp::read_pressure(unsigned long ms)
{
}
#endif

void PressureTemp::setup()
{
  ctx.cache.humidity = N2kDoubleNA;
  ctx.cache.temperature = N2kDoubleNA;
}

void PressureTemp::loop(unsigned long ms)
{
  if ((ms-last_read)>1000)
  {
    read_pressure(ms);
    last_read = ms;
  }
}

bool PressureTemp::is_enabled()
{
  return enabled;
}