
#ifndef NATIVE
#include "GPSX.h"
#include <Utils.h>
#include <TwoWireProvider.h>
#include <Log.h>
#include "N2K_router.h"
#include "Conf.h"
#include "Data.h"

// enable/disable sending sats 130577
#define SEND_SATS 0

const unsigned long READ_PERIOD = 50000; // microseconds
const int HIG_LOW_FREQ_RATIO = 4;        // manage a low freq event every 4 high freq events
const int NAVIGATION_DATA_FREQUENCY = 4; // Hz

const int GPS_SERIAL_SPEED = 57600; // Default speed for GPS serial port

const char* GPS_LOG_TAG = "GPS";

const char *SATS_TYPES[] = {
    "GPS     ", "SBAS    ", "Galileo ",
    "BeiDou  ", "IMES    ", "QZSS    ", "GLONASS "};

void reset_gps_data(GPSData &data)
{
    data.latitude_signed = NAN;  // Signed latitude for NMEA
    data.longitude_signed = NAN; // Signed longitude for NMEA
    data.cog = NAN;              // Course over ground
    data.sog = NAN;              // Speed over ground
    data.hdop = NAN;             // Horizontal Dilution of Precision
    data.pdop = NAN;             // Position Dilution of Precision
    data.vdop = NAN;             // Vertical Dilution of Precision
    data.tdop = NAN;             // Time Dilution of Precision
    data.fix = 0;                // GNSS fix type (0=no fix, 1=dead reckoning, 2=2D, 3=3D, 4=GNSS, 5=Time fix)
    data.gps_unix_time = 0;      // in seconds since epoch

    data.nSat = 0;
    data.nUsedSats = 0;
}

bool GPSX::loadSats()
{
    sat *satellites = data.satellites;
    UBX_NAV_SAT_data_t *d = &myGNSS.packetUBXNAVSAT->data;
    int usedSats = 0;
    for (int i = 0; i < d->header.numSvs; i++)
    {
        satellites[i].sat_id = d->blocks[i].svId;
        satellites[i].az = d->blocks[i].azim;
        satellites[i].elev = d->blocks[i].elev;
        satellites[i].db = d->blocks[i].cno;
        satellites[i].used = d->blocks[i].flags.bits.svUsed;

        if (satellites[i].used && usedSats < MAX_USED_SATS_SIZE)
        {
            data.sats[usedSats] = satellites[i].sat_id;
            usedSats++;
            // Log::trace("Sat {%s} PRN {%d} Az {%d} Elev {%d} DB {%d}\n",
            //     SATS_TYPES[d->blocks[i].gnssId], satellites[i].sat_id,
            //     satellites[i].az, satellites[i].elev, satellites[i].db);
        }
    }
    data.nUsedSats = usedSats;
    data.nSat = d->header.numSvs;
    // Log::trace("[GPS] Loaded {%d/%d} sats\n", gsv.nSat, gsa.nSat);
    return true;
}

bool GPSX::loadPVT()
{
    cache_ok = false;
    if (!myGNSS.getPVT())
        return false;

    cache_ok = true;

    data.fix = myGNSS.getFixType();

    if (myGNSS.getTimeFullyResolved())
    {
        uint32_t micros = 0;
        uint32_t time = myGNSS.getUnixEpoch(micros);
        data.gps_unix_time = time;
        data.gps_unix_time_ms = micros / 1000; // convert to milliseconds
    }
    else
    {
        data.gps_unix_time = 0;
        data.gps_unix_time_ms = 0;
    }

    if (myGNSS.getGnssFixOk())
    {
        data.hdop = myGNSS.getHorizontalDOP() / 100.0;
        data.vdop = myGNSS.getVerticalDOP() / 100.0;
        data.tdop = myGNSS.getTimeDOP() / 100.0;
        data.pdop = myGNSS.getPDOP() / 100.0;
        data.latitude_signed = myGNSS.getLatitude() / 10000000.0f;
        data.longitude_signed = myGNSS.getLongitude() / 10000000.0f;
        data.cog = myGNSS.getHeading() / 100000.0f;                         // headVeh is deg*1e-05
        data.sog = (myGNSS.getGroundSpeed() / 1000.0f) * 3600.0f / 1852.0f; // gSpeed is mm/s
        data.fix = myGNSS.getFixType();
    }
    else
    {
        data.hdop = NAN;
        data.vdop = NAN;
        data.tdop = NAN;
        data.pdop = NAN;
        data.gps_unix_time = 0;
        data.latitude_signed = NAN;
        data.longitude_signed = NAN;
        data.cog = NAN;
        data.sog = NAN;
        data.fix = 0; // no fix
    }
    return true;
}

GPSX::GPSX(HardwareSerial *serial, int rx, int tx) : enabled(false), last_read_time(0),
                                                     count_sent(0), myGNSS(), cache_ok(false),
                                                     serial_port(serial), rx_pin(rx), tx_pin(tx)

{
}

GPSX::~GPSX()
{
}

void GPSX::manageLowFrequency(unsigned long micros, Context &ctx)
{
    if (cache_ok && count_sent == 0)
    {
        static N2KSid _sid;
        unsigned char sid = _sid.getNew();
        //if (/*loadFix() && */loadSats())
        {
            ctx.n2k.sendGNSSPosition(data, sid);
        }
        if (ctx.conf.get_services().is_sog_2_stw())
        {
            ctx.n2k.sendSTW(data.sog);
        }
        if (ctx.conf.get_services().is_send_time())
        {
            ctx.n2k.sendSystemTime(data.gps_unix_time, sid, data.gps_unix_time_ms);
        }
#if (SEND_SATS == 1)
        ctx.n2k.sendSatellites(data, sid);
#endif
        //set_system_time(sid, ctx);
    }
}

void GPSX::manageHighFrequency(unsigned long micros, Context &ctx)
{
    // if (loadPVT())
    {
        ctx.n2k.sendCOGSOG(data.sog, data.cog, 0xFF);
        ctx.n2k.sendPosition(data.latitude_signed, data.longitude_signed);
    }
    count_sent++;
    count_sent %= HIG_LOW_FREQ_RATIO;
}

void GPSX::loop(unsigned long micros, Context &ctx)
{
    if (enabled)
    {
        if (loadPVT())
        {
            data.serial++;
            ctx.data_cache.gps = data;
            manageHighFrequency(micros, ctx);
            manageLowFrequency(micros, ctx);
        }
    }
    else if (ctx.data_cache.gps.serial != data.serial)
    {
        ctx.data_cache.gps = data;
    }
}

void GPSX::setup(Context &ctx)
{
    Log::tracex(GPS_LOG_TAG, "Setup", "Initializing GPS module");
}

bool GPSX::is_enabled()
{
    return enabled;
}

void GPSX::enable()
{
    if (!enabled)
    {
        if (serial_port == nullptr)
        {
            Log::tracex(GPS_LOG_TAG, "Enabling", "Type {%s}", "I2C");
            bool _enabled = myGNSS.begin(*TwoWireProvider::get_two_wire(), 0x42, 1100U, false);
            if (_enabled)
            {
                myGNSS.setI2COutput(COM_TYPE_UBX); // Set the I2C port to output UBX only (turn off NMEA noise)
                myGNSS.setNavigationFrequency(NAVIGATION_DATA_FREQUENCY);
                myGNSS.setAutoPVT(true);
                myGNSS.setAutoDOP(true);
                myGNSS.setAutoNAVSAT(true);
            }
            enabled = _enabled;
            Log::tracex(GPS_LOG_TAG, "Enable", "Success {%d}", enabled);
        }
        else
        {
            Log::tracex(GPS_LOG_TAG, "Enabling", "Type {%s}", "Serial");
            bool _enabled = false;
            int retry_count = 0;
            while (retry_count < 3)
            {
                Log::tracex(GPS_LOG_TAG, "Enabling", "Trying %d baud", GPS_SERIAL_SPEED);
                serial_port->end();
                serial_port->begin(GPS_SERIAL_SPEED, SERIAL_8N1, rx_pin, tx_pin); // RX, TX
                if (myGNSS.begin(*serial_port))
                {
                    _enabled = true;
                    Log::tracex(GPS_LOG_TAG, "Enabling", "Connected at %d baud", GPS_SERIAL_SPEED);
                    break;
                }
                else
                {
                    Log::tracex(GPS_LOG_TAG, "Enabling", "Connection failed at %d baud, trying 9600\n", GPS_SERIAL_SPEED);
                    serial_port->end();
                    serial_port->begin(9600, SERIAL_8N1, rx_pin, tx_pin);
                    if (myGNSS.begin(*serial_port))
                    {
                        myGNSS.setSerialRate(GPS_SERIAL_SPEED); // Set the serial port to 57600 baud)
                        myGNSS.saveConfiguration();             // Save the new baud rate to flash and BBR
                        myGNSS.end();
                        serial_port->end();
                        Log::tracex(GPS_LOG_TAG, "Enabling", "Port speeed set to %d baud\n", GPS_SERIAL_SPEED);
                    }
                }
                retry_count++;
            }

            if (_enabled)
            {
                myGNSS.setUART1Output(COM_TYPE_UBX); // Set the port to output UBX only (turn off NMEA noise)
                myGNSS.setNavigationFrequency(NAVIGATION_DATA_FREQUENCY);
                myGNSS.setAutoPVT(true);
                myGNSS.setAutoDOP(true);
                myGNSS.setAutoNAVSAT(true);
            }
            enabled = _enabled;
            Log::tracex(GPS_LOG_TAG, "Enable", "Success {%d}", enabled);
        }
    }
}

void GPSX::disable()
{
    if (enabled)
    {
        enabled = false;

        reset_gps_data(data);

        myGNSS.end();

        Log::tracex(GPS_LOG_TAG, "Disable", "Success {%d}", !enabled);
    }
}

void GPSX::dumpStats()
{
    if (enabled)
    {
        Log::tracex(GPS_LOG_TAG, "Stats", "UTC {%s} Pos {%.4f %.4f} SOG {%.2f} COG {%.2f} Sats {%d/%d} hDOP {%.2f} pDOP {%.2f} Fix {%d}",
                    time_to_ISO(data.gps_unix_time, data.gps_unix_time_ms),
                    data.latitude_signed, data.longitude_signed, data.sog, data.cog,
                    data.nSat, data.nSat, data.hdop, data.pdop, data.fix);
    }
}

#endif
