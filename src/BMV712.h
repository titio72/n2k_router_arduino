#ifndef _BMV_H
#define _BMV_H

#include <Ports.h>
#include <VeDirect.h>
#include <time.h>
#include "Context.h"

class BMV712: public PortListener, VEDirectListener
{
public:
    BMV712(Context& ctx, Port& port, BatteryData& data);
    ~BMV712();

    void loop(unsigned long t);
    void setup();
    bool is_enabled();
    void enable();
    void disable();

    double getVoltage();

private:
    void on_line_read(const char *line);
    void on_partial_x(const char *line, int len);
    void on_complete(VEDirectObject &ve);

    Context& ctx;
    Port& p;
    BatteryData& data;
    bool enabled;
    time_t delta_time;
    VEDirectObject bmv;

    unsigned long last_read_time;

    int checksum;
};

#endif