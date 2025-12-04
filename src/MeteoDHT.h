#ifndef _MeteoDHT_H
#define _MeteoDHT_H

#include "Utils.h"
#include "Agents.hpp"

class DHTesp;

class MeteoDHT
{
public:
    typedef enum
    {
        DHT11 = 0,
        DHT22 = 2
    } DHT_MODEL;

    MeteoDHT(int pin, DHT_MODEL model, uint8_t meteo_index);
    ~MeteoDHT();

    AB_AGENT

private:
    bool enabled;
    DHTesp* dht;
    unsigned long last_read_time;
    int pin;
    DHT_MODEL model;
    uint8_t meteo_index;

    void read_temp_hum(unsigned long ms, MeteoData &data);
};



#endif