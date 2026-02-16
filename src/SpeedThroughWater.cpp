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

void reset_data(WaterData &data, uint8_t error_code = STW_ERROR_OK)
{
    data.frequency = NAN;
    data.speed = NAN;
    data.speed_error = error_code;
}

void SpeedThroughWater::loop(unsigned long micros, Context &ctx)
{
  WaterData &data = ctx.data_cache.water_data;

  if (!enabled) {
    reset_data(data);
    return;
  }

  if (check_elapsed(micros, last_read, PERIOD) == 0)
    return;

  Configuration &conf = ctx.conf;
  double frequency = 0.0;
  int cnt = 0;
  //Serial.printf("STW: Reading speed sensor data... %.2f\n", conf.get_stw_paddle_alpha());
  speed_sensor.set_alpha(conf.get_stw_paddle_alpha());
  if (speed_sensor.read_data(micros/1000, frequency, cnt))
  {
    data.frequency = frequency;
    data.speed = frequency * conf.get_stw_paddle_adjustment() / 4.8; // 4.8Hz = 1Kn
    data.speed_error = STW_ERROR_OK;
    ctx.n2k.sendSTW(data.speed);
  }
  else
  {
    reset_data(data, STW_ERROR_NO_SIGNAL);
  }
}

void SpeedThroughWater::setup(Context &ctx)
{
  speed_sensor.setup();
}

void SpeedThroughWater::enable(Context &ctx)
{
  if (!enabled)
  {
    enabled = true;
    Log::tracex("STW", "Enable", "Success {%d}", enabled);
  }
}

void SpeedThroughWater::disable(Context &ctx)
{
  if (enabled)
  {
    enabled = false;
    Log::tracex("STW", "Disable", "Success {%d}", enabled);
    reset_data(ctx.data_cache.water_data);
  }
} 

bool SpeedThroughWater::is_enabled()
{
    return enabled;
}