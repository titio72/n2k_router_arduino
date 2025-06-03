#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Arduino.h>
#include <Esp.h>
#include <ArduinoPort.hpp>

#include <N2K.h>
#include <Utils.h>
#include <Log.h>

#include "Context.h"
#include "Conf.h"
#include "N2K_router.h"
#if GPS_TYPE==1
#include "GPS_I2C.h"
#elif GPS_TYPE==2
#include "GPS.h"
#endif
#include "Dummy.h"
#if DO_TACHOMETER==1
#include "Tachometer.h"
#endif
#include "MeteoBME.h"
#include "MeteoDHT.h"
#include "Display.h"

#if DO_VE_DIRECT==1
#include "BMV712.h"
#endif

#define CMD_LOG_TAG "CMD"
#define APP_LOG_TAG "APP"

#include "EnvMessanger.h"
#include "BLEConf.h"
#include <Ports.h>

#ifndef BME_ADDRESS
#define BME_ADDRESS 0x76 // 0x76 or 0x77
#endif

#ifndef DHT_TYPE
#define DHT_TYPE DHT22
#endif

void on_source_claim(const unsigned char old_source, const unsigned char new_source);
void on_command(char command, const char* command_value);

#pragma region CONTEXT
Configuration conf;
Data cache;
N2K n2k_bus = *(N2K::get_instance(NULL, on_source_claim));
N2K_router n2k(n2k_bus);
Context context(n2k, conf, cache);
#pragma endregion

#pragma region AGENTS

#if GPS_TYPE==1
GPSX gps(context);
#elif GPS_TYPE==2
ArduinoPort<HardwareSerial> gpsPort("GPS", Serial1, UART_SPEED[UART_SPEED_9600], GPS_RX_PIN, GPS_TX_PIN, false);
GPS gps(context, gpsPort);
#else
Dummy gps;
#endif

#if (DO_VE_DIRECT==1)
ArduinoPort<HardwareSerial> veDirectPort("VE", Serial2, VE_DIRECT_RX_PIN, VE_DIRECT_TX_PIN, true);
BMV712 bmv712(context, veDirectPort, cache.battery_svc, cache.battery_eng);
#else
Dummy bmv712;
#endif

#if (DO_TACHOMETER==1)
Tachometer tacho(context, cache.engine, ENGINE_RPM_PIN, 12, 1.5, 1.0);
#else
DummyTachometer tacho;
#endif

MeteoDHT dht(context, DHT_PIN, MeteoDHT::DHT_MODEL::DHT_TYPE, cache.meteo_1);
MeteoBME bme(context, BME_ADDRESS, cache.meteo_0);
EVODisplay display;
BLEConf bleConf(context, on_command);
EnvMessanger envMessanger(context);
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
  unsigned short retry_env_messager = 0;
} app_stats;

#define MAX_RETRY 3
#define N2K_BLINK_USEC 100000L /* micros */

void on_source_claim(const unsigned char old_source, const unsigned char new_source)
{
  conf.save_n2k_source(new_source);
  Log::tracex(APP_LOG_TAG, "Save new claimed n2k source", " New Source {%d}", new_source);
}

void on_message_sent(const tN2kMsg &N2kMsg, bool success)
{
  display.blink(LED_N2K, micros(), N2K_BLINK_USEC, !success);
}

void handle_display(unsigned long ms)
{
  static unsigned long t0 = 0;
  if (check_elapsed(ms, t0, 1000000))
  {
    //display.draw_text("GPS %d\nSATS %d/%d", cache.gsa.fix, cache.gsa.nSat, cache.gsv.nSat);
    display.draw_text("%.1fmB\n%d%% %.1fC", cache.get_pressure(conf)/100.0f, (int)cache.get_humidity(conf), cache.get_temperature(conf));
    N2KStats n2k_stats = n2k.get_bus().getStats();
    static N2KStats prev = n2k_stats;

    if (cache.gsa.valid && cache.gsa.fix>1)
    {
      display.on(LED_GPS);
    }
    else
    {
      display.off(LED_GPS);
    }
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
    n2k_bus.getStats().dump();
    n2k_bus.getStats().reset();
    dump_process_stats();

    app_stats.cycles = 0;
  }
}

template<typename T> bool handle_agent_enable(T &agent, bool enable, unsigned short* retry, const char* desc="")
{
  if (!agent.is_enabled())
  {
    if (retry==NULL || (*retry)<MAX_RETRY)
    {
      agent.enable();
      if (agent.is_enabled())
      {
        if (retry) (*retry) = 0;
      }
      else
      {
        if (retry)
        {
          (*retry)++;
          if ((*retry)>=MAX_RETRY) Log::tracex(APP_LOG_TAG, "Exceeded enable retry", "Module {%s}", desc);
        }
      }
    }
  }
  return agent.is_enabled();
}

template<typename T> void handle_agent_loop(T &agent, bool enable, unsigned short* retry, unsigned long micros, const char* desc="")
{
  if (enable)
  {
    if (handle_agent_enable(agent, enable, retry, desc))
    {
      agent.loop(micros);
    }
  }
  else
  {
    agent.disable();
    if (retry)
    {
      (*retry) = 0;
    }
  }
}

void loop()
{
  app_stats.cycles++;
  unsigned long t = micros();
  if (started)
  {
    n2k_bus.loop(t);
    handle_agent_loop(display, true, &app_stats.retry_display, t, "Display");
    handle_agent_loop(gps, conf.get_services().use_gps, &app_stats.retry_gps, t, "GPS");
    handle_agent_loop(bme, conf.get_services().use_bme, &app_stats.retry_bme, t, "BMP");
    handle_agent_loop(dht, conf.get_services().use_dht, &app_stats.retry_dht, t, "DHT");
    handle_agent_loop(bmv712, conf.get_services().use_vedirect, &app_stats.retry_bmv712, t, "BMV712");
    handle_agent_loop(tacho, conf.get_services().use_tacho, &app_stats.retry_tacho, t, "TACHO");
    handle_agent_loop(envMessanger, true, &app_stats.retry_env_messager, t, "ENV");
    handle_agent_loop(bleConf, true, NULL, t, "BLE");
    handle_display(t);
    report_stats(t);
  }
}

void setup()
{
  uint32_t f = getCpuFrequencyMhz();
  bool res_cpu_freq = setCpuFrequencyMhz(80);
  uint32_t f1 = getCpuFrequencyMhz();
  N2K::set_sent_message_callback(on_message_sent);
  Serial.begin(115200);
  msleep(1000);
  auto ver = __cplusplus;
  Log::tracex(APP_LOG_TAG, "CPU", "Freq {%d} C++ {%l}", f1, ver);
  conf.init();
  n2k_bus.set_desired_source(conf.get_n2k_source());
  n2k_bus.setup();
  msleep(500);
  display.setup();
  gps.setup();
  dht.setup();
  bme.setup();
  bleConf.setup();
  bmv712.setup();
  tacho.setup();
  msleep(500);
  started = true;

  display.on(LED_PWR);
}

void on_command(char command, const char* command_value)
{
    switch (command)
    {
      case 'N':
        {
          Log::tracex(CMD_LOG_TAG, "Command set device name", "N {%s}", command_value);
          bleConf.set_device_name(command_value);
        }
        break;
      case 'C':
        {
          Log::tracex(CMD_LOG_TAG, "Command set services", "C {%s}", command_value);
          N2KServices s;
          s.from_string(command_value);
          conf.save_services(s);
        }
        break;
      case 'H':
        {
          Log::tracex(CMD_LOG_TAG, "Command set hours", "H {%s}", command_value);
          uint64_t engine_time_secs = atol(command_value);
          if (engine_time_secs>0)
          {
            uint64_t new_t = (uint64_t)1000 * engine_time_secs;
            Log::tracex(CMD_LOG_TAG, "Command set hours", "ms {%lu-%03d}", (uint32_t)(new_t/1000), (uint16_t)(new_t%1000));
            tacho.set_engine_time(new_t, true);
          }
        }
        break;
      case 'T':
        {
          Log::tracex(CMD_LOG_TAG, "Command tachometer calibration", "T {%s}", command_value);
          int rpm = atoi(command_value);
          if (rpm>0)
          {
            tacho.calibrate(rpm);
          }
        }
        break;
      case 't':
        {
          Log::tracex(CMD_LOG_TAG, "Command tachometer adjustment", "t {%s}", command_value);
          int adj = atoi(command_value);
          if (adj>0)
          {
            tacho.set_adjustment(adj / 10000.0, true);
          }
        }
        break;
      default:
        Log::tracex(CMD_LOG_TAG, "Unknown command", " CMD {%c} Value {%s}", command, command_value);
    }
}