#ifndef N2K_ROUTER_H_
#define N2K_ROUTER_H_

#include <Utils.h>
#include <N2K.h>
#include "Data.h"

#ifndef N2K_SOURCE
#define N2K_SOURCE 22
#endif

class N2K_router
{

    public:
        N2K_router(N2K &n2k);

        bool sendMessage(int dest, ulong pgn, int priority, int len, unsigned char* payload);
        bool sendCOGSOG(RMC& rmc, unsigned char sid = 0xFF);
        bool sendCOGSOG(double sog, double cog, unsigned char sid);
        bool sendSTW(double sog);
        bool sendSystemTime(time_t t, unsigned char sid, short ms = 0);
        bool sendSystemTime(RMC& rmc, unsigned char sid);
        bool sendLocalTime(GSA& gsa, RMC& rmc);
        bool sendPosition(RMC& rmc);
        bool sendPosition(double latitude, double longitude);
        bool sendCabinTemp(const double temperature, unsigned char sid = 0xFF);
        bool sendEnvironmentXRaymarine(const double pressure, const double humidity, const double temperature);
        bool sendHumidity(const double humidity, unsigned char sid = 0xFF);
        bool sendPressure(const double pressure, unsigned char sid = 0xFF);
        bool sendElectronicTemperature(const double temp, unsigned char sid = 0xFF);
        bool sendGNSSPosition(GSA& gsa, RMC& rmc, unsigned char sid);
        bool sendGNSSPosition(int y, int M, int d, int h, int m, int s, unsigned long unixTime, double lat, double lon, int nsat, double hdop, double pdop, unsigned char sid);
        bool sendGNNSStatus(GSA& gsa, unsigned char sid);
        bool sendGNNSStatus(int fix, double hdop, double vdop, double tdop, unsigned char sid);
        bool sendSatellites(const sat* sats, uint n, unsigned char sid, GSA& gsa);
        bool sendBattery(unsigned char sid, const double voltage, const double current, const double temperature, const unsigned char instance);
        bool sendBatteryStatus(unsigned char sid, const double soc, const double capacity, const double ttg, const unsigned char instance);
        bool sendEngineRPM(uint8_t engine_n, uint16_t rpm);
        bool sendEngineHours(uint8_t engine_n, double hours);

        N2K& get_bus();

    private:
        N2K &n2k;
};

#endif
