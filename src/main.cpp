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
#include "GPSX.h"

#if DO_TACHOMETER == 1
#include "Tachometer.h"
#endif

#include "MeteoBME.h"
#include "MeteoDHT.h"
#include "Display.h"
#include "Leds.h"
#include "ResetButton.h"

#if DO_VE_DIRECT == 1
#include "BMV712.h"
#endif

#include "Temperature.h"
#include "SpeedThroughWater.h"
#include "EnvMessenger.h"
#include "BLEConf.h"
#include "CommandHandler.hpp"
#include "DeferredEvents.h"
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
GPSX gps(&Serial1, GPS_RX_PIN, GPS_TX_PIN);
#else
Dummy gps;
#endif

#if DO_VE_DIRECT == 1
#if SOC_UART_NUM > 2
ArduinoPort<HardwareSerial> veDirectPort("VE", Serial2, VE_DIRECT_RX_PIN, VE_DIRECT_TX_PIN, false);
#else
ArduinoPort<HardwareSerial> veDirectPort("VE", Serial0, VE_DIRECT_RX_PIN, VE_DIRECT_TX_PIN, false);
#endif
BMV712 bmv712(veDirectPort);
#else
Dummy bmv712;
#endif

EVODisplay display;
#if DO_TACHOMETER == 1
Tachometer tacho(ENGINE_RPM_PIN, &engineHours, TACHO_POLES, TACHO_RPM_RATIO, TACHO_RPM_ADJUSTMENT);
#else
Dummy tacho;
#endif
MeteoDHT dht(DHT_PIN, MeteoDHT::DHT_MODEL::DHT_TYPE, 1);
MeteoBME bme(BME_ADDRESS, 0);
WaterTemperature waterTemp(WATER_TEMP_PIN);
SpeedThroughWater speedThroughWater(STW_PADDLE_PIN);
Leds leds;
#if RESET_PIN != -1
ResetButton resetButton(RESET_PIN);
#else
Dummy resetButton;
#endif
BLEConf bleConf(on_command);
EnvMessenger environmentMessenger;
#pragma endregion

bool started = false;

struct AppStats
{
  unsigned long cycles = 0;

  AgentSlot gps;
  AgentSlot dht;
  AgentSlot bme;
  AgentSlot bmv712;
  AgentSlot tacho;
  AgentSlot display;
  AgentSlot leds;
  AgentSlot environment_messenger;
  AgentSlot water_temp;
  AgentSlot stw_paddle;

  unsigned long n2k_loop_time = 0;
  unsigned long bleConf_loop_time = 0; // BLE and the reset button have no enable retry

  void reset_loop_time()
  {
    AgentSlot *slots[] = {&gps, &dht, &bme, &bmv712, &tacho, &display, &leds, &environment_messenger, &water_temp, &stw_paddle};
    for (AgentSlot *slot : slots)
    {
      slot->loop_time = 0;
    }
    n2k_loop_time = 0;
    bleConf_loop_time = 0;
  }
} app_stats;

// Callbacks that run on other tasks (N2K task, NimBLE host task) only post to `deferred`;
// _loop() applies them on the main task. See DeferredEvents.h for the threading rule.
DeferredEvents deferred;

void on_source_claim(const unsigned char old_source, const unsigned char new_source)
{
  deferred.post_source_claim(old_source, new_source);
}

void on_message_sent(const tN2kMsg &N2kMsg, bool success)
{
  deferred.post_n2k_activity(success);
}

void handle_display(unsigned long ms)
{
  static unsigned long t0 = 0;
  if (check_elapsed(ms, t0, 1000000))
  {
#if DO_DISPLAY == 1
    // display.draw_text("GPS %d\nSATS %d/%d", cache.gsa.fix, cache.gsa.nSat, cache.gsv.nSat);
    double p = cache.get_pressure(conf) / 100.0; // Pa -> hPa (mbar)
    double h = cache.get_humidity(conf);
    double t = cache.get_temperature(conf);
    char ps[12] = "--", hs[8] = "--", ts[12] = "--"; // "--" when the value is not available
    if (!isnan(p)) snprintf(ps, sizeof(ps), "%.1f", p);
    if (!isnan(h)) snprintf(hs, sizeof(hs), "%d", (int)h);
    if (!isnan(t)) snprintf(ts, sizeof(ts), "%.1f", t);
    display.draw_text("%smB\n%s%% %sC", ps, hs, ts);
#endif
  }
}

void handle_leds(unsigned long ms)
{
  static unsigned long t0 = 0;
  if (check_elapsed(ms, t0, 1000000))
  {
    leds.switchLed(LED_GPS, cache.gps.fix > 1);
    N2KStats n2k_stats = n2k.getStats();
    leds.switchLed(LED_N2K, n2k_stats.sent, n2k_stats.canbus);
  }
}

static void dump_loop_time(const char *name, unsigned long micros)
{
  Log::tracex(APP_LOG_TAG, "Stats", "%s Loop Time {%lu} micros", name, micros);
}

void dump_process_stats()
{
  Log::tracex(APP_LOG_TAG, "Stats", "Cycles {%d} in 10s", app_stats.cycles);
  dump_loop_time("N2K", app_stats.n2k_loop_time);
  dump_loop_time("GPS", app_stats.gps.loop_time);
  dump_loop_time("DHT", app_stats.dht.loop_time);
  dump_loop_time("BME", app_stats.bme.loop_time);
  dump_loop_time("BMV712", app_stats.bmv712.loop_time);
  dump_loop_time("Tacho", app_stats.tacho.loop_time);
  dump_loop_time("Display", app_stats.display.loop_time);
  dump_loop_time("Leds", app_stats.leds.loop_time);
  dump_loop_time("Environment Messenger", app_stats.environment_messenger.loop_time);
  dump_loop_time("Water Temp", app_stats.water_temp.loop_time);
  dump_loop_time("STW Paddle", app_stats.stw_paddle.loop_time);
  dump_loop_time("BLE Conf", app_stats.bleConf_loop_time);
  app_stats.reset_loop_time();
}

void report_stats(unsigned long ms)
{
  static unsigned long last_time_stats_ms = 0;
  if (check_elapsed(ms, last_time_stats_ms, 10000000))
  {
    gps.dumpStats();
    tacho.dumpStats();
    n2k.getStats().dump();
    Log::tracex(APP_LOG_TAG, "Stats", "N2K task stack free {%u} bytes", n2k.get_task_stack_free());
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
    process_deferred_commands(deferred, conf, engineHours, cache);
    process_deferred_source_claim(deferred, conf);
    unsigned int n2k_activity = deferred.take_n2k_activity();
    if (n2k_activity)
    {
      leds.blink(LED_N2K, t, N2K_BLINK_USEC, (n2k_activity & DeferredEvents::ACTIVITY_FAILED) != 0);
    }
    handle_agent_loop(leds, context, true, app_stats.leds, t, "Leds");
    handle_agent_loop(resetButton, context, true, NULL, t, "RESET");
    handle_agent_loop(display, context, true, app_stats.display, t, "Display");
    handle_agent_loop(gps, context, conf.get_services().is_use_gps(), app_stats.gps, t, "GPS");
    handle_agent_loop(bme, context, conf.get_services().is_use_bme(), app_stats.bme, t, "BME");
    handle_agent_loop(dht, context, conf.get_services().is_use_dht(), app_stats.dht, t, "DHT");
    handle_agent_loop(bmv712, context, conf.get_services().is_use_vedirect(), app_stats.bmv712, t, "BMV712");
    handle_agent_loop(tacho, context, conf.get_services().is_use_tacho(), app_stats.tacho, t, "TACHO");
    handle_agent_loop(speedThroughWater, context, conf.get_services().is_use_stw_paddle(), app_stats.stw_paddle, t, "STW");
    handle_agent_loop(waterTemp, context, conf.get_services().is_use_tmp(), app_stats.water_temp, t, "WTRTEMP");
    handle_agent_loop(environmentMessenger, context, true, app_stats.environment_messenger, t, "ENV");
    app_stats.bleConf_loop_time += handle_agent_loop(bleConf, context, true, NULL, t, "BLE");
    handle_display(t);
    handle_leds(t);
    report_stats(t);
  }
  delay(5);
}

// runs on the loop task (the reset button is an agent, the 'R' command is deferred) just before the board restarts
static void flush_before_restart()
{
#if DO_TACHOMETER == 1
  if (tacho.flush())
  {
    Log::tracex(APP_LOG_TAG, "Restart", "Engine hours saved");
  }
#endif
}

static void restart_from_command()
{
  flush_before_restart();
  default_restarter();
}

void _setup()
{  
  setCpuFrequencyMhz(160);
  uint32_t f1 = getCpuFrequencyMhz();


  #if DO_LOGGER == 1
  Serial.begin(115200);
  msleep(3500);
  Log::enable();
  #else
  Log::disable();
  #endif

  unsigned long ver = __cplusplus;
  Log::tracex(APP_LOG_TAG, "CPU", "Freq {%d} C++ {%l}", f1, ver);
  conf.init();
  #if DO_LOGGER == 1
  if (!conf.get_services().is_use_logger())
  {
    Log::disable();
  }
  #endif
  engineHours.init();
  Log::tracex(APP_LOG_TAG, "Engine Hours", "Loaded engine time {%lu.%03d}", 
    (uint32_t)(engineHours.get_engine_hours() / 1000L), (uint16_t)(engineHours.get_engine_hours() % 1000L));
  
  N2K::set_sent_message_callback(on_message_sent);  
  n2k.set_desired_source(conf.get_n2k_source());
  n2k.setup(context);
  msleep(500);
  display.setup(context);
  leds.setup(context);
  resetButton.setup(context);
#if RESET_PIN != -1
  resetButton.set_before_restart(flush_before_restart);
#endif
  CommandHandler::set_restart_handler(restart_from_command);
  gps.setup(context);
  dht.setup(context);
  bme.setup(context);
  bleConf.setup(context);
  bmv712.setup(context);
  tacho.setup(context);
  speedThroughWater.setup(context);
  waterTemp.setup(context);
  environmentMessenger.setup(context);
  msleep(500);
  started = true;

  leds.on(LED_PWR);
}

void on_command(char command, const char *command_value)
{
  if (command == 'h')
    return; // heartbeat: BLEConf already reset its inactivity timer, nothing to apply
  deferred.post_command(command, command_value);
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