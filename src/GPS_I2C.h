#ifndef _GPSX_H
#define _GPSX_H

#include "Context.h"
#if GPS_TYPE==1
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

    void dumpStats();

private:
    void manageLowFrequency(unsigned long tstamp);
    void manageHighFrequency(unsigned long tstamp);
    bool set_system_time(unsigned char sid);
    Context ctx;
    bool enabled;
    unsigned long last_read_time;
    time_t delta_time;
    bool gps_time_set;
};
#endif //_GPS_TYPE
#endif