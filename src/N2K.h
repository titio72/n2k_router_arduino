#ifndef N2K_H
#define N2K_H

#include <N2kMessages.h>
#include "Utils.h"

class N2K {

    public:
        N2K(void (*_MsgHandler)(const tN2kMsg &N2kMsg), statistics* stats);

        bool sendMessage(int dest, ulong pgn, int priority, int len, unsigned char* payload);
        bool sendCOGSOG(RMC& rmc, unsigned char sid = 0xFF);
        bool sendCOGSOG(double sog, double cog, unsigned char sid);
        bool sendTime(time_t t, unsigned char sid, short ms = 0);
        bool sendTime(RMC& rmc, unsigned char sid);
        bool sendLocalTime(GSA& gsa, RMC& rmc);
        bool sendPosition(RMC& rmc);
        bool sendPosition(double latitude, double longitude);
        bool sendCabinTemp(const float temperature, unsigned char sid = 0xFF);
        bool sendHumidity(const float humidity, unsigned char sid = 0xFF);
        bool sendPressure(const float pressure, unsigned char sid = 0xFF);
        bool sendElectronicTemperature(const float temp, unsigned char sid = 0xFF);
        bool send126996Request(int dst);
        bool sendGNSSPosition(GSA& gsa, RMC& rmc, unsigned char sid);
        bool sendGNSSPosition(int y, int M, int d, int h, int m, int s, unsigned long unixTime, float lat, float lon, int nsat, float hdop, float pdop, unsigned char sid);
        bool sendGNNSStatus(GSA& gsa, unsigned char sid);
        bool sendGNNSStatus(int fix, float hdop, float vdop, float tdop, unsigned char sid);
        bool sendSatellites(const sat* sats, uint n, unsigned char sid, GSA& gsa);

        void setup();

        void loop(unsigned long time);

        bool send_msg(const tN2kMsg &N2kMsg);

        bool is_initialized() { return initialized; }

        // used only on linux
        void set_can_socket_name(const char* name);

        void dumpStats();

    private:
        statistics* stats;
        bool initialized;
};

#endif
