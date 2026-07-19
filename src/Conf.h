#ifndef _CONF_H
#define _CONF_H

#include "Constants.h"
#include <stdint.h>
#include <string>

#define CONF_VERSION 0x06

#define SVC_ACCESSOR_H(name)         \
    bool is_##name() const;          \
    void set_##name(bool v);

class N2KServices
{
public:
  N2KServices();

  void deserialize(uint16_t v);
  uint16_t serialize() const;

  bool from_string(const char *string);
  bool to_string(char *dest, size_t len) const;

  N2KServices &operator=(const N2KServices &svc);

  SVC_ACCESSOR_H(use_gps)
  SVC_ACCESSOR_H(use_dht)
  SVC_ACCESSOR_H(use_bme)
  SVC_ACCESSOR_H(send_time)
  SVC_ACCESSOR_H(use_tacho)
  SVC_ACCESSOR_H(sog_2_stw)
  SVC_ACCESSOR_H(use_vedirect)
  SVC_ACCESSOR_H(keep_n2k_src)
  SVC_ACCESSOR_H(use_tmp)
  SVC_ACCESSOR_H(use_stw_paddle)
  SVC_ACCESSOR_H(use_logger)

  uint8_t size() const;

  bool operator==(const N2KServices &other) const
  {
    return conf == other.conf;
  }

private:
  uint16_t conf;
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
  int16_t rpm_adjustment = DEFAULT_RPM_ADJUSTMENT * RPM_ADJUSTMENT_SCALE; // x100
  uint16_t battery_capacity_Ah = DEFAULT_BATTERY_CAPACITY; // in Ah
  char device_name[16];                                    // null-terminated
  N2KServices services;

  uint8_t sea_temp_alpha = DEFAULT_SEA_TEMP_ALPHA * SEA_TEMP_ALPHA_SCALE;
  uint16_t sea_temp_adjustment = DEFAULT_SEA_TEMP_ADJUSTMENT * SEA_TEMP_ADJUSTMENT_SCALE;
  uint8_t stw_paddle_alpha = DEFAULT_STW_PADDLE_ALPHA * STW_PADDLE_ALPHA_SCALE;
  uint16_t stw_paddle_adjustment = DEFAULT_STW_PADDLE_ADJUSTMENT * STW_PADDLE_ADJUSTMENT_SCALE;

  Conf()
  {
    device_name[0] = '\0';
  }
};

const int CONFIG_RES_ALREADY_INITIALIZED = -1;
const int CONFIG_RES_EEPROM_FAIL = -2;
const int CONFIG_RES_VERSION_MISMATCH = -3;
const int CONFIG_RES_OK = 0;

class ConfigurationPersistence
{
public:
  virtual bool init_persistence() = 0;
  virtual bool save_configuration(const Conf &conf) = 0;
  virtual bool load_configuration(Conf &conf) = 0;
};

class EngineHoursPersistence
{
public:
  virtual bool init_persistence() = 0;
  virtual bool save_engine_hours(uint64_t hours) = 0;
  virtual uint64_t load_engine_hours() = 0;
};

class Configuration
{
public:
  Configuration(ConfigurationPersistence *persistence = nullptr);
  virtual ~Configuration() {}

  int init();
  bool is_initialized() const { return initialized; }

  virtual double get_sea_temp_alpha() const;
  virtual double get_stw_paddle_alpha() const;
  virtual double get_sea_temp_adjustment() const;
  virtual double get_stw_paddle_adjustment() const;
  virtual double get_rpm_adjustment() const;
  virtual const char *get_device_name() const;
  virtual unsigned char get_n2k_source() const;
  virtual const N2KServices &get_services() const;
  virtual uint16_t get_batter_capacity() const;

  virtual MeteoSource get_pressure_source() const;
  virtual MeteoSource get_temperature_source() const;
  virtual MeteoSource get_temperature_el_source() const;
  virtual MeteoSource get_humidity_source() const;

  virtual bool save_sea_temp_alpha(double a);
  virtual bool save_stw_paddle_alpha(double a);
  virtual bool save_sea_temp_adjustment(double a);
  virtual bool save_stw_paddle_adjustment(double a);
  virtual bool save_rpm_adjustment(double d);
  virtual bool save_device_name(const char *name);
  virtual bool save_n2k_source(unsigned char src);
  virtual bool save_services(N2KServices &s);
  virtual bool save_battery_capacity(uint16_t c);

protected:
  Conf conf;
  bool initialized = false;
  ConfigurationPersistence* persistence;
};

class EngineHours
{
public:
  EngineHours(EngineHoursPersistence* persistence = nullptr);
  virtual ~EngineHours() {}

  int init();
  bool is_initialized() const { return initialized; }

  /**
   * Get the engine hours in milliseconds
   */
  virtual uint64_t get_engine_hours() const;

  /**
   * Save the engine hours in milliseconds
   */
  virtual bool save_engine_hours(uint64_t h);

protected:
  uint64_t engine_hours;
  bool initialized;
  EngineHoursPersistence* persistence;
};

#ifdef PIO_UNIT_TESTING
class MockEngineHours : public EngineHours
{
public:
  MockEngineHours();

  virtual bool save_engine_hours(uint64_t h) 
  { 
    save_engine_hours_calls++;
    return EngineHours::save_engine_hours(h);
  }

  int save_engine_hours_calls = 0;
};

class MockConfiguration : public Configuration
{
public:
  MockConfiguration();

  virtual bool save_rpm_adjustment(double d) override
  {
    save_rpm_adjustment_calls++;
    return Configuration::save_rpm_adjustment(d);
  }

  virtual bool save_device_name(const char *name) override
  {
    save_device_name_calls++;
    return Configuration::save_device_name(name);
  }

  virtual bool save_n2k_source(unsigned char src) override
  {
    save_n2k_source_calls++;
    return Configuration::save_n2k_source(src);
  }

  virtual bool save_services(N2KServices &s)
  {
    save_services_calls++;
    return Configuration::save_services(s);
  }

  virtual bool save_battery_capacity(uint16_t c)
  {
    save_battery_capacity_calls++;
    return Configuration::save_battery_capacity(c);
  }

  virtual bool save_sea_temp_alpha(double a) override
  {
    save_sea_temp_alpha_calls++;
    return Configuration::save_sea_temp_alpha(a);
  }

  virtual bool save_stw_paddle_alpha(double a) override
  {
    save_stw_paddle_alpha_calls++;
    return Configuration::save_stw_paddle_alpha(a);
  }

  virtual bool save_sea_temp_adjustment(double a) override
  {
    save_sea_temp_adjustment_calls++;
    return Configuration::save_sea_temp_adjustment(a);
  }

  virtual bool save_stw_paddle_adjustment(double a) override
  {
    save_stw_paddle_adjustment_calls++;
    return Configuration::save_stw_paddle_adjustment(a);
  }

  int save_rpm_adjustment_calls = 0;
  int save_device_name_calls = 0;
  int save_services_calls = 0;
  int save_n2k_source_calls = 0;
  int save_uart_speed_calls = 0;
  int save_battery_capacity_calls = 0;
  int save_sea_temp_alpha_calls = 0;
  int save_stw_paddle_alpha_calls = 0;
  int save_sea_temp_adjustment_calls = 0;
  int save_stw_paddle_adjustment_calls = 0;

  void reset_call_counts()
  {
    save_rpm_adjustment_calls = 0;
    save_device_name_calls = 0;
    save_services_calls = 0;
    save_n2k_source_calls = 0;
    save_uart_speed_calls = 0;
    save_battery_capacity_calls = 0;
    save_sea_temp_alpha_calls = 0;
    save_stw_paddle_alpha_calls = 0;
    save_sea_temp_adjustment_calls = 0;
    save_stw_paddle_adjustment_calls = 0;
  }
};
#endif
#endif