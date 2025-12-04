#ifndef DATA_H
#define DATA_H

#include <math.h>
#include <cstring>
#include <stdint.h>

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
  double temperature = NAN;
  double ttg = NAN;
};

struct EngineData
{
  uint16_t rpm = 0;
  uint64_t engine_time = 0;
};

class GPSData
{
public:
  uint16_t serial = 0;

  unsigned char fix = 0; // GNSS fix type (0=no fix, 1=dead reckoning, 2=2D, 3=3D, 4=GNSS, 5=Time fix)
  double latitude_signed = NAN; // Signed latitude for NMEA
  double longitude_signed = NAN; // Signed longitude for NMEA
  double cog = NAN; // Course over ground
  double sog = NAN; // Speed over ground
  double hdop = NAN; // Horizontal Dilution of Precision
  double pdop = NAN; // Position Dilution of Precision
  double vdop = NAN; // Vertical Dilution of Precision
  double tdop = NAN; // Time Dilution of Precision

  short nSat = 0;
  short nUsedSats = 0;
  sat satellites[MAX_SATS_SIZE];
  short sats[MAX_USED_SATS_SIZE];

  uint32_t gps_unix_time = 0; // in seconds since epoch
  uint16_t gps_unix_time_ms = 0; // the milliseconds part of the time (0-999)
  
  char get_longitude_cardinal()
  {
    return longitude_signed > 0.0 ? 'E' : 'W';
  }

  char get_latitude_cardinal()
  {
    return latitude_signed > 0.0 ? 'N' : 'S';
  }

  GSV get_GSV()
  {
    GSV gsv;
    gsv.nSat = nSat;
    memcpy(gsv.satellites, satellites, sizeof(satellites));
    return gsv;
  }

  GSA get_GSA()
  {
    GSA gsa;
    gsa.valid = fix > 0;
    gsa.nSat = nSat;
    gsa.fix = fix;
    memcpy(gsa.sats, sats, sizeof(sats));
    gsa.hdop = hdop;
    gsa.vdop = vdop;
    gsa.tdop = tdop;
    gsa.pdop = pdop;
    return gsa;
  }

  RMC get_RMC()
  {
    RMC rmc;
    rmc.cog = cog;
    rmc.sog = sog;
    rmc.fix = fix;
    rmc.lat = latitude_signed;
    rmc.lon = longitude_signed;
    rmc.unix_time = gps_unix_time;
    rmc.unix_time_ms = gps_unix_time_ms;
    rmc.valid = fix > 0;
    return rmc;
  }
};

class Data
{
public:
  GPSData gps;

  BatteryData battery_svc;
  BatteryData battery_eng;

  MeteoData meteo_0;
  MeteoData meteo_1;

  EngineData engine;

  double get_pressure(Configuration& conf);
  double get_humidity(Configuration& conf);
  double get_temperature(Configuration& conf);
  double get_temperature_el(Configuration& conf);
};

#endif