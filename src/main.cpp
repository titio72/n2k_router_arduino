#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "NMEA.h"
#include "N2K.h"
#include "constants.h"
#include "Utils.h"
#include <time.h>

WiFiUDP udp;
N2K n2k;

uint g_valid_rmc = 0;
uint g_valid_gsa = 0;

uint ing_valid_rmc = 0;
uint ing_valid_gsa = 0;

time_t g_last_time_report = 0;

time_t g_last_time_send_system_time = 0;

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
  debug_print("Sending {%s}\n", bfr);
  udp.beginPacket(UDP_DEST, UDP_PORT);
  udp.write(bfr, l);
  udp.write(term, 2);
  udp.endPacket();
}

int parse_and_send(const char *sentence) {
    if (NMEAUtils::is_sentence(sentence, "RMC")) {
        if (NMEAUtils::parseRMC(sentence, g_rmc) == 0)
        {
            if (g_rmc.valid)
                g_valid_rmc++;
            else
                ing_valid_rmc++;
            debug_print("Process RMC {%s}\n", sentence);
            if (!g_time_set) {
              debug_print("Setting time to {%d-%d-%d %d:%d:%d}}\n", g_rmc.y, g_rmc.M, g_rmc.d, g_rmc.h, g_rmc.m, g_rmc.s);
              struct tm ts;
              ts.tm_hour = g_rmc.h;
              ts.tm_min = g_rmc.m;
              ts.tm_sec = g_rmc.s;
              ts.tm_year = g_rmc.y - 1900;
              ts.tm_mon = g_rmc.M - 1;
              ts.tm_mday = g_rmc.d;
              time_t t = mktime(&ts);
              struct timeval tv = {
                t, 0
              };
              if (settimeofday(&tv, NULL)==OK) {
                g_time_set = true;
              } else {
                debug_print("Failed to set time {%d}\n", errno);
              }
            }
            if (g_time_set) {
              n2k.sendTime(g_gsa, g_rmc);
              n2k.sendCOGSOG(g_gsa, g_rmc);
              n2k.sendPosition(g_gsa, g_rmc);
            }
            return OK;
        }
    } else if (NMEAUtils::is_sentence(sentence, "GSA")) {
        if (NMEAUtils::parseGSA(sentence, g_gsa) == 0)
        {
            if (g_gsa.valid)
                g_valid_gsa++;
            else
                ing_valid_gsa++;
            return OK;
        }
    }
    if (g_time_set) {
      time_t t = time(0);
      if ((t - g_last_time_report) > 30) {
          g_last_time_report = time(0);
          debug_print("RMC %d/%d GSA %d/%d Sats %d Fix %d Sent %d/%d\n",
                g_valid_rmc, ing_valid_rmc,
                g_valid_gsa, ing_valid_gsa,
                g_gsa.nSat, g_gsa.fix,
                n2k.get_sent(), n2k.get_sent_fail());
          g_valid_rmc = 0;
          ing_valid_rmc = 0;
          g_valid_gsa = 0;
          ing_valid_gsa = 0;
          n2k.reset_counters();
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
  
  if (!g_time_set) return;   

  int pgn = N2kMsg.PGN;
  int src = N2kMsg.Source;
  int dst = N2kMsg.Destination;
  int priority = N2kMsg.Priority;
  int dataLen = N2kMsg.DataLen;
  const unsigned char* data = N2kMsg.Data;
  unsigned long msgTime = N2kMsg.MsgTime;

  //time, priority, pgn, source, dest, len data
  //2011-11-24-22:42:04.388,2,127251,36,255,8,7d,0b,7d,02,00,ff,ff,ff
  time_t ts = msgTime / 1000;
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
  int l = strlen(buffer);

  debug_print("Sending out buffer {%s}\n", buffer);
  sendUDPPacket((uint8_t*)buffer, l);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  n2k.setup(handleMsg);
  initWiFi();
  initGPS();
  delay(2000);
  g_initialized = true;
}

void loop() {
  if (g_initialized) {
    readGPS(250000);
    n2k.loop();
  }
}
