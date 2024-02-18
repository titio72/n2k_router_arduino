#include "Constants.h"
char socket_name[32];
#define ESP32_CAN_TX_PIN GPIO_NUM_4 // Set CAN TX port to 5
#define ESP32_CAN_RX_PIN GPIO_NUM_5 // Set CAN RX port to 4
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
    if (initialized)
    {
        NMEA2000.ParseMessages();
    }
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
        //if (_handler)
        //{
        //    NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, N2KSRC);
        //    NMEA2000.SetMsgHandler(private_message_handler);
        //}
        //else
        //{
        //    Log::trace("[N2K] Initializing node-only\n");
            NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly, N2KSRC);
        //}
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

bool N2K::sendCOGSOG(RMC &rmc, unsigned char sid)
{
    if (rmc.valid) return sendCOGSOG(rmc.sog, rmc.cog, sid);
    else return false;
}

bool N2K::sendCOGSOG(double sog, double cog, unsigned char sid)
{
    if (isnan(sog) && isnan(cog))
        return false;
    else
    {
        tN2kMsg N2kMsg(N2KSRC);
        SetN2kCOGSOGRapid(N2kMsg, sid, N2khr_true, DegToRad(isnan(cog) ? 0.0 : cog), sog * 1852.0 / 3600);
        return send_msg(N2kMsg);
    }
}

bool N2K::sendGNNSStatus(int fix, float hdop, float vdop, float tdop, unsigned char sid)
{
    tN2kMsg m(N2KSRC);
    tN2kGNSSDOPmode mode;
    switch (fix)
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
    SetN2kPGN129539(m, sid, tN2kGNSSDOPmode::N2kGNSSdm_2D, mode, hdop, vdop, tdop);
    return send_msg(m);
}

bool N2K::sendGNNSStatus(GSA &gsa, unsigned char sid)
{
    if (gsa.valid)
    {
        return sendGNNSStatus(gsa.fix, gsa.hdop, gsa.vdop, gsa.tdop, sid);
    }
    return false;
}

bool N2K::sendGNSSPosition(GSA &gsa, RMC &rmc, unsigned char sid)
{
    if (gsa.valid && rmc.valid && gsa.fix >= 2)
    {
        tN2kMsg N2kMsg(N2KSRC);
        int days_since_1970 = getDaysSince1970(rmc.y, rmc.M, rmc.d);
        double second_since_midnight = rmc.h * 60 * 60 + rmc.m * 60 + rmc.s + rmc.ms / 1000.0;
        SetN2kPGN129029(N2kMsg, sid, days_since_1970, second_since_midnight, rmc.lat, rmc.lon, N2kDoubleNA, tN2kGNSStype::N2kGNSSt_GPS,
                        tN2kGNSSmethod::N2kGNSSm_GNSSfix, gsa.nSat, gsa.hdop);
        bool res = send_msg(N2kMsg);
        if (!res)
            Log::trace("[N2K] GNSSPosition {%d} {%04d-%02d-%02dT%02d:%02d:%02d Lat %.4f Lon %.4f SOG %.2f COG %.2f Sat %d hDOP %.2f pDOP %.2f\n",
                res, rmc.y, rmc.M, rmc.d, rmc.h, rmc.m, rmc.s, rmc.lat, rmc.lon, rmc.sog, rmc.cog, gsa.nSat, gsa.hdop, gsa.pdop);
        return res;
    }
    return false;
}

bool N2K::sendGNSSPosition(int y, int M, int d, int h, int m, int s, unsigned long unixTime, float lat, float lon, int nsat, float hdop, float pdop, unsigned char sid)
{
    tN2kMsg N2kMsg(N2KSRC);
    int days_since_1970 = getDaysSince1970(y, M, d);
    double second_since_midnight = h * 60 * 60 + m * 60 + s / 1000.0;
    SetN2kPGN129029(N2kMsg, sid, days_since_1970, second_since_midnight, lat, lon, N2kDoubleNA, tN2kGNSStype::N2kGNSSt_GPS,
                    tN2kGNSSmethod::N2kGNSSm_GNSSfix, nsat, hdop, pdop);
    bool res = send_msg(N2kMsg);
    Log::trace("[N2K] GNSSPosition {%d} {%04d-%02d-%02dT%02d:%02d:%02d Lat %.4f Lon %.4f SOG %.2f COG %.2f Sat %d hDOP %.2f pDOP %.2f\n", 
        res, y, M, d, h, m, s, lat, lon, nsat, hdop, pdop);
    return res;
}

bool N2K::sendTime(RMC &rmc, unsigned char sid)
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

bool N2K::sendTime(time_t _now, unsigned char sid, short ms)
{
    tm *t = gmtime(&_now);
    int days_since_1970 = getDaysSince1970(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    double second_since_midnight = t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec * (double)ms / 1000.0;
    tN2kMsg N2kMsg(N2KSRC);
    SetN2kSystemTime(N2kMsg, sid, days_since_1970, second_since_midnight);
    return send_msg(N2kMsg);
}

bool N2K::sendPosition(RMC &rmc)
{
    if (rmc.valid)
    {
        return sendPosition(rmc.lat, rmc.lon);
    }
    return false;
}

bool N2K::sendPosition(double lat, double lon)
{
    if (isnan(lat) || isnan(lon))
        return false;
    else
    {
        tN2kMsg N2kMsg(N2KSRC);
        SetN2kPGN129025(N2kMsg, lat, lon);
        return send_msg(N2kMsg);
    }
}

bool N2K::sendElectronicTemperature(const float temp, unsigned char sid)
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

bool N2K::sendPressure(const float pressurePA, unsigned char sid)
{
    tN2kMsg N2kMsg(N2KSRC);
    SetN2kPressure(N2kMsg, sid, 0, tN2kPressureSource::N2kps_Atmospheric, mBarToPascal(pressurePA));
    return send_msg(N2kMsg);
}

bool N2K::sendHumidity(const float humidity, unsigned char sid)
{
    tN2kMsg N2kMsg(N2KSRC);
    SetN2kHumidity(N2kMsg, sid, 0, tN2kHumiditySource::N2khs_InsideHumidity, humidity);
    return send_msg(N2kMsg);
}

bool N2K::sendCabinTemp(const float temperature, unsigned char sid)
{
    tN2kMsg N2kMsg(N2KSRC);
    SetN2kTemperature(N2kMsg, sid, 0, tN2kTempSource::N2kts_MainCabinTemperature, CToKelvin(temperature));
    return send_msg(N2kMsg);
}

bool appendSatTo129540(tN2kMsg& N2kMsg, const tSatelliteInfo& SatelliteInfo) {
  int Index = 2;
  uint8_t NumberOfSVs=N2kMsg.GetByte(Index);
  NumberOfSVs++;
  Index=2;
  N2kMsg.SetByte(NumberOfSVs, Index);  // increment the number satellites
  // add the new satellite info
  N2kMsg.AddByte(SatelliteInfo.PRN);
  N2kMsg.Add2ByteDouble(SatelliteInfo.Elevation,1e-4L);
  N2kMsg.Add2ByteUDouble(SatelliteInfo.Azimuth,1e-4L);
  N2kMsg.Add2ByteDouble(SatelliteInfo.SNR,1e-2L);
  N2kMsg.Add4ByteDouble(SatelliteInfo.RangeResiduals,1e-5L);
  N2kMsg.AddByte(0xf0 | SatelliteInfo.UsageStatus);
  return true;
}

bool appendSat(tN2kMsg& m, const sat& s)
{
    //Log::trace("[N2K] Adding sat PRN {%d} Az {%d} El {%d} db {%d} Used {%d}\n", s.sat_id, s.az, s.elev, s.db, s.used);
    tSatelliteInfo info;
    info.PRN = s.sat_id;
    info.Azimuth = DegToRad(s.az);
    info.Elevation = DegToRad(s.elev);
    if (s.db) info.SNR = (s.db==-1)?N2kDoubleNA:s.db;
    if (s.db==-1)
    {
        info.UsageStatus = tN2kPRNUsageStatus::N2kDD124_NotTracked;
    }
    else
    {
        if (s.used)
            info.UsageStatus = tN2kPRNUsageStatus::N2kDD124_UsedInSolutionWithoutDifferentialCorrections;
        else
            info.UsageStatus = tN2kPRNUsageStatus::N2kDD124_TrackedButNotUsedInSolution;
    }
    return AppendN2kPGN129540(m, info);
}

bool N2K::sendSatellites(const sat *sats, uint n, unsigned char sid, GSA &gsa)
{
    //Log::trace("[N2K] Sending sats {%d/%d} %ud\n", gsa.nSat, n, sid);
    int satsInMsg = 0;
    int i = 0;
    while (i<n)
    {
        tN2kMsg m(N2KSRC);
        //m.SetIsTPMessage(true);
        SetN2kGNSSSatellitesInView(m, sid);
        while (appendSat(m, (sats[i])) && i<n)
        {
            i++;
            satsInMsg++;
        }
        if (!send_msg(m))
        {
            //Log::trace("[N2K] Failed sending sats {%d/%d/%d} %ud\n", satsInMsg, i, n, sid);
            return false;
        }
        else
        {
            //Log::trace("[N2K] Sent sats {%d/%d/%d} %ud\n", satsInMsg, i, n, sid);
        }
        satsInMsg = 0;
    }
    return i>0;
}

void N2K::dumpStats()
{
    Log::trace("[STATS] Bus: CAN.TX {%d/%d} CAN.RX {%d} UART {%d B} in 10s\n",
            stats->can_sent, stats->can_failed, stats->can_received, stats->bytes_uart);
}