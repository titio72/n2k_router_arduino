#ifndef _MeteoDHT_H
#define _MeteoDHT_H

#include "Utils.h"
#include "Context.h"
#include <DHTesp.h>

class MeteoDHT
{
public:
    typedef enum
    {
        DHT11 = 0,
        DHT22 = 2
    } DHT_MODEL;

    MeteoDHT(Context ctx, int pin, DHT_MODEL model, MeteoData& data);
    ~MeteoDHT();

    AB_AGENT

private:
    bool enabled;
    Context ctx;
    MeteoData& data;
    DHTesp dht;
    unsigned long last_read_time;
    int pin;
    DHT_MODEL model;

    void read_temp_hum(unsigned long ms);
};



#endif