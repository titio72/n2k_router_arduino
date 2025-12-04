#ifndef _CONF_H
#define _CONF_H

#include "Constants.h"
#include <stdint.h>

#define CONF_VERSION 0x05

class N2KServices
{
public:
  N2KServices();

  void deserialize(uint8_t v);
  uint8_t serialize() const;

  bool from_string(const char* string);
  bool to_string(char* dest, size_t len) const;

  N2KServices& operator =(const N2KServices &svc);

  bool is_use_gps() const;
  bool is_use_bme() const;
  bool is_use_dht() const;
  bool is_send_time() const;
  bool is_sog_2_stw() const;
  bool is_use_tacho() const;
  bool is_use_vedirect() const;

  void set_use_gps(bool v);
  void set_use_bme(bool v);
  void set_use_dht(bool v);
  void set_send_time(bool v);
  void set_sog_2_stw(bool v);
  void set_use_tacho(bool v);
  void set_use_vedirect(bool v);

  uint8_t size() const;

private:
    uint8_t conf;
};

enum MeteoSource
{
  METEO_BME = 0,
  METEO_DHT = 1,
  METEO_NONE = 2
};

struct Conf
{
    uint8_t conf_version = CONF_VERSION;
    
    uint8_t n2k_source = DEFAULT_N2K_SOURCE;
    int32_t rpm_adjustment = 1000;                          // x1000
    uint16_t battery_capacity_Ah = DEFAULT_BATTERY_CAPACITY; // in Ah
    char device_name[16];                                    // null-terminated
    N2KServices services;

    Conf()
    {
        device_name[0] = '\0';
    }
};

const int CONFIG_RES_ALREADY_INITIALIZED = -1;
const int CONFIG_RES_EEPROM_FAIL = -2;
const int CONFIG_RES_VERSION_MISMATCH = -3;
const int CONFIG_RES_OK = 0;

class Configuration
{
public:
  virtual int is_initialized() const = 0;


  virtual double get_rpm_adjustment() const = 0;
  virtual const char* get_device_name() const = 0;
  virtual unsigned char get_n2k_source() const = 0;
  virtual unsigned char get_uart_speed() const = 0;
  virtual const N2KServices& get_services() const = 0;
  virtual uint16_t get_batter_capacity() const = 0;
  virtual MeteoSource get_pressure_source() const = 0;
  virtual MeteoSource get_temperature_source() const = 0;
  virtual MeteoSource get_temperature_el_source() const = 0;
  virtual MeteoSource get_humidity_source() const = 0;
};

class EngineHours
{
public:
  virtual uint64_t get_engine_hours() const = 0;
  virtual bool save_engine_hours(uint64_t h) = 0;
};

class ConfigurationRW: public Configuration, public EngineHours {

public:
  ConfigurationRW();
  virtual ~ConfigurationRW();

  int init();

  int is_initialized() const { return initialized; }

  virtual uint64_t get_engine_hours() const;
  virtual bool save_engine_hours(uint64_t h);

  virtual double get_rpm_adjustment() const;
  uint32_t get_raw_rpm_adjustment() const { return conf.rpm_adjustment; }
  bool save_rpm_adjustment(double d);

  virtual const char* get_device_name() const;
  bool save_device_name(const char* name);

  virtual unsigned char get_n2k_source() const;
  bool save_n2k_source(unsigned char src);

  virtual unsigned char get_uart_speed() const;
  bool save_uart_speed(unsigned char speed);

  virtual const N2KServices& get_services() const;
  bool save_services(N2KServices& s);

  virtual uint16_t get_batter_capacity() const;
  bool save_batter_capacity(uint16_t c);

  virtual MeteoSource get_pressure_source() const;
  virtual MeteoSource get_temperature_source() const;
  virtual MeteoSource get_temperature_el_source() const;
  virtual MeteoSource get_humidity_source() const;

private:

  Conf conf;
  bool initialized;

  bool save();
};

class MockConfiguration: public Configuration
{
public:
  int init() { return initialized; }

  int is_initialized() const { return initialized; }


  virtual double get_rpm_adjustment() const { return rpm_adjustment; }
  virtual const char* get_device_name() const { return ble_name; }
  virtual unsigned char get_n2k_source() const { return n2k_src; }
  virtual unsigned char get_uart_speed() const { return 2; }
  virtual const N2KServices& get_services() const { return services; }
  virtual uint16_t get_batter_capacity() const { return battery_capacity; }
  virtual MeteoSource get_pressure_source() const { return pressure_source; }
  virtual MeteoSource get_temperature_source() const { return temperature_source; }
  virtual MeteoSource get_temperature_el_source() const { return temperature_el_source; }
  virtual MeteoSource get_humidity_source() const { return humidity_source; }

  bool initialized = true;

  double rpm_adjustment = 1.0;
  const char* ble_name = "TEST";
  uint8_t n2k_src = 11;
  N2KServices services;
  uint16_t battery_capacity = 280;

  MeteoSource pressure_source = MeteoSource::METEO_BME;
  MeteoSource temperature_source = MeteoSource::METEO_BME;
  MeteoSource humidity_source = MeteoSource::METEO_BME;
  MeteoSource temperature_el_source = MeteoSource::METEO_NONE;
};

#endif