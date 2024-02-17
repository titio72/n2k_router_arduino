#ifndef _GPS_H
#define _GPS_H

#include "Ports.h"
#include "Context.h"

class GPS: public PortListener
{
public:
    GPS(Context ctx);
    ~GPS();

    void set_port_name(const char* port_name);

    AB_AGENT

    void dumpStats();

private:
    virtual int on_line_read(const char *sentence);
    void send_gsv(unsigned long ms);
    bool set_system_time(int sid, RMC &rmc, bool &time_set_flag);

    Context ctx;
    Port* p;
    bool enabled;
    char* device_name;
    time_t delta_time;
    bool gps_time_set;

};




#endif