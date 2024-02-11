#include "GPS_I2C.h"
#include "Utils.h"
#include "TwoWireProvider.h"
#include "Log.h"
#include "N2K.h"
#include "Conf.h"
#include "Constants.h"
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

UBX_NAV_SAT_data_t* pSatData = NULL;
bool bRreadingSat = false;
Context* pCtx = NULL;
unsigned long ulTime = 0;

GPSX::GPSX(Context _ctx): ctx(ctx), myGNSS(NULL), enabled(false), last_read_time(0)
{
}

GPSX::~GPSX()
{
    if (myGNSS) delete myGNSS;
}

void getSat(UBX_NAV_SAT_data_t* d)
{
  static unsigned char sid = 0;
  static sat satellites[24];
  sid++;
  static ulong last_sent = 0;
  if ((ulTime - last_sent) >= 900)
  {
    last_sent = ulTime;
    pCtx->cache.gsa.nSat = d->header.numSvs;
    for (int i = 0; i<d->header.numSvs; i++)
    {
        satellites[i].sat_id = d->blocks[i].gnssId;
        satellites[i].az = d->blocks[i].azim;
        satellites[i].elev = d->blocks[i].elev;
        satellites[i].db = d->blocks[i].cno;
    }
  }
}

void GPSX::loop(unsigned long ms)
{
    if (enabled)
    {
        if (ms - last_read_time > 100)
        {
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

                    int dayUTC =  myGNSS->getDay();
                    int monthUTC = myGNSS->getMonth();
                    int yearUTC = myGNSS->getYear();
                    int hourUTC = myGNSS->getHour();
                    int minuteUTC = myGNSS->getMinute();
                    int secondsUTC = myGNSS->getSecond();
                    int millisUTC = myGNSS->getMillisecond();
                    Log::trace("%4d-%02d-%02dT%02d:%02d.:%02dZ.%03d/n", yearUTC, monthUTC, dayUTC, hourUTC, minuteUTC, secondsUTC, millisUTC);

                    ctx.cache.rmc.cog = heading;
                    ctx.cache.rmc.lat = latitude;
                    ctx.cache.rmc.lon = longitude;
                    ctx.cache.rmc.sog = speed;
                    ctx.cache.rmc.d = dayUTC;
                    ctx.cache.rmc.M = monthUTC;
                    ctx.cache.rmc.y = yearUTC;
                    ctx.cache.rmc.h = hourUTC;
                    ctx.cache.rmc.m = minuteUTC;
                    ctx.cache.rmc.s = secondsUTC;

                    static unsigned int sid = 0; sid++;

                    ctx.n2k.sendCOGSOG(ctx.cache.gsa, ctx.cache.rmc, sid);
                    ctx.n2k.sendPosition(ctx.cache.gsa, ctx.cache.rmc);
                    static ulong t0 = millis();
                    ulong t = millis();
                    if ((t - t0) > 900)
                    {
                    ctx.n2k.sendGNSSPosition(ctx.cache.gsa, ctx.cache.rmc, sid);
                    t0 = t;
                    }
                    ctx.stats.valid_rmc++;

                }
                /*
                bRreadingSat = true;
                pCtx = &ctx;
                ulTime = ms;
                myGNSS->getNAVSAT();
                pCtx = NULL;
                bRreadingSat = false;
                */
                int time = myGNSS->getUnixEpoch();
                Log::trace("Unix time %d\n", time);

            }
        }
    }
}

void GPSX::setup()
{
    myGNSS = new SFE_UBLOX_GNSS();
    //myGNSS->setAutoNAVSATcallbackPtr(getSat);
}

bool GPSX::is_enabled()
{
    return enabled;
}

void GPSX::enable()
{
  if (!enabled)
  {
    //Log::trace("[GPS] Initializing {%d}\n", ctx.conf.uart_speed);
    //int speed = atoi(UART_SPEED[ctx.conf.uart_speed]);
    //Log::trace("[GPS] Initializing serial {%d}\n", speed);
    Serial2.begin(57600, SERIAL_8N1, RXD2, TXD2);
    Log::trace("[GPS] Enabling GNSS\n");
    //enabled = myGNSS->begin(*TwoWireProvider::get_two_wire(), 0x42, 1100U, false);
    enabled = myGNSS->begin(Serial2);
    Log::trace("[GPS] Init {%d}\n", enabled);
  }
}

void GPSX::disable()
{
    Serial2.end();
    enabled = false;
}