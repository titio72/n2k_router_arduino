#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef ESP32_ARCH
#include <Arduino.h>
#include <Esp.h>
#else
#include <signal.h>
#endif

#include "Constants.h"
#include "Context.h"
#include "Conf.h"
#include "Simulator.h"
#include "N2K.h"
#include "Utils.h"
#include "Network.h"
#include "Log.h"
#include "GPS.h"
#include "HumTemp.h"
#include "PressureTemp.h"
#include "Display.h"

#pragma region CONTEXT
Configuration conf;
statistics stats;
data cache;
N2K n2k(NULL, &stats);
Context context(n2k, conf, stats, cache);
#pragma endregion

#pragma region AGENTS
NetworkHub ntwrk(context);
GPS gps(context);
HumTemp dht(context);
PressureTemp bmp(context);
Simulator simulator(context);
EVODisplay display;
#pragma endregion

bool initialized = false;

void read_conf()
{
  conf.load();
}

void send_env(unsigned long ms)
{
  static unsigned long t0 = 0;
  static unsigned char sid = 0;
  if ((ms - t0) >= 2000)
  {
    n2k.sendPressure(cache.pressure, sid);
    n2k.sendCabinTemp(cache.temperature, sid);
    n2k.sendHumidity(cache.humidity, sid);
    n2k.sendElectronicTemperature(cache.temperature_el, sid);
    t0 = ms;
    sid++;

    static char temperature_text[32];
    sprintf(temperature_text, "%5.2f C\n%6.1f Mb", cache.temperature, cache.pressure/100.0f);
    display.draw_text(temperature_text);
    display.blink(ms, 500);
  }
}

void report_stats(unsigned long ms)
{
  static unsigned long last_time_stats_ms = 0;
  if ((ms - last_time_stats_ms) > 10000)
  {
    if (conf.use_gps)
    {
      Log::trace("[STATS] GPS: RMC {%d/%d} GSA {%d/%d} GSV {%d/%d} in 10s - Sats {%d} Fix {%d}\n",
                 stats.valid_rmc, stats.invalid_rmc,
                 stats.valid_gsa, stats.invalid_gsa,
                 stats.valid_gsv, stats.invalid_gsv,
                 cache.gsa.nSat, cache.gsa.fix);
    }

    Log::trace("[STATS] Bus: CAN.TX {%d/%d} CAN.RX {%d} UART {%d B} in 10s\n",
               stats.can_sent, stats.can_failed, stats.can_received, stats.bytes_uart);

    Log::trace("[STATS] OPS: Cycles {%d} Pauses {%d} in 10s\n", stats.cycles, stats.pauses);

    last_time_stats_ms = ms;

    memset(&stats, 0, sizeof(statistics));
  }
}

void setup()
{
  #ifdef ESP32_ARCH
  Serial.begin(115200);
  #endif
  read_conf();
  msleep(500);
  n2k.setup();
  ntwrk.setup();
  display.setup();
  gps.setup();
  dht.setup();
  bmp.setup();
  initialized = true;
}

ulong handle_loop_throttling()
{
  static ulong t0 = _millis();
  ulong t = _millis();
  if ((t - t0) < 25)
  {
    stats.pauses++;
    msleep(25);
  }
  t0 = t;
  return t;
}

template<typename T> void handle_agent_loop(T &agent, bool enable, unsigned long t)
{
  if (enable)
  {
    agent.enable();
    if (agent.is_enabled()) agent.loop(t);
  }
  else
  {
    agent.disable();
  }
}

void loop()
{
  stats.cycles++;
  ulong t = handle_loop_throttling();
  if (initialized)
  {
    n2k.loop(t);
    handle_agent_loop(ntwrk, true, t);
    handle_agent_loop(display, true, t);
    handle_agent_loop(gps, conf.use_gps, t);
    handle_agent_loop(bmp, conf.use_bmp280, t);
    handle_agent_loop(dht, conf.use_dht11, t);
    handle_agent_loop(simulator, conf.simulator, t);
    if (conf.use_dht11 || conf.use_bmp280 || conf.simulator)
    {
      send_env(t);
    }
    report_stats(t);
  }
}

#ifndef ESP32_ARCH

#define NO_ARG -1

int is_arg(const char* arg, int argc, const char **argv)
{
  for (int i = 0; i<argc; i++) {
    if (strcmp(arg, argv[i])==0) return i;
  }
  return -1;
}

const char* get_arg(const char* arg_name, int argc, const char** argv)
{
  int _arg = is_arg(arg_name, argc, argv);
  if (_arg!=NO_ARG && argc>_arg) return argv[_arg+1];
  else return NULL;
}

void  stop_handler(int sig)
{
  Log::trace("Stopping\n");
  signal(sig, SIG_IGN);
  ntwrk.disable();
  exit(0);
}

int main(int argc, const char **argv)
{
  if (is_arg("help", argc, argv)!=NO_ARG)
  {
    printf("usage: n2k_router <options>\n");
    printf("Options:\n");
    printf("  gps - stars reading GPS from serial input (ttyUSB1).\n");
    printf("  sim - stars simulator.\n");
    printf("  can <can soket name> - sets the can socket device name (def. vcan0).\n");
    printf("Example: n2k_router sim can can1\n");
  }
  else
  {
    signal(SIGINT, stop_handler);
    conf.use_gps = is_arg("gps", argc, argv)!=NO_ARG;
    conf.simulator = is_arg("sim", argc, argv)!=NO_ARG;
    conf.use_dht11 = false;
    conf.use_bmp280 = false;
    conf.send_time = false;

    const char* can_device = get_arg("can", argc, argv);
    if (can_device)
    {
      n2k.set_can_socket_name(can_device);
      Log::trace("Set CAN socket device: {%s}\n", can_device);
    }
    else
    {
      n2k.set_can_socket_name("vcan0");
      Log::trace("Set default CAN socket device: {vcan0}\n");
    }

    const char* gps_device = get_arg("gps", argc, argv);
    if (gps_device)
    {
      gps.set_port_name(gps_device);
      Log::trace("Set GPS socket device: {%s}\n", gps_device);
    }
    else
    {
      gps.set_port_name(DEFAULT_GPS_DEVICE);
      Log::trace("Set defaut GPS socket device: {%s}\n", DEFAULT_GPS_DEVICE);
    }

    setup();
    while (1)
    {
      loop();
    }
  }
}
#endif
