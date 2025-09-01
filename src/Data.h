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

    unsigned short fix = 0;

    double lat = NAN;
    double lon = NAN;

    /*uint16_t y = 0;
    uint8_t M = 0;
    uint8_t d = 0;
    uint8_t h = 0;
    uint8_t m = 0;
    uint8_t s = 0;
    uint16_t ms = 0;*/

    double cog = NAN;
    double sog = NAN;

    uint32_t unix_time = 0; // in seconds since epoch
    uint16_t unix_time_ms = 0; //  the milliseconds part of the time (0-999)
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

  double latitude_signed = NAN; // Signed latitude for NMEA
  double longitude_signed = NAN; // Signed longitude for NMEA
  double cog = NAN; // Course over ground
  double sog = NAN; // Speed over ground
  double hdop = NAN; // Horizontal Dilution of Precision
  double pdop = NAN; // Position Dilution of Precision
  double vdop = NAN; // Vertical Dilution of Precision
  double tdop = NAN; // Time Dilution of Precision
  unsigned char fix = 0; // GNSS fix type (0=no fix, 1=dead reckoning, 2=2D, 3=3D, 4=GNSS, 5=Time fix)
  double latitude = NAN;  char latitude_NS = 'N';
  double longitude = NAN; char longitude_EW = 'E';
  uint32_t gps_unix_time = 0; // in seconds since epoch
  uint32_t gps_unix_time_ms = 0; // the milliseconds part of the time (0-999)

  BatteryData battery_svc;
  BatteryData battery_eng;

  EngineData engine;

  double get_pressure(Configuration& conf);
  double get_humidity(Configuration& conf);
  double get_temperature(Configuration& conf);
  double get_temperature_el(Configuration& conf);
};

#endif