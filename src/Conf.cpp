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

Configuration::Configuration() {}


Configuration::~Configuration() {}

int Configuration::load()
{
    uint8_t v = 0;
    #ifdef ESP32_ARCH
    EEPROM.begin(1);
    v = EEPROM.read(0);
    #else
    FILE* f = fopen("conf.txt", "r");
    if (f)
    {
        int _v = (char)fgetc(f);
        if (v!=EOF) v = _v;
    }
    #endif

    Log::trace("[CONF] Read value: {%#01x}\n", v);
    if (v != 0xFF)
    {
        use_gps = (v & 1);
        use_bmp280 = (v & 2);
        use_dht11 = (v & 4);
        send_time = (v & 8);
        dht11_dht22 = (v & 16) >> 4;
        uart_speed = (v & 0xE0) >> 5;
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
    int new_conf = ((use_gps ? 1 : 0) +
                   (use_bmp280 ? 2 : 0) +
                   (use_dht11 ? 4 : 0) +
                   (send_time ? 8 : 0) +
                   (dht11_dht22 << 4) +
                   (uart_speed << 5)) & 0xFF;
    Log::trace("[CONF] Writing new configuration {%d}\n", new_conf);

    #ifdef ESP32_ARCH
    EEPROM.write(0, (uint8_t)new_conf);
    if (!EEPROM.commit()) {
        Log::trace("[CONF] Failed writing conf\n");
    }
    #else
    FILE* f = fopen("conf.txt", "w");
    char xnf = (char)new_conf;
    fwrite(&xnf, sizeof(char), 1, f);
    fclose(f);
    #endif
    
    return 0;
}