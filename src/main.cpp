#ifndef NATIVE
#include <Arduino.h>

#include <Ports.h>
#include <ArduinoPort.hpp>
#include <Utils.h>
#include <Log.h>
#include <N2K.h>

#include "N2K_router.h"
#include "Context.h"
#include "Conf.h"
#include "Constants.h"
#include "Dummy.h"

#if GPS_TYPE == 1
#include "GPSX.h"
#elif GPS_TYPE == 2
#include "GPSX.h"
#endif

#if DO_TACHOMETER == 1
#include "Tachometer.h"
#endif

#include "MeteoBME.h"
#include "MeteoDHT.h"
#include "Display.h"
#include "Leds.h"

#if DO_VE_DIRECT == 1
#include "BMV712.h"
#endif

#include "Temperature.h"
#include "SpeedThroughWater.h"
#include "EnvMessenger.h"
#include "BLEConf.h"
#include "CommandHandler.hpp"
#include "Agents.hpp"

void on_source_claim(const unsigned char old_source, const unsigned char new_source);
void on_command(char command, const char *command_value);

#pragma region CONTEXT
Configuration conf;
EngineHours engineHours;
Data cache;
N2K_router n2k(on_source_claim);
Context context(n2k, conf, cache);
#pragma endregion

#pragma region AGENTS

// GPS_TYPE
//      0 -> Dummy
//			1 -> I2C (u-blox lib is required)
//			2 -> UART (u-blox lib is required)
#if GPS_TYPE == 1
GPSX gps;
#elif GPS_TYPE == 2
HardwareSerial gpsSerial(Serial1);
GPSX gps(&gpsSerial, GPS_RX_PIN, GPS_TX_PIN);
#else
Dummy gps;
#endif

#if (DO_VE_DIRECT == 1)
ArduinoPort<HardwareSerial> veDirectPort("VE", Serial1, VE_DIRECT_RX_PIN, VE_DIRECT_TX_PIN, false);
BMV712 bmv712(veDirectPort);
#else
Dummy bmv712;
#endif

EVODisplay display;
Tachometer tacho(ENGINE_RPM_PIN, &engineHours, TACHO_POLES, TACHO_RPM_RATIO, TACHO_RPM_ADJUSTMENT);
MeteoDHT dht(DHT_PIN, MeteoDHT::DHT_MODEL::DHT_TYPE, 1);
MeteoBME bme(BME_ADDRESS, 0);
WaterTemperature waterTemp(WATER_TEMP_PIN);
SpeedThroughWater speedThroughWater(STW_PADDLE_PIN);
Leds leds;
BLEConf bleConf(on_command);
EnvMessenger envMessanger;
#pragma endregion

bool started = false;
struct AppStats
{
  unsigned long cycles = 0;
  unsigned short retry_gps = 0;
  unsigned short retry_dht = 0;
  unsigned short retry_bme = 0;
  unsigned short retry_bmv712 = 0;
  unsigned short retry_tacho = 0;
  unsigned short retry_display = 0;
  unsigned short retry_leds = 0;
  unsigned short retry_env_messager = 0;
} app_stats;

void on_source_claim(const unsigned char old_source, const unsigned char new_source)
{
  if (!conf.get_services().is_keep_n2k_src())
  {
    conf.save_n2k_source(new_source);
  }
  Log::tracex(APP_LOG_TAG, "New claimed n2k source", " New Source {%d} Old Source {%d} Save {%d}", 
    new_source, old_source, conf.get_services().is_keep_n2k_src() ? 1 : 0);
}

void on_message_sent(const tN2kMsg &N2kMsg, bool success)
{
  leds.blink(LED_N2K, micros(), N2K_BLINK_USEC, !success);
}

void handle_display(unsigned long ms)
{
  static unsigned long t0 = 0;
  if (check_elapsed(ms, t0, 1000000))
  {
    // display.draw_text("GPS %d\nSATS %d/%d", cache.gsa.fix, cache.gsa.nSat, cache.gsv.nSat);
    display.draw_text("%.1fmB\n%d%% %.1fC", cache.get_pressure(conf) / 100.0f, (int)cache.get_humidity(conf), cache.get_temperature(conf));
  }
}

void handle_leds(unsigned long ms)
{
  static unsigned long t0 = 0;
  if (check_elapsed(ms, t0, 1000000))
  {
    leds.switchLed(LED_GPS, cache.gps.fix > 1);
  }
}

void dump_process_stats()
{
  Log::tracex(APP_LOG_TAG, "Stats", "Cycles {%d} in 10s", app_stats.cycles);
}

void report_stats(unsigned long ms)
{
  static unsigned long last_time_stats_ms = 0;
  if (check_elapsed(ms, last_time_stats_ms, 10000000))
  {
    gps.dumpStats();
    tacho.dumpStats();
    n2k.getStats().dump();
    dump_process_stats();

    app_stats.cycles = 0;
  }
}

void _loop()
{
  app_stats.cycles++;
  unsigned long t = micros();
  if (started)
  {
    n2k.loop(t, context);
    handle_agent_loop(leds, context, true, &app_stats.retry_leds, t, "Leds");
    handle_agent_loop(display, context, true, &app_stats.retry_display, t, "Display");
    handle_agent_loop(gps, context, conf.get_services().is_use_gps(), &app_stats.retry_gps, t, "GPS");
    handle_agent_loop(bme, context, conf.get_services().is_use_bme(), &app_stats.retry_bme, t, "BMP");
    handle_agent_loop(dht, context, conf.get_services().is_use_dht(), &app_stats.retry_dht, t, "DHT");
    handle_agent_loop(bmv712, context, conf.get_services().is_use_vedirect(), &app_stats.retry_bmv712, t, "BMV712");
    handle_agent_loop(tacho, context, conf.get_services().is_use_tacho(), &app_stats.retry_tacho, t, "TACHO");
    handle_agent_loop(speedThroughWater, context, conf.get_services().is_stw_paddle(), NULL, t, "STW");
    handle_agent_loop(waterTemp, context, true, NULL, t, "WTRTEMP");
    handle_agent_loop(envMessanger, context, true, &app_stats.retry_env_messager, t, "ENV");
    handle_agent_loop(bleConf, context, true, NULL, t, "BLE");
    handle_display(t);
    handle_leds(t);
    report_stats(t);
  }
}

void _setup()
{  
  Serial.begin(115200);
  bool res_cpu_freq = setCpuFrequencyMhz(80);
  uint32_t f1 = getCpuFrequencyMhz();
  Log::enable();

  msleep(500);

  unsigned long ver = __cplusplus;
  Log::tracex(APP_LOG_TAG, "CPU", "Freq {%d} C++ {%l}", f1, ver);
  conf.init();
  engineHours.init();
  Log::tracex(APP_LOG_TAG, "Engine Hours", "Loaded engine time {%lu.%03d}", 
    (uint32_t)(engineHours.get_engine_hours() / 1000L), (uint16_t)(engineHours.get_engine_hours() % 1000L));
  
  N2K::set_sent_message_callback(on_message_sent);  
  n2k.set_desired_source(conf.get_n2k_source());
  n2k.setup(context);
  msleep(500);
  display.setup(context);
  leds.setup(context);
  gps.setup(context);
  dht.setup(context);
  bme.setup(context);
  bleConf.setup(context);
  bmv712.setup(context);
  tacho.setup(context);
  speedThroughWater.setup(context);
  waterTemp.setup();
  envMessanger.setup(context);
  msleep(500);
  started = true;

  leds.on(LED_PWR);
}

void on_command(char command, const char *command_value)
{
  CommandHandler::on_command(command, command_value, conf, engineHours, cache);
}

#ifndef PIO_UNIT_TESTING
void setup()
{
  _setup();
}

void loop()
{
  _loop();
}
#endif

#else
#ifndef PIO_UNIT_TESTING
int main(int argc, const char** argv)
{}
#endif
#endif