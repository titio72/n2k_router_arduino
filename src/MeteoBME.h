#ifndef _MeteoBME_H
#define _MeteoBME_H

#include "Utils.h"
#include "Context.h"

class Adafruit_BME280;

class MeteoBME
{
public:
    MeteoBME(Context ct, int address, MeteoData& data);
    ~MeteoBME();

    AB_AGENT

private:
    bool enabled;
    Context ctx;
    MeteoData& data;
    Adafruit_BME280 *bme;
    unsigned long last_read;
    int address;
    void read(unsigned long ms);
};

#endif