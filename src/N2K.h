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

        int get_sent() { return g_pos_sent; }
        int get_sent_fail() { return g_pos_sent_fail; }
        void reset_counters() {
            g_pos_sent = 0;
            g_pos_sent_fail = 0;
        }

        void setup(void (*_MsgHandler)(const tN2kMsg &N2kMsg));

        void loop();

    private:
        uint g_pos_sent;
        uint g_pos_sent_fail;
        void (*handler)(const tN2kMsg &N2kMsg);
        void handle_message(const tN2kMsg &N2kMsg);
};

#endif