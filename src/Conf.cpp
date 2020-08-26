#include "Conf.h"
#ifdef ESP32_ARCH
#include <Arduino.h>
#include <EEPROM.h>
#endif
#include "Log.h"

Configuration::Configuration() {}


Configuration::~Configuration() {}

int Configuration::load()
{
#ifdef ESP32_ARCH
    EEPROM.begin(1);
    uint8_t v = EEPROM.read(0);
    Log::trace("[CONF] Read value: {%d}\n", v);
    if (v != 0xFF)
    {
        use_gps = (v & 1);
        use_bmp280 = (v & 2);
        use_dht11 = (v & 4);
        send_time = (v & 8);
        dht11_dht22 = (v & 16) >> 4;
        uart_speed = (v & 0xE0) >> 5;

        Log::trace("[CONF] Load gps {%d} bmp280 {%d} dht11 {%d} time {%d} dht {%d} uart {%d}\n", use_gps, use_bmp280, use_dht11, send_time, dht11_dht22, uart_speed);
    }
    else
    {
        Log::trace("[CONF] Use default conf: gps {%d} bmp280 {%d} dht11 {%d} time {%d} dht {%d} uart {%d\n", use_gps, use_bmp280, use_dht11, send_time, dht11_dht22, uart_speed);
    }
#endif
    return 0;
}

int Configuration::save()
{
#ifdef ESP32_ARCH
    int new_conf = ((use_gps ? 1 : 0) +
                   (use_bmp280 ? 2 : 0) +
                   (use_dht11 ? 4 : 0) +
                   (send_time ? 8 : 0) + 
                   (dht11_dht22 << 4) +
                   (uart_speed << 5)) & 0xFF;
    Log::trace("[CONF] Writing new configuration {%d}\n", (uint8_t)new_conf);
    EEPROM.write(0, (uint8_t)new_conf);
    if (!EEPROM.commit()) {
        Log::trace("[CONF] Failed writing conf\n");
    }
#endif
    return 0;
}