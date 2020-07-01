#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "NMEA.h"
#include "N2K.h"
#include "constants.h"
#include "Utils.h"
#include <time.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include "dhtAB.h"
#include "WiFiManager.h"
#include "WebServer.h"

#define I2C_SDA 22
#define I2C_SCL 21
#define SEALEVELPRESSURE_HPA (1013.25)
#define DHTPIN 4

WEBServer web;
WiFiManager* wifi;
N2K n2k;
TwoWire I2CBME = TwoWire(0);
Adafruit_BMP280* bmp;
dht DHT;

configuration conf;
statistics stats;
data cache;

bool initialized = false;

bool bmp_initialized = false;

bool gps_initialized;
bool gps_time_set = false;

int status = 0;

time_t delta_time = 0;

void enableGPS() {
  if (!gps_initialized) {
    debug_println("Initializing GPS");
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
    gps_initialized = true;
    debug_print("  GPS {%d}\n", gps_initialized);
  }
}

void disableGPS() {
  if (gps_initialized) {
    gps_initialized = false;
    Serial.println("Closing GPS");
    Serial2.end();
  }
}

void enableBMP280() {
  if (!bmp_initialized) {
    debug_print("Initializing BMP sensor ");
    bool two_wires_ok = I2CBME.begin(I2C_SDA, I2C_SCL, 100000);
    debug_print(" TwoWires {%d} ", two_wires_ok);
    if (two_wires_ok) {
      bmp = new Adafruit_BMP280(&I2CBME);
      bmp_initialized = bmp->begin(0x76, 0x60);
      if (!bmp_initialized) {
        delete bmp;
      }
    }
    debug_print(" BMP280 {%d}\n", bmp_initialized);
  }
}

void disableBMP280() {
  if (bmp_initialized) {
    debug_print("Closing BMP sensor ");
    delete bmp;
    bmp_initialized = false;
  }
}

void report_stats(unsigned long ms) {
  static unsigned long last_time_stats_ms = 0;
  if ((ms-last_time_stats_ms)>10000) {
    if (conf.use_gps) {
      if (gps_time_set) {
        debug_print("Stats: RMC %d/%d GSA %d/%d Sats %d Fix %d\n",
              stats.valid_rmc, stats.invalid_rmc,
              stats.valid_gsa, stats.invalid_gsa,
              cache.gsa.nSat, cache.gsa.fix);
      }
    }

    debug_print("Stats: UDP {%d} CAN {%d %d} in 10s\n", stats.udp_sent, n2k.get_sent(), n2k.get_sent_fail());
    
    n2k.reset_counters();

    last_time_stats_ms = ms;

    memset(&stats, 0, sizeof(statistics));
  }
}

int parse_and_send(const char *sentence) {
    if (NMEAUtils::is_sentence(sentence, "RMC")) {
        if (NMEAUtils::parseRMC(sentence, cache.rmc) == 0) {
            if (cache.rmc.valid) stats.valid_rmc++; else stats.invalid_rmc++;
            //debug_print("Process RMC {%s}\n", sentence);
            if (!gps_time_set) {
              if (cache.rmc.y>0) {
                tm _gps_time;
                _gps_time.tm_hour = cache.rmc.h;
                _gps_time.tm_min = cache.rmc.m;
                _gps_time.tm_sec = cache.rmc.s;
                _gps_time.tm_year = cache.rmc.y - 1900;
                _gps_time.tm_mon = cache.rmc.M - 1;
                _gps_time.tm_mday = cache.rmc.d;
                time_t _gps_time_t = mktime(&_gps_time);
                delta_time = _gps_time_t - time(0); 

                debug_print("Setting time to {%d-%d-%d %d:%d:%d}}\n", cache.rmc.y, cache.rmc.M, cache.rmc.d, cache.rmc.h, cache.rmc.m, cache.rmc.s);
                gps_time_set = true;
              }
            }
            if (gps_time_set && conf.use_gps) {
              n2k.sendCOGSOG(cache.gsa, cache.rmc);
              n2k.sendPosition(cache.gsa, cache.rmc);
            }
            return OK;
        }
    } else if (NMEAUtils::is_sentence(sentence, "GSA")) {
        if (NMEAUtils::parseGSA(sentence, cache.gsa) == 0) {
            if (cache.gsa.valid) stats.valid_gsa++; else stats.invalid_gsa++;
            //debug_print("Process GSA {%s}\n", sentence);
            return OK;
        }
    }
    return KO;
}

void readGPS(long microsecs) {
  static uint8_t buffer[256];
  static int ix = 0;

  long t0GPS = micros();
  while (Serial2.available() && (micros()-t0GPS)<microsecs) {
    uint8_t c = (uint8_t)Serial2.read();
    if (c==13 || c==10) {
      if (buffer[0]!=0) {
        parse_and_send((const char*)buffer);
        ix = 0;
        buffer[ix] = 0;
      }
    } else {        
      buffer[ix] = c;
      ix++;
      buffer[ix] = 0;
    }
  }
}

void handleMsg(const tN2kMsg &N2kMsg) {
  
  if (!initialized) return;
  
  int pgn = N2kMsg.PGN;
  int src = N2kMsg.Source;
  int dst = N2kMsg.Destination;
  int priority = N2kMsg.Priority;
  int dataLen = N2kMsg.DataLen;
  const unsigned char* data = N2kMsg.Data;
  unsigned long msgTime = N2kMsg.MsgTime;

  int ts_millis = msgTime % 1000;
  time_t ts = msgTime/1000;
  tm t;
  gmtime_r(&ts, &t);

  char buffer[2048];
  snprintf(buffer, 2048, "%04d-%02d-%02d-%02d:%02d:%02d.%03d,%d,%d,%d,%d,%d,", 
    t.tm_year + 2000, t.tm_mon + 1, t.tm_mday, 
    t.tm_hour, t.tm_min, t.tm_sec, ts_millis, 
    priority, pgn, src, dst, dataLen);
  char* x = buffer + strlen(buffer);
  for (int i = 0; i<dataLen; i++) {
    sprintf(x, "%02x", data[i]);
    x += sizeof(char)*2;
    if (i!=(dataLen-1)) {
      sprintf(x, ",");
      x += sizeof(char);
    }
  }

  stats.udp_sent += wifi->sendUDPPacket((uint8_t*)buffer, strlen(buffer));
}

void send_system_time(unsigned long ms) {
  static unsigned long t0 = 0;
  if ((ms-t0)>1000) {
    n2k.sendTime(time(0) + delta_time);
    t0 = ms;
  }
}

void read_pressure() {
  cache.pressure = N2kDoubleNA;
  if (bmp_initialized) {
    cache.pressure = bmp->readPressure();
    if (TRACE_DHT11) debug_print("Press {%.1f}\n", cache.pressure);
  }
}

void read_temp_hum() {
  cache.humidity = N2kDoubleNA;
  cache.temperature = N2kDoubleNA;
  if (conf.use_dht11) {
    int chk = DHT.read11(DHTPIN);
    switch (chk) {
      case DHTLIB_OK:  
        cache.humidity = DHT.humidity;
        cache.temperature = DHT.temperature;
        if (TRACE_BMP280) debug_print("Temp {%.1f} Hum {%.1f}\n", cache.temperature, cache.humidity);
        return;
        break;
      case DHTLIB_ERROR_CHECKSUM: 
        debug_print("DHT11 Checksum error\n"); 
        break;
      case DHTLIB_ERROR_TIMEOUT: 
        debug_print("DHT11 Time out error\n"); 
        break;
      default: 
        debug_print("DHT11 Unknown error\n"); 
        break;
    }
  }
}

void send_env(unsigned long ms) {
  static unsigned long t0 = 0;
  if ((ms-t0)>2000) {
    read_pressure();
    read_temp_hum();
    if (n2k.sendEnvironment(cache.pressure, cache.humidity, cache.temperature)) {
      stats.can_sent++;
    } else {
      debug_println("Failed to send environment message!");
    }
    t0 = ms;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  n2k.setup(handleMsg);
  wifi = new WiFiManager();
  initialized = true;
  status = 1;
}

void loop() {
  unsigned long ms = millis();
  if (initialized) {
    wifi->start();
    web.setup(&cache, &conf);

    if (conf.use_gps) {
      enableGPS();
      readGPS(250000);
    } else {
      disableGPS();
    }

    if (conf.use_bmp280) {
      enableBMP280();
    } else {
      disableBMP280();
    }

    if (conf.use_dht11 || conf.use_bmp280) {
      send_env(ms);
    }

    if (conf.send_time) {
      send_system_time(ms);
    }
    report_stats(ms);
    n2k.loop();
    web.on_loop(ms);
  }
}
