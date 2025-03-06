#ifndef _BMV_H
#define _BMV_H

#include <Ports.h>
#include <VeDirect.h>
#include <time.h>
#include "Context.h"

class BMV712: public PortListener
{
public:
    BMV712(Context& ctx, Port& port);
    ~BMV712();

    void loop(unsigned long t);
    void setup();
    bool is_enabled();
    void enable();
    void disable();

    double getVoltage();

private:
    virtual void on_line_read(const char *line);
    virtual void on_partial_x(const char *line, int len);

    Context& ctx;
    Port& p;
    bool enabled;
    time_t delta_time;
    VEDirectObject bmv;

    unsigned long last_read_time;

    int checksum;
};

#endif