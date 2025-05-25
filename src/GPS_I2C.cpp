#if GPS_TYPE == 1
#include "GPS_I2C.h"
#include <Utils.h>
#include <TwoWireProvider.h>
#include <Log.h>
#include "N2K_router.h"
#include "Conf.h"
#include "Data.h"


#define GPS_LOG_TAG "GPS"

// enable/disable sending sats 130577
#define SEND_SATS 0

#define READ_PERIOD 50000           // microseconds
#define HIG_LOW_FREQ_RATIO 5        // manage a low freq event every 4 high freq events
#define NAVIGATION_DATA_FREQUENCY 5 // Hz

#pragma region GPS_UTILS
const char *SATS_TYPES[] = {
    "GPS     ", "SBAS    ", "Galileo ",
    "BeiDou  ", "IMES    ", "QZSS    ", "GLONASS "};

void reset_gps_data(Data& data)
{
    RMC &rmc = data.rmc;
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
    data.latitude = NAN;
    data.latitude_NS = 'N';
    data.longitude = NAN;
    data.longitude_EW = 'E';

    GSA &gsa = data.gsa;
    gsa.nSat = 0;
    gsa.fix = 0;
    gsa.valid = 0xFFFF;
    gsa.hdop = NAN;
    gsa.vdop = NAN;
    gsa.tdop = NAN;
    gsa.pdop = NAN;

    data.gsv.nSat = 0;
}


bool GPSX::loadSats()
{
    if (!myGNSS.getNAVSAT()) return false;

    sat *satellites = ctx.cache.gsv.satellites;
    UBX_NAV_SAT_data_t* d = &myGNSS.packetUBXNAVSAT->data;
    int usedSats = 0;
    for (int i = 0; i < d->header.numSvs; i++)
    {
        satellites[i].sat_id = d->blocks[i].svId;
        satellites[i].az = d->blocks[i].azim;
        satellites[i].elev = d->blocks[i].elev;
        satellites[i].db = d->blocks[i].cno;
        satellites[i].used = d->blocks[i].flags.bits.svUsed;

        if (satellites[i].used)
        {
            ctx.cache.gsa.sats[usedSats] = satellites[i].sat_id;
            usedSats++;
            // Log::trace("Sat {%s} PRN {%d} Az {%d} Elev {%d} DB {%d}\n",
            //     SATS_TYPES[d->blocks[i].gnssId], satellites[i].sat_id,
            //     satellites[i].az, satellites[i].elev, satellites[i].db);
        }
    }
    ctx.cache.gsa.nSat = usedSats;
    ctx.cache.gsv.nSat = d->header.numSvs;
    // Log::trace("[GPS] Loaded {%d/%d} sats\n", gsv.nSat, gsa.nSat);
    return true;
}

bool GPSX::loadFix()
{
    // 0=no fix, 1=dead reckoning, 2=2D, 3=3D, 4=GNSS, 5=Time fix
    GSA& gsa = ctx.cache.gsa;
    if (myGNSS.getDOP() && myGNSS.getPVT())
    {
        gsa.fix = myGNSS.getFixType();
        gsa.hdop = myGNSS.getHorizontalDOP() / 100.0;
        gsa.vdop = myGNSS.getVerticalDOP() / 100.0;
        gsa.tdop = myGNSS.getTimeDOP() / 100.0;
        gsa.pdop = myGNSS.getPDOP() / 100.0;
        gsa.valid = gsa.fix == 2 || gsa.fix == 3; // dPVT.flags.bits.gnssFixOK;
        return true;
    }
    else
    {
        return false;
    }
}

bool GPSX::loadPVT()
{
    if (!myGNSS.getPVT()) return false;

    RMC &rmc = ctx.cache.rmc;
    uint8_t fixType = myGNSS.getFixType();
    if (fixType >= 1)
    {
        rmc.unix_time = myGNSS.getUnixEpoch();
    }
    else
    {
        rmc.unix_time = 0;
    }

    rmc.valid = fixType == 2 || fixType == 3; // d->flags.bits.gnssFixOK;
    // set cog in deg
    rmc.cog = myGNSS.getHeading() / 100000.0f; // headVeh is deg*1e-05
    // set sog in Kn
    rmc.sog = (myGNSS.getGroundSpeed() / 1000.0f) * 3600.0f / 1852.0f; // gSpeed is mm/s
    rmc.y = myGNSS.getYear();
    rmc.M = myGNSS.getMonth();
    rmc.d = myGNSS.getDay();
    rmc.h = myGNSS.getHour();
    rmc.m = myGNSS.getMinute();
    rmc.s = myGNSS.getSecond();
    rmc.lat = myGNSS.getLatitude() / 10000000.0f;
    rmc.lon = myGNSS.getLongitude() / 10000000.0f;

    ctx.cache.latitude = abs(rmc.lat / 10000000.0f);
    ctx.cache.latitude_NS = rmc.lat > 0.0 ? 'N' : 'S';
    ctx.cache.longitude = abs(rmc.lon / 10000000.0f);
    ctx.cache.longitude_EW = rmc.lon > 0.0 ? 'W' : 'E';

    return true;
}

GPSX::GPSX(Context _ctx) : ctx(_ctx), enabled(false), last_read_time(0), delta_time(0), gps_time_set(false), count_sent(0), myGNSS()
{
}

GPSX::~GPSX()
{
}

bool GPSX::set_system_time(unsigned char sid)
{
    if (ctx.cache.rmc.unix_time > 0)
    {
        if (!gps_time_set)
        {
            delta_time = ctx.cache.rmc.unix_time - time(0);
            Log::tracex(GPS_LOG_TAG, "Set time", "UTC {%04d-%02d-%02d %02d:%02d:%02d}",
                        ctx.cache.rmc.y, ctx.cache.rmc.M, ctx.cache.rmc.d, ctx.cache.rmc.h, ctx.cache.rmc.m, ctx.cache.rmc.s);
            gps_time_set = true;
        }
    }
    return false;
}

void GPSX::manageLowFrequency(unsigned long micros)
{
    if (loadFix() && loadPVT() && count_sent == 0)
    {
        static N2KSid _sid;
        unsigned char sid = _sid.getNew();
        ctx.n2k.sendGNNSStatus(ctx.cache.gsa, sid);
        ctx.n2k.sendGNSSPosition(ctx.cache.gsa, ctx.cache.rmc, sid);
        if (ctx.conf.get_services().sog_2_stw)
        {
            ctx.n2k.sendSTW(ctx.cache.rmc.sog);
        }
#if (SEND_SATS == 1)
        ctx.n2k.sendSatellites(ctx.cache.gsv.satellites, ctx.cache.gsv.nSat, sid, ctx.cache.gsa);
#endif
        set_system_time(sid);
    }
}

void GPSX::manageHighFrequency(unsigned long micros)
{
    if (loadPVT())
    {
        ctx.n2k.sendCOGSOG(ctx.cache.rmc);
        ctx.n2k.sendPosition(ctx.cache.rmc);
    }
    count_sent++;
    count_sent %= HIG_LOW_FREQ_RATIO;

}

void GPSX::loop(unsigned long micros)
{
    if (enabled && check_elapsed(micros, last_read_time, READ_PERIOD))
    {
        manageHighFrequency(micros);
        manageLowFrequency(micros);
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
        Log::tracex(GPS_LOG_TAG, "Enabling", "Type {%s}", "I2C");
        bool _enabled = myGNSS.begin(*TwoWireProvider::get_two_wire(), 0x42, 1100U, false);
        if (_enabled)
        {
            myGNSS.setI2COutput(COM_TYPE_UBX); // Set the I2C port to output UBX only (turn off NMEA noise)
            myGNSS.setNavigationFrequency(NAVIGATION_DATA_FREQUENCY);
            myGNSS.setAutoPVT(true);
            myGNSS.setAutoNAVSAT(true);
            myGNSS.setAutoDOP(true);
        }
        enabled = _enabled;
        Log::tracex(GPS_LOG_TAG, "Enable", "Success {%d}", enabled);
    }
}

void GPSX::disable()
{
    if (enabled)
    {
        enabled = false;

        reset_gps_data(ctx.cache);

        myGNSS.end();

        Log::tracex(GPS_LOG_TAG, "Disable", "Success {%d}", !enabled);
    }
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