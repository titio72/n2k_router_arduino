#ifndef _HumTemp_H
#define _HumTemp_H

#include <stdlib.h>
#include "Context.h"

class DHTesp;

class HumTemp 
{
public:
    HumTemp(Context ctx);
    ~HumTemp();

    AB_AGENT

private:
    bool enabled;
    Context ctx;
    DHTesp* DHT;

    void read_temp_hum(unsigned long ms);
};



#endif