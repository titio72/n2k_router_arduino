#if (GPS_TYPE==1)
#include "GPS_I2C.h"
#include "Utils.h"
#include "TwoWireProvider.h"
#include "Log.h"
#include "N2K_router.h"
#include "Conf.h"
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

#define GPS_LOG_TAG "GPS"

Context* pCtx = NULL;
SFE_UBLOX_GNSS myGNSS;

bool bSAT, bDOP, bPVT;
UBX_NAV_SAT_data_t dSAT;
UBX_NAV_DOP_data_t dDOP;
UBX_NAV_PVT_data_t dPVT;

time_t unix_time;

// enable/disable sending sats 130577
#define SEND_SATS 0

GPSX::GPSX(Context _ctx): ctx(_ctx), enabled(false), last_read_time(0), delta_time(0), gps_time_set(false)
{
    pCtx = &ctx;
}

GPSX::~GPSX()
{
}

const char* SATS_TYPES[] = {
        "GPS     ", "SBAS    ", "Galileo ",
        "BeiDou  ", "IMES    ", "QZSS    ", "GLONASS "
    };

void loadSats(sat* satellites, UBX_NAV_SAT_data_t* d, GSA& gsa, GSV& gsv)
{
    int usedSats = 0;
    for (int i = 0; i<d->header.numSvs; i++)
    {
        satellites[i].sat_id = d->blocks[i].svId;
        satellites[i].az = d->blocks[i].azim;
        satellites[i].elev = d->blocks[i].elev;
        satellites[i].db = d->blocks[i].cno;
        satellites[i].used = d->blocks[i].flags.bits.svUsed;

        if (satellites[i].used)
        {
            gsa.sats[usedSats] = satellites[i].sat_id;
            usedSats++;
            //Log::trace("Sat {%s} PRN {%d} Az {%d} Elev {%d} DB {%d}\n",
            //    SATS_TYPES[d->blocks[i].gnssId], satellites[i].sat_id,
            //    satellites[i].az, satellites[i].elev, satellites[i].db);
        }
    }
    gsa.nSat = usedSats;
    gsv.nSat = d->header.numSvs;
    //Log::trace("[GPS] Loaded {%d/%d} sats\n", gsv.nSat, gsa.nSat);
}

void loadFix(UBX_NAV_SAT_data_t* d, GSA& gsa) {
    gsa.fix = dPVT.fixType;
    gsa.hdop = dDOP.hDOP / 100.0;
    gsa.vdop = dDOP.vDOP / 100.0;
    gsa.tdop = dDOP.tDOP / 100.0;
    gsa.pdop = dDOP.pDOP / 100.0;
    gsa.valid = dPVT.flags.bits.gnssFixOK;
}

bool GPSX::set_system_time(unsigned char sid)
{
  if (unix_time > 0)
  {
    if (ctx.conf.get_services().send_time)
    {
      ctx.n2k.sendSystemTime(ctx.cache.rmc, sid);
    }
    if (!gps_time_set)
    {
      delta_time = unix_time - time(0);
      Log::tracex(GPS_LOG_TAG, "Set time", "UTC {%04d-%02d-%02d %02d:%02d:%02d}",
        ctx.cache.rmc.y, ctx.cache.rmc.M, ctx.cache.rmc.d, ctx.cache.rmc.h, ctx.cache.rmc.m, ctx.cache.rmc.s);
      gps_time_set = true;
    }
  }
  return false;
}

void GPSX::manageLowFrequency(unsigned long ms)
{
    static unsigned long lastT = 0;
    if (bSAT && bDOP && bPVT && (ms-lastT)>950)
    {
        lastT = ms;

        loadSats(ctx.cache.gsv.satellites, &dSAT, ctx.cache.gsa, ctx.cache.gsv);
        loadFix(&dSAT, ctx.cache.gsa);

        static N2KSid _sid;
        unsigned char sid = _sid.getNew();
        ctx.n2k.sendGNNSStatus(ctx.cache.gsa, sid);
        ctx.n2k.sendGNSSPosition(ctx.cache.gsa, ctx.cache.rmc, sid);
        if (ctx.conf.get_services().sog_2_stw)
        {
            ctx.n2k.sendSTW(ctx.cache.rmc.sog);
        }
        #if (SEND_SATS==1)
        ctx.n2k.sendSatellites(ctx.cache.gsv.satellites, ctx.cache.gsv.nSat, sid, ctx.cache.gsa);
        #endif
        set_system_time(sid);
    }
}

void getDOP(UBX_NAV_DOP_data_t* d)
{
    dDOP = *d;
    bDOP = true;
}

void getSat(UBX_NAV_SAT_data_t* d)
{
    dSAT = *d;
    bSAT = true;
}

bool high_freq_signal = false;

void getPVT(UBX_NAV_PVT_data_t* d)
{
    high_freq_signal = true;
    RMC& rmc = pCtx->cache.rmc;
    dPVT = *d;
    bPVT = true;

    if (d->fixType>=1)
    {
        unix_time = myGNSS.getUnixEpoch();
    }
    else
    {
        rmc.unix_time = 0;
    }

    rmc.valid = d->flags.bits.gnssFixOK;
    // set cog in deg
    rmc.cog = d->headMot / 100000.0f; // headVeh is deg*1e-05
    // set sog in Kn
    rmc.sog = (d->gSpeed / 1000.0f) * 3600.0f / 1852.0f; // gSpeed is mm/s
    rmc.y = d->year;
    rmc.M = d->month;
    rmc.d = d->day;
    rmc.h = d->hour;
    rmc.m = d->min;
    rmc.s = d->sec;
    rmc.unix_time = unix_time;
    rmc.lat = d->lat / 10000000.0f;
    rmc.lon = d->lon / 10000000.0f;

    pCtx->cache.latitude = abs(d->lat / 10000000.0f);
    pCtx->cache.latitude_NS = d->lat>0.0?'N':'S';
    pCtx->cache.longitude = abs(d->lon / 10000000.0f);
    pCtx->cache.longitude_EW = d->lon>0.0?'W':'E';
}

void GPSX::manageHighFrequency(unsigned long ms)
{
    ctx.n2k.sendCOGSOG(ctx.cache.rmc);
    ctx.n2k.sendPosition(ctx.cache.rmc);
}

void GPSX::loop(unsigned long ms)
{
    if (enabled)
    {
        if (check_elapsed(ms, last_read_time, 20000))
        {
            pCtx = &ctx;
            bDOP = false;
            bPVT = false;
            bSAT = false;
            myGNSS.checkUblox();
            myGNSS.checkCallbacks();
            if (high_freq_signal)
            {
                high_freq_signal = false;
                manageHighFrequency(ms);
                manageLowFrequency(ms);
            }
        }
    }
}

void GPSX::setup()
{
}

bool GPSX::is_enabled()
{
    return enabled;
}

void GPSX::enable()
{
  if (!enabled)
  {
    enabled = myGNSS.begin(*TwoWireProvider::get_two_wire(), 0x42, 1100U, false);
    if (enabled)
    {
        myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
        myGNSS.setNavigationFrequency(4);
        myGNSS.setAutoNAVSATcallbackPtr(&getSat);
        myGNSS.setAutoPVTcallbackPtr(&getPVT);
        myGNSS.setAutoDOPcallbackPtr(&getDOP);

    }
    Log::tracex(GPS_LOG_TAG, "Enable", "Success {%d}", enabled);
  }
}

void GPSX::disable()
{
    RMC& rmc = ctx.cache.rmc;
    rmc.unix_time = 0;
    rmc.valid = 0xFFFF;
    rmc.cog = NAN;
    rmc.sog = NAN;
    rmc.y = 0xFFFF;
    rmc.M = 0xFFFF;
    rmc.d = 0xFFFF;
    rmc.h = 0xFFFF;
    rmc.m = 0xFFFF;
    rmc.s = 0xFFFF;
    rmc.lat = NAN;
    rmc.lon = NAN;
    ctx.cache.latitude = NAN;
    ctx.cache.latitude_NS = 'N';
    ctx.cache.longitude = NAN;
    ctx.cache.longitude_EW = 'E';

    myGNSS.end();
    enabled = false;
    Log::tracex(GPS_LOG_TAG, "Enable", "Success {%d}", !enabled);
}

void GPSX::dumpStats()
{
    if (enabled)
    {
        Log::tracex(GPS_LOG_TAG, "Stats", "UTC {%04d-%02d-%02dT%02d:%02d:%02d} Pos {%.4f %.4f} SOG {%.2f} COG {%.2f} Sats {%d/%d} hDOP {%.2f} pDOP {%.2f} Fix {%d}",
            ctx.cache.rmc.y, ctx.cache.rmc.M, ctx.cache.rmc.d, ctx.cache.rmc.h, ctx.cache.rmc.m, ctx.cache.rmc.s,
            ctx.cache.rmc.lat, ctx.cache.rmc.lon, ctx.cache.rmc.sog, ctx.cache.rmc.cog,
            ctx.cache.gsv.nSat, ctx.cache.gsa.nSat, ctx.cache.gsa.hdop, ctx.cache.gsa.pdop, ctx.cache.gsa.fix);
    }
}
#endif