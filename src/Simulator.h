#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "Context.h"

class Simulator {

public:
    double heading = 210;
    double speed = 6;
    double wind_speed = 8;
    double wind_direction = 100;
    double lat = 43.9599;
    double lon = 09.7745;
    double dest_lat = 43.052173;
    double dest_lon = 9.840522;

    unsigned long t0_1000;
    unsigned long t0_0250;
    unsigned long t0_0100;
    unsigned long t0;

    AB_AGENT

    Simulator(Context ctx);
    ~Simulator();
private:
    Context ctx;
    bool enabled;
};



#endif
