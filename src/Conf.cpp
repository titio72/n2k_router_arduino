#ifdef ESP32_ARCH
#include <Arduino.h>
#include <EEPROM.h>
#else
#include <cstdio>
#include <cstdint>
#endif
#include "Constants.h"
#include "Conf.h"
#include "Log.h"


uint8_t get_default_conf()
{
    return ((DEFAULT_USE_GPS ? 1 : 0) +
            (DEFAULT_USE_BMP ? 2 : 0) +
            (DEFAULT_USE_DHT ? 4 : 0) +
            (DEFAULT_USE_TIME ? 8 : 0) +
            (DEFAULT_DHT << 4) +
            (DEFAULT_GPS_SPEED << 5)) & 0xFF;
}

#define DEFAULT_CONF get_default_conf()

void load_conf(Configuration& c, uint8_t v)
{
    c.use_gps = (v & 1);
    c.use_bmp280 = (v & 2);
    c.use_dht11 = (v & 4);
    c.send_time = (v & 8);
    c.dht11_dht22 = (v & 16) >> 4;
    c.uart_speed = (v & 0xE0) >> 5;
}

Configuration::Configuration()
{
    use_gps = DEFAULT_USE_GPS;
    use_bmp280 = DEFAULT_USE_BMP;
    use_dht11 = DEFAULT_USE_DHT;
    send_time = DEFAULT_USE_TIME;
    dht11_dht22 = DEFAULT_DHT;
    uart_speed = DEFAULT_GPS_SPEED;
    simulator = DEFAULT_SIMULATOR;
    network = DEFAULT_USE_NETWORK;
}

Configuration::~Configuration() {}

#ifdef ESP32_ARCH
int get_conf()
{
    uint8_t v = DEFAULT_CONF;
    EEPROM.begin(1);
    uint8_t _v = EEPROM.read(0);
    if (_v==0xFF) v = _v;
    return v;
}

bool put_conf(uint8_t new_conf)
{
    EEPROM.write(0, (uint8_t)new_conf);
    return EEPROM.commit();
}

#else
int get_conf()
{
    uint8_t v = DEFAULT_CONF;
    FILE* f = fopen("conf.txt", "r");
    if (f)
    {
        int _v = (char)fgetc(f);
        if (_v!=EOF) v = _v;
        fclose(f);
    }
    return v;
}

bool put_conf(uint8_t new_conf)
{
    FILE* f = fopen("conf.txt", "w");
    if (f)
    {
        char xnf = (char)new_conf;
        size_t s = fwrite(&xnf, sizeof(char), 1, f);
        fclose(f);
        return s>0;
    }
    return false;
}
#endif

int Configuration::load()
{
    uint8_t v = get_conf();
    Log::trace("[CONF] Read value: {%#01x}\n", v);
    if (v != 0xFF)
    {
        load_conf(*this, v);
        Log::trace("[CONF] Load gps {%d} bmp280 {%d} dht {%d} time {%d} dht_type {%s} uart_speed {%s}\n", 
            use_gps, use_bmp280, use_dht11, send_time, DHTxx[dht11_dht22], UART_SPEED[uart_speed]);
    }
    else
    {
        Log::trace("[CONF] Use default conf: gps {%d} bmp280 {%d} dht {%d} time {%d} dht_type {%s} uart_speed {%s}\n", 
            use_gps, use_bmp280, use_dht11, send_time, DHTxx[dht11_dht22], UART_SPEED[uart_speed]);
    }
    return 0;
}

int Configuration::save()
{
    uint8_t new_conf = ((use_gps ? 1 : 0) +
                   (use_bmp280 ? 2 : 0) +
                   (use_dht11 ? 4 : 0) +
                   (send_time ? 8 : 0) +
                   (dht11_dht22 << 4) +
                   (uart_speed << 5)) & 0xFF;
    Log::trace("[CONF] Writing new configuration {%d}\n", new_conf);

    if (!put_conf((uint8_t)new_conf))
    {
        Log::trace("[CONF] Failed writing conf\n");
    }
    else
    {
        Log::trace("[CONF] Confifuration writtten\n");
    }
}