#include <Arduino.h>
#include <EEPROM.h>
#include "Conf.h"
#include "Log.h"

#define NO_CONF 0xFF
#define CONF_VERSION 0x03

#define E_SIZE 16

void load_conf(Configuration& c, uint8_t *v)
{
    if (v[0]==NO_CONF)
    {
        c.use_gps = DEFAULT_USE_GPS;
        c.use_bmp = DEFAULT_USE_BMP;
        c.use_dht = DEFAULT_USE_DHT;
        c.send_time = DEFAULT_USE_TIME;
        c.sog_2_stw = DEFAULT_SOG_2_STW;
        c.use_tacho = DEFAULT_USE_TACHO;
    }
    else
    {
        c.use_gps = (v[0] & 0x01);
        c.use_bmp = (v[0] & 0x02);
        c.use_dht = (v[0] & 0x04);
        c.send_time = (v[0] & 0x08);
        c.sog_2_stw = (v[0] & 0x10);
        c.use_tacho = (v[0] & 0x20);
    }
    c.n2k_source = (v[1]==NO_CONF)?DEFAULT_N2K_SOURCE:v[1];
    c.rpm_calibration = 255.0/v[2];
    c.uart_speed = DEFAULT_GPS_SPEED;
}

Configuration::Configuration()
{
    use_gps = DEFAULT_USE_GPS;
    use_bmp = DEFAULT_USE_BMP;
    use_dht = DEFAULT_USE_DHT;
    send_time = DEFAULT_USE_TIME;
    use_tacho = DEFAULT_USE_TACHO;
    rpm_calibration = DEFAULT_RPM_CALIBRATION;
    simulator = DEFAULT_SIMULATOR;
    n2k_source = DEFAULT_N2K_SOURCE;
    uart_speed = DEFAULT_GPS_SPEED;
}

Configuration::~Configuration() {}

bool get_conf(uint8_t *v)
{
    uint8_t conf_version = EEPROM.read(0);
    if (conf_version==CONF_VERSION)
    {
        // the configuration template is compatible
        v[0] = EEPROM.read(1);
        v[1] = EEPROM.read(2);
        v[2] = EEPROM.read(3);
    }
    else
    {
        // the configuration template is not compatible
        v[0] = NO_CONF;
        v[1] = NO_CONF;
        v[2] = NO_CONF;
    }
    return true;
}

bool put_conf(uint8_t* new_conf)
{
    EEPROM.write(0, CONF_VERSION);
    EEPROM.write(1, new_conf[0]);
    EEPROM.write(2, new_conf[1]);
    EEPROM.write(3, new_conf[2]);
    return EEPROM.commit();
}

bool Configuration::load()
{
    EEPROM.begin(E_SIZE);
    uint8_t v[3];
    get_conf(v);
    Log::tracex("CONF", "Read", "Value {%#01x %#01x %#01x}", v[0], v[1], v[2]);
    load_conf(*this, v);
    Log::tracex("CONF", "Load", "gps {%d} bmp {%d} dht {%d} time {%d} tacho {%d} stw {%d} rpm {%.2f} n2k_source {%d}",
        use_gps, use_bmp, use_dht, send_time, use_tacho, sog_2_stw, rpm_calibration, n2k_source);
    return true;
}

void serialize(Configuration& c, uint8_t *v)
{
    v[0] = (c.use_gps ? 0x01 : 0) +
                   (c.use_bmp ? 0x02 : 0) +
                   (c.use_dht ? 0x04 : 0) +
                   (c.send_time ? 0x08 : 0) +
                   (c.sog_2_stw ? 0x10 : 0) +
                   (c.use_tacho ? 0x20 : 0);
    v[1] = c.n2k_source;
    v[2] = (int)(255.0 * c.rpm_calibration + 0.5);
}

bool Configuration::save()
{
    uint8_t v[3];
    serialize(*this, v);
    Log::tracex("CONF", "Write", "Value {%#01x %#01x %#01x}", v[0], v[1], v[2]);

    if (!put_conf(v))
    {
        Log::tracex("CONF", "Write", "Failed writing configuration");
        return 0;
    }
    else
    {
        Log::tracex("CONF", "Write", "Configuration writtten");
        return 1;
    }
}

uint64_t Configuration::get_engine_hours()
{
    uint64_t hh = EEPROM.readULong64(4);
    Log::tracex("CONF", "Read", "engine time {%lu-%03d}", (uint32_t)(hh/1000), (uint16_t)hh%1000);
    return hh;
}

void Configuration::save_engine_hours(uint64_t h)
{
    unsigned long t0 = micros();
    EEPROM.writeULong64(4, h);
    bool r = EEPROM.commit();
    unsigned long t1 = micros();
    Log::debugx("CONF", "Write", "engine time {%lu-%d %lu %d}", (uint32_t)(h/1000), (uint16_t)(h%1000), t1-t0, r);
}

double Configuration::get_rpm_adjustment()
{
    int i_adj = EEPROM.readInt(12);
    double adj = i_adj / 10000.0;
    Log::tracex("CONF", "Read", "rpm adjustment {%.4f}", adj);
    return adj;
}

void Configuration::save_rpm_adjustment(double d)
{
    int i_adj = (int)(d * 10000.0);
    bool r = EEPROM.writeInt(12, i_adj);
    Log::tracex("CONF", "Write", "rpm adjustment {%.4f}", d);
}