#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "NMEA.h"
#include "N2K.h"
#include "constants.h"
#include "Utils.h"
#include <time.h>
#include <TimeLib.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

WiFiUDP udp;
N2K n2k;

#define I2C_SDA 22
#define I2C_SCL 21
#define SEALEVELPRESSURE_HPA (1013.25)

TwoWire I2CBME = TwoWire(0);
Adafruit_BMP280* bmp;
bool g_bmp_initialized = false;

struct nmea_stats {
  uint valid_rmc = 0;
  uint valid_gsa = 0;
  uint invalid_rmc = 0;
  uint invalid_gsa = 0;
} g_stats;

RMC g_rmc;
GSA g_gsa;

bool g_time_set = false;
bool g_initialized = false;

void initWiFi() {
  WiFi.begin(SSID, PSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    debug_println("Connecting to WiFi..");
  }
  debug_println("Connected to the WiFi network");  
}

void initGPS() {
  Serial.print("Initializing GPS ");
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  debug_println("- OK");
}

void sendUDPPacket(const uint8_t* bfr, int l) {
  static uint8_t term[] = {13,10};
  debug_print("UDP Sending {%s}\n", bfr);
  udp.beginPacket(UDP_DEST, UDP_PORT);
  udp.write(bfr, l);
  udp.write(term, 2);
  udp.endPacket();
}

void report_stats() {
  static time_t last_report_time = 0;
  if (g_time_set) {
    time_t t = time(0);
    if ((t - last_report_time) > 30) {
        last_report_time = t;
        debug_print("RMC %d/%d GSA %d/%d Sats %d Fix %d Sent %d/%d\n",
              g_stats.valid_rmc, g_stats.invalid_rmc,
              g_stats.valid_gsa, g_stats.invalid_gsa,
              g_gsa.nSat, g_gsa.fix,
              n2k.get_sent(), n2k.get_sent_fail());
        memset(&g_stats, 0, sizeof(nmea_stats));
        n2k.reset_counters();
    }
  }
}

int parse_and_send(const char *sentence) {
    if (NMEAUtils::is_sentence(sentence, "RMC")) {
        if (NMEAUtils::parseRMC(sentence, g_rmc) == 0) {
            if (g_rmc.valid) g_stats.valid_rmc++; else g_stats.invalid_rmc++;
            debug_print("Process RMC {%s}\n", sentence);
            if (!g_time_set) {
              if (g_rmc.y>0) {
                debug_print("Setting time to {%d-%d-%d %d:%d:%d}}\n", g_rmc.y, g_rmc.M, g_rmc.d, g_rmc.h, g_rmc.m, g_rmc.s);
                setTime(g_rmc.h, g_rmc.m, g_rmc.s, g_rmc.d, g_rmc.M, g_rmc.y);
                g_time_set = true;
              }
            }
            if (g_time_set) {
              n2k.sendCOGSOG(g_gsa, g_rmc);
              n2k.sendPosition(g_gsa, g_rmc);
            }
            return OK;
        }
    } else if (NMEAUtils::is_sentence(sentence, "GSA")) {
        if (NMEAUtils::parseGSA(sentence, g_gsa) == 0) {
            if (g_gsa.valid) g_stats.valid_gsa++; else g_stats.invalid_gsa++;
            debug_print("Process GSA {%s}\n", sentence);
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
  
  //if (!g_time_set) return;   

  int pgn = N2kMsg.PGN;
  int src = N2kMsg.Source;
  int dst = N2kMsg.Destination;
  int priority = N2kMsg.Priority;
  int dataLen = N2kMsg.DataLen;
  const unsigned char* data = N2kMsg.Data;
  unsigned long msgTime = N2kMsg.MsgTime;

  time_t _now = now();
  time_t _t0 = time(0);
  //time, priority, pgn, source, dest, len data
  //2011-11-24-22:42:04.388,2,127251,36,255,8,7d,0b,7d,02,00,ff,ff,ff
  time_t ts = (msgTime / 1000) + (_now - _t0);
  int ts_millis = msgTime % 1000;
  tm t;
  gmtime_r(&ts, &t);

  char buffer[512];
  sprintf(buffer, "%04d-%02d-%02d-%02d:%02d:%02d.%03d,%d,%d,%d,%d,%d,", 
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
  sendUDPPacket((uint8_t*)buffer, strlen(buffer));
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  n2k.setup(handleMsg);
  initWiFi();
  initGPS();

  debug_print("Initializing BMP sensor ");
  debug_print(" TwoWires {%d} ", I2CBME.begin(I2C_SDA, I2C_SCL, 100000));
  bmp = new Adafruit_BMP280(&I2CBME);
  g_bmp_initialized = bmp->begin(0x76, 0x60);
  debug_print(" BMP280 {%d}\n", g_bmp_initialized);

  delay(2000);
  g_initialized = true;
}

void send_system_time(unsigned long ms) {
  static unsigned long t0 = 0;
  if ((ms-t0)>1000) {
    n2k.sendTime();
    t0 = ms;
  }
}

void read_pressure(unsigned long ms) {
  static unsigned long t0 = 0;
  if ((ms-t0)>5000 && g_bmp_initialized) {
    float pressure = bmp->readPressure();
    Serial.printf("Read {%f} pressure\n", pressure);
    n2k.sendPressure(pressure);
    t0 = ms;
  }
}

void loop() {
  unsigned long ms = millis();
  if (g_initialized) {
    readGPS(250000);
    send_system_time(ms);
    report_stats();
    read_pressure(ms);
    n2k.loop();
  }
}
