#ifndef _GPSX_H
#define _GPSX_H

#include "Utils.h"
#include "Context.h"
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

#ifndef GPS_RX_PIN
#define GPS_RX_PIN 17 // Default RX pin for GPS
#endif
#ifndef GPS_TX_PIN
#define GPS_TX_PIN 16 // Default TX pin for GPS
#endif

class GPSX
{
public:
    GPSX(Context _ctx, HardwareSerial *serial_port = nullptr, int rx_pin = GPS_RX_PIN, int tx_pin = GPS_TX_PIN);
    ~GPSX();

    AB_AGENT

    double getLatitude();
    double getLongitude();
    double getAltitude();

    void dumpStats();

private:
    void manageLowFrequency(unsigned long tstamp);
    void manageHighFrequency(unsigned long tstamp);
    bool set_system_time(unsigned char sid);
    Context ctx;
    bool enabled;
    unsigned long last_read_time;
    time_t delta_time;
    bool gps_time_set;
    int count_sent;
    bool cache_ok;
    HardwareSerial *serial_port;
    int rx_pin;
    int tx_pin;

    SFE_UBLOX_GNSS myGNSS;

    bool loadFix();
    bool loadSats();
    bool loadPVT();
};
#endif
