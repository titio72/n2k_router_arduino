#include <Arduino.h>
#include <EEPROM.h>
#include "Conf.h"
#include "Log.h"

#define NO_CONF 0xFF
#define CONF_VERSION 0x00

void load_conf(Configuration& c, uint8_t *v)
{
    c.use_gps = (v[0] & 0x01);
    c.use_bmp = (v[0] & 0x02);
    c.use_dht = (v[0] & 0x04);
    c.send_time = (v[0] & 0x08);
    // skip 4th bit (unused)
    c.uart_speed = (v[0] & 0xE0) >> 5;  // use the last 3 bits [8 possible values]
    c.n2k_source = v[1];
}

Configuration::Configuration()
{
    use_gps = DEFAULT_USE_GPS;
    use_bmp = DEFAULT_USE_BMP;
    use_dht = DEFAULT_USE_DHT;
    send_time = DEFAULT_USE_TIME;
    uart_speed = DEFAULT_GPS_SPEED;
    simulator = DEFAULT_SIMULATOR;
    n2k_source = DEFAULT_N2K_SOURCE;
}

Configuration::~Configuration() {}

bool is_no_conf(uint8_t *v)
{
    return v[0]==NO_CONF && v[1]==NO_CONF;
}

bool get_conf(uint8_t *v)
{
    if (EEPROM.begin(3))
    {
        uint8_t conf_version = EEPROM.read(0);
        if (conf_version==CONF_VERSION)
        {
            // the configuration template is compatible
            v[0] = EEPROM.read(1);
            v[1] = EEPROM.read(2);
        }
        else
        {
            // the configuration template is not compatible
            v[0] = NO_CONF;
            v[1] = NO_CONF;
        }
        EEPROM.end();
        return true;
    }
    else
    {
        return false;
    }
}

bool put_conf(uint8_t* new_conf)
{
    if (EEPROM.begin(3))
    {
        EEPROM.write(0, CONF_VERSION);
        EEPROM.write(1, new_conf[0]);
        EEPROM.write(2, new_conf[1]);
        return EEPROM.commit();
    }
    else
    {
        return false;
    }
}

bool Configuration::load()
{
    uint8_t v[2];
    if (get_conf(v))
    {
        Log::trace("[CONF] Read value: {%#01x %#01x}\n", v[0], v[1]);
        if (is_no_conf(v))
        {
            Log::trace("[CONF] Use default gps {%d} bmp {%d} dht {%d} time {%d} uart_speed {%s} n2k_source {%d}\n", 
                use_gps, use_bmp, use_dht, send_time, UART_SPEED[uart_speed], n2k_source);
        }
        else
        {
            load_conf(*this, v);
            Log::trace("[CONF] Load gps {%d} bmp {%d} dht {%d} time {%d} uart_speed {%s} n2k_source {%d}\n",
                use_gps, use_bmp, use_dht, send_time, UART_SPEED[uart_speed], n2k_source);
        }
        return true;
    }
    else
    {
        return false;
    }
}

void serialize(Configuration& c, uint8_t *v)
{
    v[0] = ((c.use_gps ? 0x01 : 0) +
                   (c.use_bmp ? 0x02 : 0) +
                   (c.use_dht ? 0x04 : 0) +
                   (c.send_time ? 0x08 : 0) +
                   (0 << 4) + // skip 4th it
                   (c.uart_speed << 5)) & 0xFF;
    v[1] = c.n2k_source;
}

bool Configuration::save()
{
    uint8_t v[2];
    serialize(*this, v);
    Log::trace("[CONF] Writing new configuration {%#01x %#01x}\n", v[0], v[1]);

    if (!put_conf(v))
    {
        Log::trace("[CONF] Failed writing conf\n");
        return 0;
    }
    else
    {
        Log::trace("[CONF] Configuration writtten\n");
        return 1;
    }
}