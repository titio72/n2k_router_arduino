#ifndef _MeteoBME_H
#define _MeteoBME_H

#include "Agents.hpp"

class BME280Internal
{
public:
    virtual bool start() = 0;
    virtual void stop() = 0;
    
    virtual float readPressure() = 0;
    virtual float readTemperature() = 0;
    virtual float readHumidity() = 0;

};

class MeteoBME
{
public:
    MeteoBME(int address, uint8_t meteo_index = 0, BME280Internal* impl = nullptr);
    ~MeteoBME();

    AB_AGENT

private:
    bool internalStateOwned;
    BME280Internal *bme;
    bool enabled;
    unsigned long last_read;
    int address;
    uint8_t meteo_index;

#ifdef PIO_UNIT_TESTING
public:
#endif
    void read(unsigned long ms, MeteoData& data);
};

#endif