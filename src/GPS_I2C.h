#ifndef _GPSX_H
#define _GPSX_H

#include "Context.h"

class SFE_UBLOX_GNSS;

class GPSX
{
public:
    GPSX(Context _ctx);
    ~GPSX();

    AB_AGENT


    double getLatitude();
    double getLongitude();
    double getAltitude();

private:
    SFE_UBLOX_GNSS* myGNSS;
    bool enabled;
    unsigned long last_read_time;
    Context ctx;
};

#endif