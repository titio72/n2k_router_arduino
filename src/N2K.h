#ifndef N2K_H
#define N2K_H

#include <N2kMessages.h>
#include "Utils.h"

class N2K {

    public:
        bool sendCOGSOG(GSA& gsa, RMC& rmc, int sid);
        bool sendTime(time_t t, int sid, short ms = 0);
        bool sendTime(RMC& rmc, int sid);
        bool sendLocalTime(GSA& gsa, RMC& rmc);
        bool sendPosition(GSA& gsa, RMC& rmc);
        bool sendEnvironment(const float pressure, const float humidity, const float temperature, int sid);
        bool sendElectronicTemperature(const float temp, int sid);
        bool send126996Request(int dst);

        bool sendPressure(float pressure, int sid);
        bool sendHumidity(float pressure, int sid);
        bool sendTemperature(float pressure, int sid);

        bool sendMessage(int dest, ulong pgn, int priority, int len, unsigned char* payload);

        bool sendGNSSPosition(GSA& gsa, RMC& rmc, int sid);

        bool sendGNNSStatus(GSA& gsa, int sid);

        void setup(void (*_MsgHandler)(const tN2kMsg &N2kMsg), statistics* stats, uint8_t src);

        bool sendSatellites(const sat* sats, uint n, int sid, GSA& gsa);

        void loop();

    private:
        uint8_t src;
        statistics* stats;
        bool send_msg(const tN2kMsg &N2kMsg);
};

#endif