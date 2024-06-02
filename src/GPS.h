#ifndef _GPS_H
#define _GPS_H

#include "Ports.h"
#include "Context.h"

class GPS_stats
{
public:
    unsigned long invalid_gsa = 0;
    unsigned long valid_gsa = 0;
    unsigned long invalid_gsv = 0;
    unsigned long valid_gsv = 0;
    unsigned long invalid_rmc = 0;
    unsigned long valid_rmc = 0;
    unsigned char gps_fix = 0;
    void reset();
    void dump();
};

class GPS: public PortListener
{
public:
    GPS(Context ctx, Port* port);
    ~GPS();

    AB_AGENT

    void dumpStats();

private:
    virtual void on_line_read(const char *sentence);
    virtual void on_partial(const char *sentence) {};
    void send_gsv(unsigned long ms);
    bool set_system_time(int sid, RMC &rmc, bool &time_set_flag);

    Context ctx;
    Port* p;
    bool enabled;
    time_t delta_time;
    bool gps_time_set;
    GPS_stats stats;
};




#endif