#include "GPS_I2C.h"
#include "Utils.h"
#include "TwoWireProvider.h"
#include "Log.h"
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

GPSX::GPSX(Context _ctx): myGNSS(NULL), ctx(ctx), enabled(false), last_read_time(0)
{
}

GPSX::~GPSX()
{
    if (myGNSS) delete myGNSS;
}

void GPSX::loop(unsigned long ms)
{
    if (enabled)
    {
        if (ms - last_read_time > 100)
        {
            myGNSS->enableGNSS(true, sfe_ublox_gnss_ids_e::SFE_UBLOX_GNSS_ID_GPS);
            last_read_time = ms; //Update the timer
            if (myGNSS->checkUblox())
            {
                int fixType = myGNSS->getFixType();
                Log::trace("Fix: %d\n", fixType);

                if (myGNSS->getGnssFixOk())
                {
                    long latitude = myGNSS->getLatitude();
                    Log::trace("Lat: %f\n", latitude);

                    long longitude = myGNSS->getLongitude();
                    Log::trace("Lon: %f\n", longitude);

                    double speed = myGNSS->getGroundSpeed() / 1000.0;
                    Log::trace("Speed: %.2f mm/s<n", speed);

                    double heading = myGNSS->getHeading() / 10000.0;
                    Log::trace("Heading: %.2f\n", heading);

                    double pDOP = myGNSS->getPDOP() / 100.0;
                    Log::trace("pDOP: %.2f\n", pDOP);
                }

                if (myGNSS->getNAVSAT())
                {
                    UBX_NAV_SAT_data_t data;
                    myGNSS->setNavsat
                }


                int time = myGNSS->getUnixEpoch();
                Log::trace("Unix time %d\n", time);
                if (myGNSS->getDateValid())
                {
                    int dayUTC =  myGNSS->getDay();
                    int monthUTC = myGNSS->getMonth();
                    int yearUTC = myGNSS->getYear();
                    int hourUTC = myGNSS->getHour();
                    int minuteUTC = myGNSS->getMinute();
                    int secondsUTC = myGNSS->getSecond();
                    int millisUTC = myGNSS->getMillisecond();
                    Log::trace("%4d-%02d-%02dT%02d:%02d.:%02dZ.%03d/n", yearUTC, monthUTC, dayUTC, hourUTC, minuteUTC, secondsUTC, millisUTC);
                }
            }
        }
    }
}

void GPSX::setup()
{
    myGNSS = new SFE_UBLOX_GNSS();
}

bool GPSX::is_enabled()
{
    return enabled;
}

void GPSX::enable()
{
  if (!enabled)
  {
    enabled = myGNSS->begin(*TwoWireProvider::get_two_wire(), 0x42, 1100U, false);
  }
}

void GPSX::disable()
{
    enabled = false;
}