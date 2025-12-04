#ifndef _BMV_H
#define _BMV_H

#include <Ports.h>
#include <VeDirect.h>
#include <time.h>
#include "Agents.hpp"
#include <Utils.h>

#define VEDIRECT_BAUD_RATE 19200

class BMV712: public PortListener, VEDirectListener
{
public:
    BMV712(Port& port);
    ~BMV712();

    AB_AGENT

    const VEDirectObject &getLastValidValue() const { return bmv_vedirect; }

private:
    void on_line_read(const char *line);
    void on_partial_x(const char *line, int len);
    void on_complete(VEDirectObject &ve);
    Port& p;
    BatteryData data_svc;
    BatteryData data_eng;
    bool read;

    bool enabled;
    time_t delta_time;
    VEDirectObject bmv_vedirect;

    unsigned long last_read_time;

    int checksum;

    N2KSid n2ksid;
};

#endif