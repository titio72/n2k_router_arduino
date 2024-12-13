#include <Arduino.h>
#include <EEPROM.h>
#include "Conf.h"
#include "Log.h"

#define NO_CONF 0xFF
#define CONF_VERSION 0x03

#define E_SIZE 32
/*
0       Conf version
1-3     Services (GPS, BPM, DHT, TIME, STW, RPM, ...)
4-11    Engine time
12-15   RPM adjustement
16-31   Device name (X BLE)
*/

#define CONF_VERSION_START_BYTE 0
#define CONF_SERVICES_START_BYTE 1
#define CONF_N2K_SOURCE_START_BYTE 2
#define CONF_GPS_UART_SPEED_START_BYTE 3
#define CONF_ENGINE_TIME_START_BYTE 4
#define CONF_RPM_ADJUSTMENT_START_BYTE 12
#define CONF_DEVICE_NAME_START_BYTE 16

#define CONF_LOG_TAG "CONF"

Configuration::Configuration(): initialized(false), cache_rpm_adj(1.0)
{
    strcpy(cache_device_name, DEFAULT_DEVICE_NAME);
}

Configuration::~Configuration() {}

void Configuration::init()
{
    if (initialized) return;

    EEPROM.begin(E_SIZE);
    uint8_t conf_version = EEPROM.read(0);
    if (conf_version != CONF_VERSION)
    {
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
}

void Configuration::save_uart_speed(unsigned char s)
{
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
}
unsigned char Configuration::get_n2k_source()
{
    unsigned char src = EEPROM.readInt(CONF_N2K_SOURCE_START_BYTE);
    Log::tracex(CONF_LOG_TAG, "Read", "n2k src {%d}", src);
    return src;
}

void Configuration::save_n2k_source(unsigned char src)
{
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

N2KServices& Configuration::get_services()
{
    if  (!initialized)
    {
        uint8_t c = EEPROM.read(CONF_SERVICES_START_BYTE);
        cache_services.deserialize(c);
        Log::tracex(CONF_LOG_TAG, "Read", "gps {%d} bmp {%d} dht {%d} time {%d} tacho {%d} stw {%d} value {%x}",
                cache_services.use_gps, cache_services.use_bmp, cache_services.use_dht, cache_services.send_time, cache_services.use_tacho, cache_services.sog_2_stw, c);
    }
    return cache_services;
}

void Configuration::save_services(N2KServices& s)
{
    cache_services = s;
    uint8_t c = cache_services.serialize();
    EEPROM.write(CONF_SERVICES_START_BYTE, c);
    bool r = EEPROM.commit();
    Log::tracex(CONF_LOG_TAG, "Write", "gps {%d} bmp {%d} dht {%d} time {%d} tacho {%d} stw {%d} value {%x} success {%d}",
                cache_services.use_gps, cache_services.use_bmp, cache_services.use_dht, cache_services.send_time, cache_services.use_tacho, cache_services.sog_2_stw, c, r);
}

void N2KServices::deserialize(uint8_t v)
{
    use_gps = (v & 0x01);
    use_bmp = (v & 0x02);
    use_dht = (v & 0x04);
    send_time = (v & 0x08);
    sog_2_stw = (v & 0x10);
    use_tacho = (v & 0x20);
}

uint8_t N2KServices::serialize()
{
    return (use_gps ? 0x01 : 0) +
           (use_bmp ? 0x02 : 0) +
           (use_dht ? 0x04 : 0) +
           (send_time ? 0x08 : 0) +
           (sog_2_stw ? 0x10 : 0) +
           (use_tacho ? 0x20 : 0);
}

N2KServices& N2KServices::operator =(const N2KServices &svc)
{
    use_gps = svc.use_gps;
    use_bmp = svc.use_bmp;
    use_dht = svc.use_dht;
    send_time = svc.send_time;
    sog_2_stw = svc.sog_2_stw;
    use_tacho = svc.use_tacho;
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
    use_bmp = set_conf(BMP_ID, value, "BMP");
    send_time = set_conf(SYT_ID, value, "SYT");
    sog_2_stw = set_conf(STW_ID, value, "STW");
    use_tacho = set_conf(RPM_ID, value, "RPM");
    return true;
}

const char* N2KServices::to_string()
{
  buffer[MAX_CONF] = 0;
  buffer[GPS_ID] = use_gps ? '1' : '0';
  buffer[BMP_ID] = use_bmp ? '1' : '0';
  buffer[DHT_ID] = use_dht ? '1' : '0';
  buffer[SYT_ID] = send_time ? '1' : '0';
  buffer[STW_ID] = sog_2_stw ? '1' : '0';
  buffer[RPM_ID] = use_tacho ? '1' : '0';
  return buffer;
}


