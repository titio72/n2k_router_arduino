#include "constants.h"

#ifdef ESP32_ARCH
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include "DHTesp.h"
#endif

#include "NMEA.h"
#include "Conf.h"
#include "N2K.h"
#include "Ports.h"
#include "Utils.h"
#include "Network.h"
#include "Log.h"
#include "Log.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

Configuration conf;
statistics stats;
statistics last_stats;
data cache;

#ifdef ESP32_ARCH
TwoWire I2CBME = TwoWire(0);
Adafruit_BMP280 *bmp;
DHTesp DHT;
#endif

N2K n2k;
NMEAUtils nmea;
Port p("/dev/ttyUSB1");
NetworkHub ntwrk(UDP_PORT, UDP_DEST, &cache, &conf, &last_stats, &n2k);

bool initialized = false;
bool bmp_initialized = false;
bool gps_initialized;
bool gps_time_set = false;

int status = 0;

time_t delta_time = 0;

void read_conf()
{
  conf.load();
}

void enableGPS()
{
  if (!gps_initialized)
  {
    gps_initialized = true;
  }
}

void disableGPS()
{
  if (gps_initialized)
  {
    gps_initialized = false;
    p.close();
  }
}

void enableBMP280()
{
#ifdef ESP32_ARCH
  if (!bmp_initialized)
  {
    Log::trace("[BMP] ");
    bool two_wires_ok = I2CBME.begin(I2C_SDA, I2C_SCL, 100000);
    Log::trace(" TwoWires {%d} ", two_wires_ok);
    if (two_wires_ok)
    {
      bmp = new Adafruit_BMP280(&I2CBME);
      bmp_initialized = bmp->begin(0x76, 0x60);
      if (!bmp_initialized)
      {
        delete bmp;
      }
    }
    Log::trace(" {%d}\n", bmp_initialized);
  }
#endif
}

void disableBMP280()
{
#ifdef ESP32_ARCH
  if (bmp_initialized)
  {
    Log::trace("[BMP] Closing sensor ");
    delete bmp;
    bmp_initialized = false;
  }
#endif
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

    Log::trace("[STATS] Bus: UDP {%d/%d} CAN.TX {%d/%d} CAN.RX {%d} UART {%d B} in 10s\n",
               stats.udp_sent, stats.udp_failed, stats.can_sent, stats.can_failed, stats.can_received, stats.bytes_uart);

    Log::trace("[STATS] OPS: Cycles {%d} Pauses {%d} in 10s\n", stats.cycles, stats.pauses);

    last_time_stats_ms = ms;

    memcpy(&last_stats, &stats, sizeof(statistics));
    memset(&stats, 0, sizeof(statistics));
  }
}

time_t get_time(RMC &rmc, time_t &t, short &ms)
{
  tm _gps_time;
  _gps_time.tm_hour = rmc.h;
  _gps_time.tm_min = rmc.m;
  _gps_time.tm_sec = rmc.s;
  _gps_time.tm_year = rmc.y - 1900;
  _gps_time.tm_mon = rmc.M - 1;
  _gps_time.tm_mday = rmc.d;
  t = mktime(&_gps_time);
  ms = rmc.ms;
  return t;
}

bool set_system_time(int sid, RMC &rmc, bool &time_set_flag)
{
  if (rmc.y > 0)
  {
    time_t _gps_time_t;
    short _gps_ms;
    get_time(rmc, _gps_time_t, _gps_ms);
    if (conf.send_time)
    {
      n2k.sendTime(rmc, sid);
    }
    if (!time_set_flag)
    {
      delta_time = _gps_time_t - time(0);
      Log::trace("[GPS] Setting time to {%d-%d-%d %d:%d:%d.%d}}\n", rmc.y, rmc.M, rmc.d, rmc.h, rmc.m, rmc.s, rmc.ms);
      time_set_flag = true;
    }
  }
  return false;
}

int parse_and_send(const char *sentence)
{
  //Serial.println(sentence);
  static unsigned char sid = 0;
  sid++;
  Log::debug("[GPS] Process Sentence {%s}\n", sentence);
  if (NMEAUtils::is_sentence(sentence, "RMC"))
  {
    if (NMEAUtils::parseRMC(sentence, cache.rmc) == 0)
    {
      set_system_time(sid, cache.rmc, gps_time_set);
      if (cache.rmc.valid)
      {
        n2k.sendCOGSOG(cache.gsa, cache.rmc, sid);
        n2k.sendPosition(cache.gsa, cache.rmc);
        static ulong t0 = millis();
        ulong t = millis();
        if ((t - t0) > 900)
        {
          n2k.sendGNSSPosition(cache.gsa, cache.rmc, sid);
          t0 = t;
        }
        stats.valid_rmc++;
        return 0;
      }
      else
      {
        stats.invalid_rmc++;
      }
    }
  }
  else if (NMEAUtils::is_sentence(sentence, "GSA"))
  {
    if (nmea.parseGSA(sentence, cache.gsa) == 0)
    {
      if (cache.gsa.valid)
      {
        stats.valid_gsa++;
        stats.gps_fix = cache.gsa.fix;
      }
      else
      {
        stats.invalid_gsa++;
        stats.gps_fix = 0;
      }
      return 0;
    }
  }
  else if (NMEAUtils::is_sentence(sentence, "GSV"))
  {
    if (nmea.parseGSV(sentence) == 0)
    {
      stats.valid_gsv++;
    }
    else
    {
      stats.invalid_gsv++;
    }
  }
  return -1;
}

void send_gsv(ulong ms)
{
  static unsigned char sid = 0;
  sid++;
  static ulong last_sent = 0;
  if ((ms - last_sent) >= 900)
  {
    last_sent = ms;
    n2k.sendGNNSStatus(cache.gsa, sid);
    n2k.sendSatellites(nmea.get_satellites(), nmea.get_n_satellites(), sid, cache.gsa);
  }
}

void read_pressure()
{
#ifdef ESP32_ARCH
  cache.pressure = N2kDoubleNA;
  if (bmp_initialized)
  {
    cache.pressure = bmp->readPressure();
    cache.temperature_el = bmp->readTemperature();
  }
#endif
}

void read_temp_hum(ulong ms)
{
#ifdef ESP32_ARCH
  static ulong last_dht_time = 0;
  cache.humidity = N2kDoubleNA;
  cache.temperature = N2kDoubleNA;
  if (conf.use_dht11)
  {
    if ((ms - last_dht_time)>DHT.getMinimumSamplingPeriod()) {
      TempAndHumidity th = DHT.getTempAndHumidity();
      cache.humidity = th.humidity;
      cache.temperature = th.temperature;
      DHT.getTempAndHumidity();
    }
  }
#endif
}

void send_env(unsigned long ms)
{
#ifdef ESP32_ARCH
  static unsigned long t0 = 0;
  static unsigned char sid = 0;
  if ((ms - t0) >= 2000)
  {
    read_pressure();
    read_temp_hum(ms);
    n2k.sendEnvironment(cache.pressure, cache.humidity, cache.temperature, sid);
    n2k.sendElectronicTemperature(cache.temperature_el, sid);
    t0 = ms;
    sid++;
  }
#endif
}

void msg_handler(const tN2kMsg &N2kMsg)
{

  if (!initialized)
    return;

  int pgn = N2kMsg.PGN;
  int src = N2kMsg.Source;
  int dst = N2kMsg.Destination;
  int priority = N2kMsg.Priority;
  int dataLen = N2kMsg.DataLen;
  const unsigned char *data = N2kMsg.Data;
  unsigned long msgTime = N2kMsg.MsgTime;

  int ts_millis = msgTime % 1000;
  time_t ts = msgTime / 1000;
  tm t;
  gmtime_r(&ts, &t);

  static char buffer[2048];
  snprintf(buffer, 2048, "%04d-%02d-%02d-%02d:%02d:%02d.%03d,%d,%d,%d,%d,%d,",
           t.tm_year + 2000, t.tm_mon + 1, t.tm_mday,
           t.tm_hour, t.tm_min, t.tm_sec, ts_millis,
           priority, pgn, src, dst, dataLen);
  char *x = buffer + strlen(buffer);
  for (int i = 0; i < dataLen; i++)
  {
    sprintf(x, "%02x", data[i]);
    x += sizeof(char) * 2;
    if (i != (dataLen - 1))
    {
      sprintf(x, ",");
      x += sizeof(char);
    }
  }

  if (conf.wifi_broadcast) {
    if (ntwrk.send_udp(buffer, strlen(buffer)))
      stats.udp_sent++;
    else
      stats.udp_failed++;
  }
}

void setup()
{
#ifdef ESP32_ARCH
  Serial.begin(115200);
  delay(1000);
  read_conf();
#endif
  ntwrk.begin();
  status = 1;
  n2k.setup(msg_handler, &stats, conf.src);
  p.set_handler(parse_and_send);
  DHT.setup(DHTPIN, (conf.dht11_dht22==CONF_DHT11)?DHTesp::DHT11:DHTesp::DHT22);
  initialized = true;
}

void loop()
{
  stats.cycles++;

  static ulong t0 = _millis();
  ulong t = _millis();
  if ((t - t0) < 25)
  {
    stats.pauses++;
    msleep(25);
  }
  t0 = t;

  if (initialized)
  {
    ntwrk.loop(t);

    n2k.loop();

    if (conf.use_gps)
    {
      p.set_speed(atoi(UART_SPEED[conf.uart_speed]));
      p.listen(250);
      stats.bytes_uart += p.get_bytes();
      p.reset_bytes();
      send_gsv(t);
    }
    else
    {
      p.close();
    }

    if (conf.use_bmp280)
    {
      enableBMP280();
    }
    else
    {
      disableBMP280();
    }

    if (conf.use_dht11 || conf.use_bmp280)
    {
      send_env(t);
    }

    report_stats(t);
  }
}

#ifndef ESP32_ARCH
int main(int argc, const char **argv)
{

  setup();
  while (1)
  {
    loop();
  }
}
#endif