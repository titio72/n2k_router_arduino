#include "SpeedThroughWater.h"
#include "Utils.h"
#include "Conf.h"
#include <Log.h>

/*
ST810 Speed Through Water Sensor specs says 4.8Hz = 1Kn
*/

SpeedThroughWater::SpeedThroughWater(int pin) : speed_sensor(pin), enabled(false)
{
}

SpeedThroughWater::~SpeedThroughWater()
{
}

void SpeedThroughWater::loop(unsigned long milliseconds, Context &ctx)
{
  if (!enabled)
    return;

  WaterData &data = ctx.data_cache.water_data;
  Configuration &conf = ctx.conf;
  double frequency = 0.0;
  int cnt = 0;
  speed_sensor.set_alpha(conf.get_stw_paddle_alpha());
  if (speed_sensor.read_data(milliseconds, frequency, cnt))
  {
    data.frequency = frequency;
    data.speed = frequency * conf.get_stw_paddle_adjustement() * 4.8; // 4.8Hz = 1Kn
    data.speed_error = STW_ERROR_OK;
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