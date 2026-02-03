#include "SpeedThroughWater.h"
#include "N2K_router.h"
#include "Utils.h"
#include "Conf.h"
#include <Log.h>

/*
ST810 Speed Through Water Sensor specs says 4.8Hz = 1Kn
*/
#define PERIOD 1000000L // Period for stw calculation and n2k sending

SpeedThroughWater::SpeedThroughWater(int pin) : speed_sensor(pin), enabled(false)
{
}

SpeedThroughWater::~SpeedThroughWater()
{
}

void SpeedThroughWater::loop(unsigned long micros, Context &ctx)
{
  if (!enabled)
    return;

  if (check_elapsed(micros, last_read, PERIOD) == 0)
    return;

  WaterData &data = ctx.data_cache.water_data;
  Configuration &conf = ctx.conf;
  double frequency = 0.0;
  int cnt = 0;
  speed_sensor.set_alpha(conf.get_stw_paddle_alpha());
  if (speed_sensor.read_data(micros/1000, frequency, cnt))
  {
    data.frequency = frequency;
    data.speed = frequency * conf.get_stw_paddle_adjustement() * 4.8; // 4.8Hz = 1Kn
    data.speed_error = STW_ERROR_OK;
    ctx.n2k.sendSTW(data.speed);
    ctx.n2k.sendMagneticHeading(135.0); // for tests only
  }
  else
  {
    data.speed_error = STW_ERROR_NO_SIGNAL;
  }
}

void SpeedThroughWater::setup(Context &ctx)
{
  speed_sensor.setup();
}

void SpeedThroughWater::enable()
{
  if (!enabled)
  {
    enabled = true;
  }
}

void SpeedThroughWater::disable()
{
  if (enabled)
  {
    enabled = false;
  }
} 

bool SpeedThroughWater::is_enabled()
{
    return enabled;
}