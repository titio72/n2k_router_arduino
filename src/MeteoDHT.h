#ifndef _MeteoDHT_H
#define _MeteoDHT_H

#include "Utils.h"
#include "Context.h"

class DHTesp;

class MeteoDHT 
{
public:
    MeteoDHT(const MeteoDHT& humtemp);
    MeteoDHT(Context ctx);
    ~MeteoDHT();

    AB_AGENT

private:
    bool enabled;
    Context ctx;
    DHTesp* DHT;
    unsigned long last_read_time;

    void read_temp_hum(unsigned long ms);
};



#endif