#include "Constants.h"
char socket_name[32];

#ifdef ESP32_ARCH
#define ESP32_CAN_TX_PIN GPIO_NUM_5 // Set CAN TX port to 5
#define ESP32_CAN_RX_PIN GPIO_NUM_4 // Set CAN RX port to 4
#else
#define SOCKET_CAN_PORT socket_name
// #define SOCKET_CAN_PORT "vcan0"
#endif
#include <NMEA2000_CAN.h>

#include <time.h>
#include <math.h>
#include "N2K.h"
#include "Utils.h"
#include "Log.h"

ulong *_can_received;
void (*_handler)(const tN2kMsg &N2kMsg);

inline int get_n2k_src()
{
    return DEFAULT_N2K_SRC;
}

#define N2KSRC get_n2k_src()

N2K::N2K(void (*_msg_handler)(const tN2kMsg &N2kMsg), statistics *s) : stats(s), initialized(false)
{
    _handler = _msg_handler;
    _can_received = &(s->can_received);
}

void private_message_handler(const tN2kMsg &N2kMsg)
{
    (*_can_received)++;
    _handler(N2kMsg);
}

void N2K::loop(unsigned long time)
{
    NMEA2000.ParseMessages();
}

bool N2K::sendMessage(int dest, ulong pgn, int priority, int len, unsigned char *payload)
{
    tN2kMsg m(N2KSRC);
    m.Init(priority, pgn, N2KSRC, dest);
    for (int i = 0; i < len; i++)
        m.AddByte(payload[i]);
    return send_msg(m);
}

bool N2K::send126996Request(int dst)
{
    tN2kMsg N2kMsg(N2KSRC);
    SetN2kPGNISORequest(N2kMsg, dst, 126996);
    return send_msg(N2kMsg);
}

void N2K::set_can_socket_name(const char* name)
{
    strcpy(socket_name, name);
}

void N2K::setup()
{
    if (!initialized)
    {
        Log::trace("[N2k] Initializing N2K\n");
        NMEA2000.SetN2kCANSendFrameBufSize(3);
        NMEA2000.SetN2kCANReceiveFrameBufSize(150);
        NMEA2000.SetN2kCANMsgBufSize(15);
        NMEA2000.SetProductInformation("00000001",                         // Manufacturer's Model serial code
                                       100,                                // Manufacturer's product code
                                       "ABN2k                           ", // Manufacturer's Model ID
                                       "1.0.3.0 (2024-01-07)",             // Manufacturer's Software version code
                                       "1.0.2.0 (2019-07-07)"              // Manufacturer's Model version
        );
        NMEA2000.SetDeviceInformation(1,   // Unique number. Use e.g. Serial number.
                                      145, // Device function=Analog to NMEA 2000 Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                      60,  // Device class=Inter/Intranetwork Device. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                      2046 // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
        );
        if (_handler)
        {
            NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, N2KSRC);
            NMEA2000.SetMsgHandler(private_message_handler);
        }
        else
        {
            Log::trace("[N2K] Initializing node-only\n");
            NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly, N2KSRC);
        }
        NMEA2000.EnableForward(false); // Disable all msg forwarding to USB (=Serial)
        initialized = NMEA2000.Open();
        Log::trace("[N2K] N2K initialized {%s}\n", initialized ? "OK" : "KO");
    }
}

bool N2K::send_msg(const tN2kMsg &N2kMsg)
{
    if (initialized)
    {
        if (_handler)
            _handler(N2kMsg);
        if (NMEA2000.SendMsg(N2kMsg))
        {
            stats->can_sent++;
            return true;
        }
        else
        {
            Log::trace("[N2K] Failed message {%d}\n", N2kMsg.PGN);
            stats->can_failed++;
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool N2K::sendCOGSOG(GSA &gsa, RMC &rmc, int sid)
{
    if (/*gsa.valid && */rmc.valid/* && gsa.fix >= 2*/)
    {
        if (isnan(rmc.sog) && isnan(rmc.cog))
            return false;
        else
        {
            tN2kMsg N2kMsg(N2KSRC);
            SetN2kCOGSOGRapid(N2kMsg, sid, N2khr_true, DegToRad(isnan(rmc.cog) ? 0.0 : rmc.cog), rmc.sog * 1852.0 / 3600);
            return send_msg(N2kMsg);
        }
    }
    return false;
}

bool N2K::sendGNNSStatus(GSA &gsa, int sid)
{
    if (gsa.valid)
    {
        tN2kMsg N2kMsg(N2KSRC);
        tN2kGNSSDOPmode mode;
        switch (gsa.fix)
        {
        case 3:
            mode = tN2kGNSSDOPmode::N2kGNSSdm_3D;
            break;
        case 2:
            mode = tN2kGNSSDOPmode::N2kGNSSdm_2D;
            break;
        case 1:
            mode = tN2kGNSSDOPmode::N2kGNSSdm_1D;
            break;
        default:
            mode = tN2kGNSSDOPmode::N2kGNSSdm_Unavailable;
        }
        N2kMsg.SetPGN(129539L);
        N2kMsg.Priority = 6;
        N2kMsg.AddByte(sid);
        N2kMsg.AddByte(tN2kGNSSDOPmode::N2kGNSSdm_2D | ((mode & 0x07) << 3));
        N2kMsg.Add2ByteDouble(gsa.hdop, 0.01);
        N2kMsg.Add2ByteDouble(gsa.vdop, 0.01);
        N2kMsg.Add2ByteDouble(gsa.pdop, 0.01);
        return send_msg(N2kMsg);
    }
    return false;
}

bool N2K::sendGNSSPosition(GSA &gsa, RMC &rmc, int sid)
{
    if (gsa.valid && rmc.valid && gsa.fix >= 2)
    {
        tN2kMsg N2kMsg(N2KSRC);
        int days_since_1970 = getDaysSince1970(rmc.y, rmc.M, rmc.d);
        double second_since_midnight = rmc.h * 60 * 60 + rmc.m * 60 + rmc.s + rmc.ms / 1000.0;
        SetN2kPGN129029(N2kMsg, sid, days_since_1970, second_since_midnight, rmc.lat, rmc.lon, N2kDoubleNA, tN2kGNSStype::N2kGNSSt_GPS,
                        tN2kGNSSmethod::N2kGNSSm_GNSSfix, gsa.nSat, gsa.hdop);
        return send_msg(N2kMsg);
    }
    return false;
}

bool N2K::sendTime(RMC &rmc, int sid)
{
    if (rmc.valid)
    {
        tN2kMsg N2kMsg(N2KSRC);
        int days_since_1970 = getDaysSince1970(rmc.y, rmc.M, rmc.d);
        double second_since_midnight = rmc.h * 60 * 60 + rmc.m * 60 + rmc.s + rmc.ms / 1000.0;
        SetN2kSystemTime(N2kMsg, sid, days_since_1970, second_since_midnight);
        return send_msg(N2kMsg);
    }
    return false;
}

bool N2K::sendLocalTime(GSA &gsa, RMC &rmc)
{
    if (gsa.valid && rmc.valid && gsa.fix >= 2)
    {
        tN2kMsg N2kMsg(N2KSRC);
        int days_since_1970 = getDaysSince1970(rmc.y, rmc.M, rmc.d);
        double second_since_midnight = rmc.h * 60 * 60 + rmc.m * 60 + rmc.s + rmc.ms / 1000.0;
        SetN2kLocalOffset(N2kMsg, days_since_1970, second_since_midnight, 0);
        return send_msg(N2kMsg);
    }
    return false;
}

bool N2K::sendTime(time_t _now, int sid, short ms)
{
    tm *t = gmtime(&_now);
    int days_since_1970 = getDaysSince1970(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    double second_since_midnight = t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec * (double)ms / 1000.0;
    tN2kMsg N2kMsg(N2KSRC);
    SetN2kSystemTime(N2kMsg, sid, days_since_1970, second_since_midnight);
    return send_msg(N2kMsg);
}

bool N2K::sendPosition(GSA &gsa, RMC &rmc)
{
    if (/*gsa.valid && */rmc.valid/* && gsa.fix >= 2*/)
    {
        tN2kMsg N2kMsg(N2KSRC);
        SetN2kPGN129025(N2kMsg, rmc.lat, rmc.lon);
        return send_msg(N2kMsg);
    }
    return false;
}

bool N2K::sendElectronicTemperature(const float temp, int sid)
{
    tN2kMsg N2kMsg(N2KSRC);
    N2kMsg.SetPGN(130312L);
    N2kMsg.Priority = 5;
    N2kMsg.AddByte((unsigned char)sid);
    N2kMsg.AddByte((unsigned char)1);
    N2kMsg.AddByte((unsigned char)32 /* custom defined */);
    N2kMsg.Add2ByteUDouble(CToKelvin(temp), 0.01);
    N2kMsg.Add2ByteUDouble(CToKelvin(60.0), 0.01);
    N2kMsg.AddByte(0xff); // Reserved
    return send_msg(N2kMsg);
}

bool N2K::sendPressure(const float pressurePA, int sid)
{
    tN2kMsg N2kMsg(N2KSRC);
    SetN2kPressure(N2kMsg, sid, 0, tN2kPressureSource::N2kps_Atmospheric, mBarToPascal(pressurePA));
    return send_msg(N2kMsg);
}

bool N2K::sendHumidity(const float humidity, int sid)
{
    tN2kMsg N2kMsg(N2KSRC);
    SetN2kHumidity(N2kMsg, sid, 0, tN2kHumiditySource::N2khs_InsideHumidity, humidity);
    return send_msg(N2kMsg);
}

bool N2K::sendCabinTemp(const float temperature, int sid)
{
    tN2kMsg N2kMsg(N2KSRC);
    SetN2kTemperature(N2kMsg, sid, 0, tN2kTempSource::N2kts_MainCabinTemperature, CToKelvin(temperature));
    return send_msg(N2kMsg);
}

bool N2K::sendSatellites(const sat *sats, uint n, int sid, GSA &gsa)
{
    if (n > 0)
    {
        tN2kMsg m(N2KSRC);
        m.Init(6, 129540, N2KSRC, 255);
        m.AddByte((unsigned char)sid);
        m.AddByte((unsigned char)(3 & 0x04) << 6);
        m.AddByte((unsigned char)(n < 18 ? n : 18));
        // limit to 15 sats so to remain within 232 bytes
        for (int i = 0; i < n && i < 15; i++)
        {
            sat s = sats[i];
            m.AddByte((unsigned char)s.sat_id);
            m.Add2ByteInt((int)(s.elev / 180.0 * M_PI / 0.0001));
            m.Add2ByteInt((int)(s.az / 180.0 * M_PI / 0.0001));
            if (s.db)
                m.Add2ByteUInt((int)(s.db / 0.01));
            else
                m.Add2ByteUInt(N2kUInt16NA);
            m.Add4ByteUInt(N2kInt32NA);
            m.AddByte((s.status & 0x0F) | 0xF0);
        }
        return send_msg(m);
    }
    return false;
}
