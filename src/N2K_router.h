#ifndef N2K_ROUTER_H_
#define N2K_ROUTER_H_

#include <Utils.h>
#include <N2K.h>
#include "Agents.hpp"
#include "Data.h"

#ifndef N2K_SOURCE
#define N2K_SOURCE 22
#endif


class N2KSender
{
public:
    virtual bool sendMessage(int dest, ulong pgn, int priority, int len, unsigned char* payload) = 0;
    virtual bool sendCOGSOG(RMC& rmc, unsigned char sid = 0xFF) = 0;
    virtual bool sendCOGSOG(double sog, double cog, unsigned char sid) = 0;
    virtual bool sendSTW(double sog) = 0;
    virtual bool sendSystemTime(uint32_t t, unsigned char sid, uint16_t t_ms = 0) = 0;
    virtual bool sendSystemTime(RMC& rmc, unsigned char sid) = 0;
    virtual bool sendLocalTime(GSA& gsa, RMC& rmc) = 0;
    virtual bool sendPosition(RMC& rmc) = 0;
    virtual bool sendPosition(double latitude, double longitude) = 0;
    virtual bool sendCabinTemp(const double temperature, unsigned char sid = 0xFF) = 0;
    virtual bool sendEnvironmentXRaymarine(const double pressure, const double humidity, const double temperature) = 0;
    virtual bool sendHumidity(const double humidity, unsigned char sid = 0xFF) = 0;
    virtual bool sendPressure(const double pressure, unsigned char sid = 0xFF) = 0;
    virtual bool sendElectronicTemperature(const double temp, unsigned char sid = 0xFF) = 0;
    virtual bool sendGNSSPosition(GSA& gsa, RMC& rmc, unsigned char sid) = 0;
    virtual bool sendGNSSPosition(int y, int M, int d, int h, int m, int s, unsigned long unixTime, double lat, double lon, int nsat, double hdop, double pdop, unsigned char sid) = 0;
    virtual bool sendGNNSStatus(GSA& gsa, unsigned char sid) = 0;
    virtual bool sendGNNSStatus(int fix, double hdop, double vdop, double tdop, unsigned char sid) = 0;
    virtual bool sendSatellites(const sat* sats, uint n, unsigned char sid, GSA& gsa) = 0;
    virtual bool sendBattery(unsigned char sid, const double voltage, const double current, const double temperature, const unsigned char instance) = 0;
    virtual bool sendBatteryStatus(unsigned char sid, const double soc, const double capacity, const double ttg, const unsigned char instance) = 0;
    virtual bool sendEngineRPM(uint8_t engine_n, uint16_t rpm) = 0;
    virtual bool sendEngineHours(uint8_t engine_n, double hours) = 0;

    virtual N2KStats getStats() = 0;
};

class NullN2KSender: public N2KSender
{
public:
    NullN2KSender()
    {
        stats.canbus = 1;
    }

    virtual bool sendMessage(int dest, ulong pgn, int priority, int len, unsigned char* payload) { stats.sent++; return true; }
    virtual bool sendCOGSOG(RMC& rmc, unsigned char sid = 0xFF) { stats.sent++; return true; }
    virtual bool sendCOGSOG(double sog, double cog, unsigned char sid) { stats.sent++; return true; }
    virtual bool sendSTW(double sog) { stats.sent++; return true; }
    virtual bool sendSystemTime(uint32_t t, unsigned char sid, uint16_t t_ms = 0) { stats.sent++; return true; }
    virtual bool sendSystemTime(RMC& rmc, unsigned char sid) { stats.sent++; return true; }
    virtual bool sendLocalTime(GSA& gsa, RMC& rmc) { stats.sent++; return true; }
    virtual bool sendPosition(RMC& rmc) { stats.sent++; return true; }
    virtual bool sendPosition(double latitude, double longitude) { stats.sent++; return true; }
    virtual bool sendCabinTemp(const double temperature, unsigned char sid = 0xFF) { stats.sent++; return true; }
    virtual bool sendEnvironmentXRaymarine(const double pressure, const double humidity, const double temperature) { stats.sent++; return true; }
    virtual bool sendHumidity(const double humidity, unsigned char sid = 0xFF) { stats.sent++; return true; }
    virtual bool sendPressure(const double pressure, unsigned char sid = 0xFF) { stats.sent++; return true; }
    virtual bool sendElectronicTemperature(const double temp, unsigned char sid = 0xFF) { stats.sent++; return true; }
    virtual bool sendGNSSPosition(GSA& gsa, RMC& rmc, unsigned char sid) { stats.sent++; return true; }
    virtual bool sendGNSSPosition(int y, int M, int d, int h, int m, int s, unsigned long unixTime, double lat, double lon, int nsat, double hdop, double pdop, unsigned char sid) { stats.sent++; return true; }
    virtual bool sendGNNSStatus(GSA& gsa, unsigned char sid) { stats.sent++; return true; }
    virtual bool sendGNNSStatus(int fix, double hdop, double vdop, double tdop, unsigned char sid) { stats.sent++; return true; }
    virtual bool sendSatellites(const sat* sats, uint n, unsigned char sid, GSA& gsa) { stats.sent++; return true; }
    
    virtual bool sendBattery(unsigned char sid, const double voltage, const double current, const double temperature, const unsigned char instance) 
    {
        stats.sent++;
        battery++; 
        return true;
    }

    virtual bool sendBatteryStatus(unsigned char sid, const double soc, const double capacity, const double ttg, const unsigned char instance)
    {
        stats.sent++;
        batteryStatus++;
        return true;
    }

    virtual bool sendEngineRPM(uint8_t engine_n, uint16_t rpm) { stats.sent++; return true; }
    virtual bool sendEngineHours(uint8_t engine_n, double hours) { stats.sent++; return true; }

    virtual N2KStats getStats() { return stats; }

    unsigned int battery = 0;
    unsigned int batteryStatus = 0;

private:
    N2KStats stats;
};


class N2K_router: public N2KSender
{

    public:
        N2K_router(n2k_source_change_handler source_handler = nullptr);

        virtual bool sendMessage(int dest, ulong pgn, int priority, int len, unsigned char* payload);
        virtual bool sendCOGSOG(RMC& rmc, unsigned char sid = 0xFF);
        virtual bool sendCOGSOG(double sog, double cog, unsigned char sid);
        virtual bool sendSTW(double sog);
        virtual bool sendSystemTime(uint32_t t, unsigned char sid, uint16_t t_ms = 0);
        virtual bool sendSystemTime(RMC& rmc, unsigned char sid);
        virtual bool sendLocalTime(GSA& gsa, RMC& rmc);
        virtual bool sendPosition(RMC& rmc);
        virtual bool sendPosition(double latitude, double longitude);
        virtual bool sendCabinTemp(const double temperature, unsigned char sid = 0xFF);
        virtual bool sendEnvironmentXRaymarine(const double pressure, const double humidity, const double temperature);
        virtual bool sendHumidity(const double humidity, unsigned char sid = 0xFF);
        virtual bool sendPressure(const double pressure, unsigned char sid = 0xFF);
        virtual bool sendElectronicTemperature(const double temp, unsigned char sid = 0xFF);
        virtual bool sendGNSSPosition(GSA& gsa, RMC& rmc, unsigned char sid);
        virtual bool sendGNSSPosition(int y, int M, int d, int h, int m, int s, unsigned long unixTime, double lat, double lon, int nsat, double hdop, double pdop, unsigned char sid);
        virtual bool sendGNNSStatus(GSA& gsa, unsigned char sid);
        virtual bool sendGNNSStatus(int fix, double hdop, double vdop, double tdop, unsigned char sid);
        virtual bool sendSatellites(const sat* sats, uint n, unsigned char sid, GSA& gsa);
        virtual bool sendBattery(unsigned char sid, const double voltage, const double current, const double temperature, const unsigned char instance);
        virtual bool sendBatteryStatus(unsigned char sid, const double soc, const double capacity, const double ttg, const unsigned char instance);
        virtual bool sendEngineRPM(uint8_t engine_n, uint16_t rpm);
        virtual bool sendEngineHours(uint8_t engine_n, double hours);

        virtual N2KStats getStats();

        void set_desired_source(unsigned char src);

        AB_AGENT

    private:
        N2K &n2k;
};

#endif
