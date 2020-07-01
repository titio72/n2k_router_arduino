#ifndef N2K_H
#define N2K_H

#include <N2kMessages.h>
#include "Utils.h"

class N2K {

    public:
        bool sendCOGSOG(GSA& gsa, RMC& rmc);
        bool sendTime(time_t t);
        bool sendTime(GSA& gsa, RMC& rmc);
        bool sendLocalTime(GSA& gsa, RMC& rmc);
        bool sendPosition(GSA& gsa, RMC& rmc);
        bool sendEnvironment(const float pressure, const float humidity, const float temperature);

        void setup(void (*_MsgHandler)(const tN2kMsg &N2kMsg), statistics* stats);

        void loop();

    private:
        statistics* stats;
        bool send_msg(const tN2kMsg &N2kMsg);
};

#endif