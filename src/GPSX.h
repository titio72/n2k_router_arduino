#ifndef _GPSX_H
#define _GPSX_H

#ifndef NATIVE

#include "Utils.h"
#include "Agents.hpp"
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
//#include <SparkFun_u-blox_GNSS_v3.h>

#ifndef GPS_RX_PIN
#define GPS_RX_PIN 17 // Default RX pin for GPS
#endif
#ifndef GPS_TX_PIN
#define GPS_TX_PIN 16 // Default TX pin for GPS
#endif

class GPSX
{
public:
    GPSX(HardwareSerial *serial_port = nullptr, int rx_pin = GPS_RX_PIN, int tx_pin = GPS_TX_PIN);
    ~GPSX();

    AB_AGENT

    void dumpStats();

private:
    void manageLowFrequency(unsigned long tstamp, Context& ctx);
    void manageHighFrequency(unsigned long tstamp, Context &ctx);
    bool enabled;
    unsigned long last_read_time;

    int count_sent;
    bool cache_ok;
    HardwareSerial *serial_port;
    int rx_pin;
    int tx_pin;
    TaskHandle_t GNSS_Task;
    SFE_UBLOX_GNSS myGNSS;

    GPSData data;

    bool loadFix();
    bool loadSats();
    bool loadPVT();
};
#endif

#endif
