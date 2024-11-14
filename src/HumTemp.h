#ifndef _HumTemp_H
#define _HumTemp_H

#include <stdlib.h>
#include "Context.h"

class DHTesp;

class HumTemp 
{
public:
    HumTemp(const HumTemp& humtemp);
    HumTemp(Context ctx);
    ~HumTemp();

    AB_AGENT

private:
    bool enabled;
    Context ctx;
    DHTesp* DHT;
    unsigned long last_read_time;

    void read_temp_hum(unsigned long ms);
};



#endif