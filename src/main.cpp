#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Arduino.h>
#include <Esp.h>
#include <ArduinoPort.h>

#include <N2K.h>
#include <Utils.h>
#include <Log.h>

#include "Context.h"
#include "Conf.h"
#include "Simulator.h"
#include "N2K_router.h"
#if GPS_I2C==1
#include "GPS_I2C.h"
#else
#include "GPS.h"
#endif
#include "HumTemp.h"
#include "PressureTemp.h"
#include "Display.h"
#include "NMEA.h"
#include "BMV712.h"
#include "BLEConf.h"
#include <Ports.h>

#define DO_BMV     0

void on_source_claim(const unsigned char old_source, const unsigned char new_source);

#pragma region CONTEXT
Configuration conf;
data cache;
N2K n2k_bus = *(N2K::get_instance(NULL, on_source_claim));
N2K_router n2k(n2k_bus);
Context context(n2k, conf, cache);
#pragma endregion

#pragma region AGENTS
#if GPS_I2C==1
GPSX gps(context);
#else
Port* gpsPort = new ArduinoPort(Serial1, DEFAULT_GPS_SPEED, GPS_RX_PIN, GPS_TX_PIN, false);
GPS gps(context, gpsPort);
#endif
#if (DO_BMV==1)
ArduinoPort veDirectPort(Serial1, VE_DIRECT_RX_PIN, VE_DIRECT_TX_PIN, true);
BMV712 bmv712(context, veDirectPort);
#endif
HumTemp dht(context);
PressureTemp bmp(context);
Simulator simulator(context);
EVODisplay display;
BLEConf bleConf(context);
#pragma endregion

bool started = false;

struct AppStats
{
  unsigned long cycles = 0;
  unsigned long pauses = 0;
  unsigned short retry_gps = 0;
  unsigned short retry_dht = 0;
  unsigned short retry_bmp = 0;
  unsigned short retry_display = 0;
} app_stats;

#define MAX_RETRY 3

void read_conf()
{
  conf.load();
}

void on_source_claim(const unsigned char old_source, const unsigned char new_source)
{
  conf.n2k_source = new_source;
  Log::trace("[APP] Save new claimed n2k source {%d}\n", new_source);
  conf.save();
}

void send_env(unsigned long ms)
{
  static unsigned long t0 = 0;
  if ((ms - t0) >= 2000)
  {
    n2k.sendPressure(cache.pressure);
    n2k.sendCabinTemp(cache.temperature);
    n2k.sendHumidity(cache.humidity);
    n2k.sendEnvironmentXRaymarine(cache.pressure, cache.humidity, cache.temperature);
    n2k.sendElectronicTemperature(cache.temperature_el);
    t0 = ms;

    static char temperature_text[32];
    sprintf(temperature_text, "%5.2fC\n%6.1f Mb", cache.temperature, cache.pressure/100.0f);
    //Log::trace("%lu %s\n", ms, temperature_text);
    display.draw_text(temperature_text);
    display.blink(ms, 200);
  }
}

void dumpStats()
{
  Log::trace("[STATS] OPS: Cycles {%d} Pauses {%d} in 10s\n", app_stats.cycles, app_stats.pauses);
}

void report_stats(unsigned long ms)
{
  static unsigned long last_time_stats_ms = 0;
  if ((ms - last_time_stats_ms) > 10000)
  {
    last_time_stats_ms = ms;

    gps.dumpStats();
    n2k_bus.getStats().dump();
    dumpStats();

    app_stats.cycles = 0;
    app_stats.pauses = 0;
  }
}

unsigned long throttle_main_loop()
{
  static unsigned long t0 = _millis();
  unsigned long t = _millis();
  if ((t - t0) < 25)
  {
    app_stats.pauses++;
    msleep(25);
  }
  t0 = t;
  return t;
}

template<typename T> bool handle_agent_enable(T &agent, bool enable, unsigned short* retry, unsigned long t, const char* desc="")
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
          if ((*retry)>=MAX_RETRY) Log::trace("[APP] Exceeded enable retry {%s}\n", desc);
        }
      }
    }
  }
  return agent.is_enabled();
}

template<typename T> void handle_agent_loop(T &agent, bool enable, unsigned short* retry, unsigned long t, const char* desc="")
{
  if (enable)
  {
    if (handle_agent_enable(agent, enable, retry, t, desc)) agent.loop(t);
  }
  else
  {
    agent.disable();
    if (retry) (*retry) = 0;
  }
}

void loop()
{
  app_stats.cycles++;
  unsigned long t = throttle_main_loop();
  if (started)
  {
    n2k_bus.loop(t);
    handle_agent_loop(display, true, &app_stats.retry_display, t, "Display");
    if (!conf.simulator)
    {
      handle_agent_loop(gps, conf.use_gps, &app_stats.retry_gps, t, "GPS");
      handle_agent_loop(bmp, conf.use_bmp, &app_stats.retry_bmp, t, "BMP");
      handle_agent_loop(dht, conf.use_dht, &app_stats.retry_dht, t, "DHT");
      #if (DO_BMV==1)
      handle_agent_loop(bmv712, true, t);
      #endif
      if (conf.use_dht || conf.use_bmp || conf.simulator)
      {
        send_env(t);
      }
    }
    else
    {
      handle_agent_loop(simulator, conf.simulator, NULL, t, "Sim");
    }
    handle_agent_loop(bleConf, true, NULL, t, "BLE");
    report_stats(t);
  }
}

void setup()
{
  Serial.begin(115200);
  msleep(2500);
  uint32_t f = getCpuFrequencyMhz();
  bool res_cpu_freq = setCpuFrequencyMhz(80);
  uint32_t f1 = getCpuFrequencyMhz();
  Log::trace("[APP] CPU Frequency {%d}\n", f1);
  read_conf();
  n2k_bus.set_desired_source(conf.n2k_source);
  msleep(2500);
  n2k_bus.setup();
  msleep(500);
  display.setup();
  gps.setup();
  dht.setup();
  bmp.setup();
  bleConf.setup();
  #if (DO_BMV==1)
  bmv712.setup();
  #endif
  msleep(500);
  started = true;
}