#ifndef _MeteoDHT_H
#define _MeteoDHT_H

#include "Utils.h"
#include "Agents.hpp"

class DHTInternal
{
public:
    virtual bool setup() = 0;
    virtual void getTempAndHumidity(double &temp, double &humidity) = 0;
    virtual unsigned long getMinimumSamplingPeriod() = 0;
};

class MeteoDHT
{
public:
    typedef enum
    {
        DHT11 = 0,
        DHT22 = 2
    } DHT_MODEL;

    MeteoDHT(int pin, DHT_MODEL model, uint8_t meteo_index, DHTInternal *impl = nullptr);
    ~MeteoDHT();

    AB_AGENT

private:
    bool enabled;
    DHTInternal* dht;
    bool own;
    unsigned long last_read_time;
    int pin;
    DHT_MODEL model;
    uint8_t meteo_index;

    void read_temp_hum(unsigned long ms, MeteoData &data);
};



#endif