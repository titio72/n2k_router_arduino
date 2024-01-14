#ifndef N2K_H
#define N2K_H

#include <N2kMessages.h>
#include "Utils.h"

class N2K {

    public:
        N2K(void (*_MsgHandler)(const tN2kMsg &N2kMsg), statistics* stats);

        bool sendMessage(int dest, ulong pgn, int priority, int len, unsigned char* payload);
        bool sendCOGSOG(GSA& gsa, RMC& rmc, int sid);
        bool sendTime(time_t t, int sid, short ms = 0);
        bool sendTime(RMC& rmc, int sid);
        bool sendLocalTime(GSA& gsa, RMC& rmc);
        bool sendPosition(GSA& gsa, RMC& rmc);
        bool sendCabinTemp(const float temperature, int sid);
        bool sendHumidity(const float humidity, int sid);
        bool sendPressure(const float pressure, int sid);
        bool sendElectronicTemperature(const float temp, int sid);
        bool send126996Request(int dst);
        bool sendGNSSPosition(GSA& gsa, RMC& rmc, int sid);
        bool sendGNNSStatus(GSA& gsa, int sid);
        bool sendSatellites(const sat* sats, uint n, int sid, GSA& gsa);

        void setup();

        void loop(unsigned long time);

        bool send_msg(const tN2kMsg &N2kMsg);

        bool is_initialized() { return initialized; }

        // used only on linux
        void set_can_socket_name(const char* name);

    private:
        statistics* stats;
        bool initialized;
};

#endif
