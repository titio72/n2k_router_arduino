#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "N2K.h"

class Simulator {

public:
    bool sim_position           = true;
    bool sim_sogcog             = true;
    bool sim_wind               = true;
    bool sim_heading            = true;
    bool sim_speed              = false;
    bool sim_pressure           = true;
    bool sim_humidity           = true;
    bool sim_temperature        = true;
    bool sim_water_temperature  = true;
    bool sim_satellites         = true;
    bool sim_dops               = true;
    bool sim_attitude           = true;
    bool sim_depth              = true;
    bool sim_nav                = true;

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

    void loop(unsigned long ms, N2K* n2k);
};



#endif
