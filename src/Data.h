#ifndef DATA_H
#define DATA_H

#include <math.h>
#include <cstring>
#include <stdint.h>

#define MAX_USED_SATS_SIZE 24
#define MAX_SATS_SIZE 256

const uint8_t STW_ERROR_OK = 0;
const uint8_t STW_ERROR_NO_SIGNAL = 1;
const uint8_t TEMP_ERROR_OK = 0;
const uint8_t TEMP_ERROR_NO_SIGNAL = 1;

class Configuration;

struct sat
{
  int sat_id;
  int elev;
  int az;
  int db;
  int used;
};

struct WaterData
{
  double speed = NAN;
  double frequency = NAN;
  double temperature = NAN;
  uint8_t speed_error = STW_ERROR_NO_SIGNAL;
  uint8_t temperature_error = TEMP_ERROR_NO_SIGNAL;
};

struct MeteoData
{
  double pressure = NAN;
  double humidity = NAN;
  double temperature = NAN;
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
  /** Revolutions per minute */
  uint16_t rpm = 0;
  /** Engine time in milliseconds */
  uint64_t engine_time = 0;
};

class GPSData
{
public:
  uint16_t serial = 0;

  unsigned char fix = 0;         // GNSS fix type (0=no fix, 1=dead reckoning, 2=2D, 3=3D, 4=GNSS, 5=Time fix)
  double latitude_signed = NAN;  // Signed latitude for NMEA
  double longitude_signed = NAN; // Signed longitude for NMEA
  double cog = NAN;              // Course over ground
  double sog = NAN;              // Speed over ground
  double hdop = NAN;             // Horizontal Dilution of Precision
  double pdop = NAN;             // Position Dilution of Precision
  double vdop = NAN;             // Vertical Dilution of Precision
  double tdop = NAN;             // Time Dilution of Precision

  uint32_t gps_unix_time = 0;    // in seconds since epoch
  uint16_t gps_unix_time_ms = 0; // the milliseconds part of the time (0-999)
  uint16_t year = 0;
  uint8_t month = 0;
  uint8_t day = 0;

  short nSat = 0;
  short nUsedSats = 0;
  sat satellites[MAX_SATS_SIZE];
  short sats[MAX_USED_SATS_SIZE];

  char get_longitude_cardinal() const
  {
    return longitude_signed > 0.0 ? 'E' : 'W';
  }

  char get_latitude_cardinal() const
  {
    return latitude_signed > 0.0 ? 'N' : 'S';
  }

  bool isValid() const
  {
    return fix > 0;
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

  WaterData water_data;

  double get_pressure(Configuration &conf);
  double get_humidity(Configuration &conf);
  double get_temperature(Configuration &conf);
  double get_temperature_el(Configuration &conf);
};

#endif