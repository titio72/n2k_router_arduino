#ifndef UTILS_H
#define UTILS_H

#ifdef ESP32_ARCH
#include <Arduino.h>
#endif

#include <stdlib.h>

int getDaysSince1970(int y, int m, int d);
char* replace(char const * const original, char const * const pattern, char const * const replacement, bool first = false);
bool array_contains(int test, int* int_set, int sz);
ulong _millis();
int msleep(long msec);

void format_thousands_sep(char* buffer, long l);

struct statistics {
  uint gps_fix = 0;
  uint valid_rmc = 0;
  uint valid_gsa = 0;
  uint valid_gsv = 0;
  uint invalid_rmc = 0;
  uint invalid_gsa = 0;
  uint invalid_gsv = 0;
  ulong udp_sent = 0;
  ulong udp_failed = 0;
  ulong can_sent = 0;
  ulong can_failed = 0;
  ulong can_received = 0;
  ulong bytes_uart = 0;
  ulong cycles = 0;
  ulong pauses = 0;
};

struct sat {
    int sat_id;
    int elev;
    int az;
    int db;
    int status;
};

struct GSA {
    short valid;
    int nSat;
    int fix;
    int sats[24];
    float hdop;
    float vdop;
    float pdop;
};

struct RMC {
    short valid;
    float lat;
    float lon;

    short y;
    short M;
    short d;
    short h;
    short m;
    short s;
    short ms;

    float cog;
    float sog;
};

struct data {
  RMC rmc;
  GSA gsa;
  double pressure;
  double humidity;
  double temperature;
  double temperature_el;
  double latitude;
  char latitude_NS = 'N';
  double longitude;
  char longitude_EW = 'E';
};

#endif
