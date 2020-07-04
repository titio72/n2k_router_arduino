#ifndef _POSITION_FILTER_H
#define _POSITION_FILTER_H

#include "NMEA.h"

class PositionFilter {

public:
    PositionFilter();
    ~PositionFilter();

    RMC* sample(RMC* rmc);


private:
    RMC** _rolling_window;
    int _win_size;


};




#endif


