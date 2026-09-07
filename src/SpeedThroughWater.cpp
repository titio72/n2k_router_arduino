#include "SpeedThroughWater.h"
#include "N2K_router.h"
#include "Utils.h"
#include "Conf.h"
#include <Log.h>

/*
ST810 Speed Through Water Sensor specs says 4.8Hz = 1Kn
*/
#define PERIOD 1000000L // Period for stw calculation and n2k sending

void IRAM_ATTR SpeedThroughWater::timer_callback(void *arg)
{
    SpeedThroughWater *self = static_cast<SpeedThroughWater *>(arg);
    self->speed_sensor.loop_micros(_micros());
}

SpeedThroughWater::SpeedThroughWater(int pin) : speed_sensor(pin), enabled(false)
{
}

SpeedThroughWater::~SpeedThroughWater()
{
    if (timer_handle)
    {
        #ifndef NATIVE
        esp_timer_stop(timer_handle);
        esp_timer_delete(timer_handle);
        #endif
        timer_handle = NULL;
    }
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
  speed_sensor.set_alpha(conf.get_stw_paddle_alpha());
  if (speed_sensor.read_data(micros/1000, frequency, cnt))
  {
    Log::tracex("STW", "Reading speed sensor data", "alpha {%.2f} freq {%.2f} cnt {%d}", conf.get_stw_paddle_alpha(), frequency, cnt);
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
    #ifndef NATIVE
    esp_timer_create_args_t timer_args = {};
    timer_args.callback = &SpeedThroughWater::timer_callback;
    timer_args.arg = this;
    timer_args.dispatch_method = ESP_TIMER_TASK;
    timer_args.name = "stw_loop";
    esp_timer_create(&timer_args, &timer_handle);
    esp_timer_start_periodic(timer_handle, 1000); // 1000 µs = 1 ms
    #endif
    Log::tracex("STW", "Enable", "Success {%d}", enabled);
  }
}

void SpeedThroughWater::disable(Context &ctx)
{
  if (enabled)
  {
    enabled = false;
    if (timer_handle)
    {
      #ifndef NATIVE
      esp_timer_stop(timer_handle);
      esp_timer_delete(timer_handle);
      #endif
      timer_handle = NULL;
    }
    Log::tracex("STW", "Disable", "Success {%d}", enabled);
    reset_data(ctx.data_cache.water_data);
  }
}

bool SpeedThroughWater::is_enabled()
{
    return enabled;
}