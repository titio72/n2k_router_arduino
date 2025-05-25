#ifndef DATA_H
#define DATA_H

#include <Arduino.h>

#define MAX_USED_SATS_SIZE 24
#define MAX_SATS_SIZE 255

class Configuration;

struct sat {
    int sat_id;
    int elev;
    int az;
    int db;
    int used;
};

struct GSA {
    short valid =0;
    short nSat = 0;
    unsigned char fix = 0;
    short sats[MAX_USED_SATS_SIZE];
    double hdop = NAN;
    double vdop = NAN;
    double tdop = NAN;
    double pdop = NAN;
};

struct RMC {
    short valid = 0;
    double lat = NAN;
    double lon = NAN;

    short y = 0;
    short M = 0;
    short d = 0;
    short h = 0;
    short m = 0;
    short s = 0;
    short ms = 0;

    double cog = NAN;
    double sog = NAN;

    time_t unix_time = 0;
};

struct GSV {
  short nSat = 0;
  sat satellites[MAX_SATS_SIZE];
};

struct MeteoData
{
  double pressure = NAN;
  double humidity = NAN;
  double temperature = NAN;
  double temperature_el = NAN;
};

struct BatteryData
{
  double voltage = NAN;
  double current = NAN;
  double soc = NAN;
};

struct EngineData
{
  uint16_t rpm = 0;
  uint64_t engine_time = 0;
};

class Data
{
public:
  RMC rmc;
  GSA gsa;
  GSV gsv;

  MeteoData meteo_0;
  MeteoData meteo_1;

  double latitude = NAN;
  char latitude_NS = 'N';
  double longitude = NAN;
  char longitude_EW = 'E';

  BatteryData battery;

  EngineData engine;

  double get_pressure(Configuration& conf);
  double get_humidity(Configuration& conf);
  double get_temperature(Configuration& conf);
  double get_temperature_el(Configuration& conf);
};

#endif