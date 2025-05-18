#include <Arduino.h>
#include <EEPROM.h>
#include <Log.h>
#include "Conf.h"


#define NO_CONF 0xFF
#define CONF_VERSION 0x04

#define E_SIZE 36
/*
0       Conf version
1-3     Services (GPS, BPM, DHT, TIME, STW, RPM, ...)
4-11    Engine time
12-15   RPM adjustement
16-31   Device name (X BLE)
32-35   Battery capacity
*/

#define CONF_VERSION_START_BYTE 0
#define CONF_SERVICES_START_BYTE 1
#define CONF_N2K_SOURCE_START_BYTE 2
#define CONF_GPS_UART_SPEED_START_BYTE 3
#define CONF_ENGINE_TIME_START_BYTE 4
#define CONF_RPM_ADJUSTMENT_START_BYTE 12
#define CONF_DEVICE_NAME_START_BYTE 16
#define CONF_BATTERY_CAPACITY_START_BYTE 32

#define DEFAULT_BATTERY_CAPACITY 280

#define CONF_LOG_TAG "CONF"

Configuration::Configuration(): initialized(false), cache_rpm_adj(1.0), cache_batt_cap(0xFFFFFFFF), n2k_source(NO_CONF)
{
    strcpy(cache_device_name, DEFAULT_DEVICE_NAME);
}

Configuration::~Configuration() {}

void Configuration::init()
{
    if (initialized) return;

    if (!EEPROM.begin(E_SIZE))
    {
        Log::tracex(CONF_LOG_TAG, "Init", "Failed to init EEPROM");
    }

    uint8_t conf_version = EEPROM.read(0);
    if (conf_version != CONF_VERSION)
    {
        Log::tracex(CONF_LOG_TAG, "Init", "Conf version check failed - start with defaults");
        EEPROM.write(CONF_VERSION_START_BYTE, CONF_VERSION);
        save_services(cache_services);
        save_device_name(cache_device_name);
        save_engine_hours(0);
        save_rpm_adjustment(1.0);
    }
    else
    {
        // force caching
        get_device_name();
        get_services();
        get_rpm_adjustment();
    }
    initialized = true;
}

MeteoSource Configuration::get_pressure_source()
{
    if (cache_services.use_bme)
    {
        return METEO_BME;
    }
    else
    {
        return METEO_NONE;
    }
}

MeteoSource Configuration::get_temperature_source()
{
    if (cache_services.use_dht)
    {
        return METEO_DHT;
    }
    else if (cache_services.use_bme)
    {
        return METEO_BME;
    }
    else
    {
        return METEO_NONE;
    }
}

MeteoSource Configuration::get_temperature_el_source()
{
    if (cache_services.use_bme && cache_services.use_dht)
    {
        return METEO_BME;
    }
    else
    {
        return METEO_NONE;
    }
}

MeteoSource Configuration::get_humidity_source()
{
    if (cache_services.use_dht)
    {
        return METEO_DHT;
    }
    else if (cache_services.use_bme)
    {
        return METEO_BME;
    }
    else
    {
        return METEO_NONE;
    }
}

uint64_t Configuration::get_engine_hours()
{
    uint64_t hh = EEPROM.readULong64(CONF_ENGINE_TIME_START_BYTE);
    Log::tracex(CONF_LOG_TAG, "Read", "engine time {%lu-%03d}", (uint32_t)(hh / 1000), (uint16_t)hh % 1000);
    return hh;
}

void Configuration::save_engine_hours(uint64_t h)
{
    bool r = EEPROM.writeULong64(CONF_ENGINE_TIME_START_BYTE, h);
    r = r && EEPROM.commit();
    Log::debugx(CONF_LOG_TAG, "Write", "engine time {%lu-%d %lu} success {%d}", (uint32_t)(h / 1000), (uint16_t)(h % 1000), r);
}

unsigned char Configuration::get_uart_speed()
{
    return DEFAULT_GPS_SPEED;
    /*
    unsigned char s = EEPROM.readInt(CONF_GPS_UART_SPEED_START_BYTE);
    if (s<UART_SPEEDS)
    {
        Log::tracex(CONF_LOG_TAG, "Read", "uart {%d %d}", s, UART_SPEED[s]);
    }
    else
    {
        Log::tracex(CONF_LOG_TAG, "Read error", "illegal uart speed {%d}", s);
        s = DEFAULT_GPS_SPEED;
    }
    return s;
    */
}

void Configuration::save_uart_speed(unsigned char s)
{
    /*
    if (s<UART_SPEEDS)
    {
        bool r = EEPROM.writeInt(CONF_GPS_UART_SPEED_START_BYTE, s);
        r = r && EEPROM.commit();
        Log::tracex(CONF_LOG_TAG, "Write", "uart {%d %d} success {%d}", s, UART_SPEED[s], r);
    }
    else
    {
        Log::tracex(CONF_LOG_TAG, "Write error", "illegal uart speed {%d}", s);
    }
    */
}

unsigned char Configuration::get_n2k_source()
{
    if (n2k_source == NO_CONF)
    {
        n2k_source = EEPROM.readInt(CONF_N2K_SOURCE_START_BYTE);
        Log::tracex(CONF_LOG_TAG, "Read", "n2k src {%d}", n2k_source);
        if (n2k_source == NO_CONF)
        {
            n2k_source = DEFAULT_N2K_SOURCE;
        }
    }
    return n2k_source;
}

void Configuration::save_n2k_source(unsigned char src)
{
    n2k_source = src;
    bool r = EEPROM.writeInt(CONF_N2K_SOURCE_START_BYTE, src);
    r = r &&  EEPROM.commit();
    Log::tracex(CONF_LOG_TAG, "Write", "n2k src {%d} success {%d}", src, r);
}

double Configuration::get_rpm_adjustment()
{
    if (!initialized)
    {
        int32_t i_adj = EEPROM.readInt(CONF_RPM_ADJUSTMENT_START_BYTE);
        cache_rpm_adj = i_adj / 10000.0;
        Log::tracex(CONF_LOG_TAG, "Read", "rpm adjustment {%.4f}", cache_rpm_adj);
    }
    return cache_rpm_adj;
}

void Configuration::save_rpm_adjustment(double d)
{
    cache_rpm_adj = d;
    int i_adj = (int)(d * 10000.0);
    bool r = EEPROM.writeInt(CONF_RPM_ADJUSTMENT_START_BYTE, i_adj);
    r = r && EEPROM.commit();
    Log::tracex(CONF_LOG_TAG, "Write", "rpm adjustment {%.4f} success {%d}", d, r);
}

const char *Configuration::get_device_name()
{
    if (!initialized)
    {
        EEPROM.readString(CONF_DEVICE_NAME_START_BYTE, cache_device_name, 15);
        Log::tracex(CONF_LOG_TAG, "Read", "device name {%s}", cache_device_name);
    }
    return cache_device_name;
}

void Configuration::save_device_name(const char *name)
{
    if (strlen(name) <= 15)
    {
        strcpy(cache_device_name, name);
        bool r = EEPROM.writeString(CONF_DEVICE_NAME_START_BYTE, cache_device_name);
        r = r && EEPROM.commit();
        Log::tracex(CONF_LOG_TAG, "Write", "device name {%s} success {%d}", cache_device_name, r);
    }
    else
    {
        Log::tracex(CONF_LOG_TAG, "Write error", "invald device name {%s}", name);
    }
}

uint32_t Configuration::get_batter_capacity()
{
    if (!initialized)
    {
        uint32_t c = EEPROM.readUInt(CONF_BATTERY_CAPACITY_START_BYTE);
        cache_batt_cap = c;
        Log::tracex(CONF_LOG_TAG, "Read", "battery capacity {%d}", c);
    }
    return DEFAULT_BATTERY_CAPACITY;
}

void Configuration::save_batter_capacity(uint32_t c)
{
    cache_batt_cap = c;
    bool r = EEPROM.writeUInt(CONF_BATTERY_CAPACITY_START_BYTE, c);
    r = r && EEPROM.commit();
    Log::tracex(CONF_LOG_TAG, "Write", "battery capacity {%d} success {%d}", c, r);
}

N2KServices& Configuration::get_services()
{
    if  (!initialized)
    {
        uint8_t c = EEPROM.read(CONF_SERVICES_START_BYTE);
        cache_services.deserialize(c);
        Log::tracex(CONF_LOG_TAG, "Read", "gps {%d} bme {%d} dht {%d} time {%d} tacho {%d} stw {%d} ved {%d} value {%x}",
                cache_services.use_gps, cache_services.use_bme, cache_services.use_dht, cache_services.send_time,
                cache_services.use_tacho, cache_services.sog_2_stw, cache_services.use_vedirect, c);
    }
    return cache_services;
}

void Configuration::save_services(N2KServices& s)
{
    cache_services = s;
    uint8_t c = cache_services.serialize();
    EEPROM.write(CONF_SERVICES_START_BYTE, c);
    bool r = EEPROM.commit();
    Log::tracex(CONF_LOG_TAG, "Write", "gps {%d} bme {%d} dht {%d} time {%d} tacho {%d} stw {%d} ved {%d} value {%x} success {%d}",
                cache_services.use_gps, cache_services.use_bme, cache_services.use_dht, cache_services.send_time,
                cache_services.use_tacho, cache_services.sog_2_stw, cache_services.use_vedirect, c, r);
}

void N2KServices::deserialize(uint8_t v)
{
    use_gps = (v & 0x01);
    use_bme = (v & 0x02);
    use_dht = (v & 0x04);
    send_time = (v & 0x08);
    sog_2_stw = (v & 0x10);
    use_tacho = (v & 0x20);
    use_vedirect = (v & 0x40);
}

uint8_t N2KServices::serialize()
{
    return (use_gps ? 0x01 : 0) +
           (use_bme ? 0x02 : 0) +
           (use_dht ? 0x04 : 0) +
           (send_time ? 0x08 : 0) +
           (sog_2_stw ? 0x10 : 0) +
           (use_tacho ? 0x20 : 0) +
           (use_vedirect ? 0x40 : 0);
}

N2KServices& N2KServices::operator =(const N2KServices &svc)
{
    use_gps = svc.use_gps;
    use_bme = svc.use_bme;
    use_dht = svc.use_dht;
    send_time = svc.send_time;
    sog_2_stw = svc.sog_2_stw;
    use_tacho = svc.use_tacho;
    use_vedirect = svc.use_vedirect;
    return *this;
}

bool set_conf(int i, const char *value, const char *descr)
{
  if (strlen(value) > i)
  {
    boolean res = (value[i] == '1');
    Log::tracex(CONF_LOG_TAG, "Parse", "Conf {%s} ConfId {%d} Enabled {%d}", descr, i, res);
    return res;
  }
  else
  {
    Log::tracex(CONF_LOG_TAG, "Parse Error", "Conf {%s} Error {%d}", descr, i);
  }
  return false;
}

bool N2KServices::from_string(const char* value)
{
    use_gps = set_conf(GPS_ID, value, "GPS");
    use_dht = set_conf(DHT_ID, value, "DHT");
    use_bme = set_conf(BME_ID, value, "BME");
    send_time = set_conf(SYT_ID, value, "SYT");
    sog_2_stw = set_conf(STW_ID, value, "STW");
    use_tacho = set_conf(RPM_ID, value, "RPM");
    use_vedirect = set_conf(VED_ID, value, "VE Direct");
    return true;
}

const char* N2KServices::to_string()
{
  buffer[MAX_CONF] = 0;
  buffer[GPS_ID] = use_gps ? '1' : '0';
  buffer[DHT_ID] = use_dht ? '1' : '0';
  buffer[SYT_ID] = send_time ? '1' : '0';
  buffer[STW_ID] = sog_2_stw ? '1' : '0';
  buffer[RPM_ID] = use_tacho ? '1' : '0';
  buffer[VED_ID] = use_vedirect ? '1' : '0';
  buffer[BME_ID] = use_bme ? '1' : '0';
  return buffer;
}


