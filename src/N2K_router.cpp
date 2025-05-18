#include "N2K_router.h"
#include "Data.h"
#include <N2kMessages.h>
#include <N2kMsg.h>

#include <Utils.h>
#include <Log.h>
#include <time.h>
#include <math.h>

N2K_router::N2K_router(N2K &_n2k): n2k(_n2k)
{
    n2k.add_pgn(126992); // System Time
    n2k.add_pgn(129025); // Position
    n2k.add_pgn(129026); // COG & SOG
    n2k.add_pgn(129029); // GNSS Position Data
    n2k.add_pgn(129539); // GNSS DOPs
    n2k.add_pgn(130311); // Environmental Parameters
    n2k.add_pgn(130312); // Temperature
    n2k.add_pgn(130313); // Humidity
    n2k.add_pgn(130314); // Actual Pressure
    n2k.add_pgn(127488); // Engine RPM
    n2k.add_pgn(127489); // Engine hours
}

bool N2K_router::sendMessage(int dest, ulong pgn, int priority, int len, unsigned char *payload)
{
    tN2kMsg m;
    m.Init(priority, pgn, n2k.get_source(), dest);
    for (int i = 0; i < len; i++)
        m.AddByte(payload[i]);
    return n2k.send_msg(m);
}

bool N2K_router::sendCOGSOG(RMC &rmc, unsigned char sid)
{
    if (rmc.valid) return sendCOGSOG(rmc.sog, rmc.cog, sid);
    else return false;
}

bool N2K_router::sendSTW(double stw)
{
    if (isnan(stw))
        return false;
    else
    {
        tN2kMsg N2kMsg(n2k.get_source());
        SetN2kBoatSpeed(N2kMsg, 0, stw * 1852.0 / 3600.0, stw * 1852.0 / 3600.0, tN2kSpeedWaterReferenceType::N2kSWRT_Paddle_wheel);
        return n2k.send_msg(N2kMsg);
    }
}

bool N2K_router::sendCOGSOG(double sog, double cog, unsigned char sid)
{
    if (isnan(sog) && isnan(cog))
        return false;
    else
    {
        tN2kMsg N2kMsg(n2k.get_source());
        SetN2kCOGSOGRapid(N2kMsg, sid, N2khr_true, DegToRad(isnan(cog) ? 0.0 : cog), sog * 1852.0 / 3600.0);
        return n2k.send_msg(N2kMsg);
    }
}

bool N2K_router::sendGNNSStatus(int fix, double hdop, double vdop, double tdop, unsigned char sid)
{
    tN2kMsg m(n2k.get_source());
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
    return n2k.send_msg(m);
}

bool N2K_router::sendGNNSStatus(GSA &gsa, unsigned char sid)
{
    if (gsa.valid)
    {
        return sendGNNSStatus(gsa.fix, gsa.hdop, gsa.vdop, gsa.tdop, sid);
    }
    return false;
}

bool N2K_router::sendGNSSPosition(GSA &gsa, RMC &rmc, unsigned char sid)
{
    if (gsa.valid && rmc.valid && gsa.fix >= 2)
    {
        tN2kMsg N2kMsg(n2k.get_source());
        int days_since_1970 = getDaysSince1970(rmc.y, rmc.M, rmc.d);
        double second_since_midnight = rmc.h * 60 * 60 + rmc.m * 60 + rmc.s + rmc.ms / 1000.0;
        SetN2kPGN129029(N2kMsg, sid, days_since_1970, second_since_midnight, rmc.lat, rmc.lon, N2kDoubleNA, tN2kGNSStype::N2kGNSSt_GPS,
                        tN2kGNSSmethod::N2kGNSSm_GNSSfix, gsa.nSat, gsa.hdop);
        return n2k.send_msg(N2kMsg);
    }
    return false;
}

bool N2K_router::sendGNSSPosition(int y, int M, int d, int h, int m, int s, unsigned long unixTime, double lat, double lon, int nsat, double hdop, double pdop, unsigned char sid)
{
    tN2kMsg N2kMsg(n2k.get_source());
    int days_since_1970 = getDaysSince1970(y, M, d);
    double second_since_midnight = h * 60 * 60 + m * 60 + s / 1000.0;
    SetN2kPGN129029(N2kMsg, sid, days_since_1970, second_since_midnight, lat, lon, N2kDoubleNA, tN2kGNSStype::N2kGNSSt_GPS,
                    tN2kGNSSmethod::N2kGNSSm_GNSSfix, nsat, hdop, pdop);
    return n2k.send_msg(N2kMsg);
}

bool N2K_router::sendSystemTime(RMC &rmc, unsigned char sid)
{
    if (rmc.valid)
    {
        tN2kMsg N2kMsg(n2k.get_source());
        int days_since_1970 = getDaysSince1970(rmc.y, rmc.M, rmc.d);
        double second_since_midnight = rmc.h * 60 * 60 + rmc.m * 60 + rmc.s + rmc.ms / 1000.0;
        SetN2kSystemTime(N2kMsg, sid, days_since_1970, second_since_midnight);
        return n2k.send_msg(N2kMsg);
    }
    return false;
}

bool N2K_router::sendLocalTime(GSA &gsa, RMC &rmc)
{
    if (gsa.valid && rmc.valid && gsa.fix >= 2)
    {
        tN2kMsg N2kMsg(n2k.get_source());
        int days_since_1970 = getDaysSince1970(rmc.y, rmc.M, rmc.d);
        double second_since_midnight = rmc.h * 60 * 60 + rmc.m * 60 + rmc.s + rmc.ms / 1000.0;
        SetN2kLocalOffset(N2kMsg, days_since_1970, second_since_midnight, 0);
        return n2k.send_msg(N2kMsg);
    }
    return false;
}

bool N2K_router::sendSystemTime(time_t _now, unsigned char sid, short ms)
{
    tm *t = gmtime(&_now);
    int days_since_1970 = getDaysSince1970(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    double second_since_midnight = t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec * (double)ms / 1000.0;
    tN2kMsg N2kMsg(n2k.get_source());
    SetN2kSystemTime(N2kMsg, sid, days_since_1970, second_since_midnight);
    return n2k.send_msg(N2kMsg);
}

bool N2K_router::sendPosition(RMC &rmc)
{
    if (rmc.valid)
    {
        return sendPosition(rmc.lat, rmc.lon);
    }
    return false;
}

bool N2K_router::sendPosition(double lat, double lon)
{
    if (isnan(lat) || isnan(lon))
        return false;
    else
    {
        tN2kMsg N2kMsg(n2k.get_source());
        SetN2kPGN129025(N2kMsg, lat, lon);
        return n2k.send_msg(N2kMsg);
    }
}

bool N2K_router::sendElectronicTemperature(const double temp, unsigned char sid)
{
    tN2kMsg m(n2k.get_source());
    m.SetPGN(130312L);
    m.Priority = 5;
    m.AddByte((unsigned char)sid);
    m.AddByte((unsigned char)1);
    m.AddByte((unsigned char)32 /* custom defined */);
    m.Add2ByteUDouble(CToKelvin(temp), 0.01);
    m.Add2ByteUDouble(CToKelvin(60.0), 0.01);
    m.AddByte(0xff); // Reserved
    return n2k.send_msg(m);
}

bool N2K_router::sendEnvironmentXRaymarine(const double pressure, const double humidity, const double temperature)
{
    //Log::trace("[N2K] Send {%f} {%f} {%f}\n", CToKelvin(temperature), humidity, pressure);
    tN2kMsg N2kMsg(n2k.get_source());
    SetN2kEnvironmentalParameters(N2kMsg, 1,
        tN2kTempSource::N2kts_OutsideTemperature, CToKelvin(temperature),
        tN2kHumiditySource::N2khs_OutsideHumidity, humidity,
        pressure);
    return n2k.send_msg(N2kMsg);
}

bool N2K_router::sendPressure(const double pressurePA, unsigned char sid)
{
    tN2kMsg N2kMsg(n2k.get_source());
    SetN2kPressure(N2kMsg, sid, 0, tN2kPressureSource::N2kps_Atmospheric, pressurePA);
    return n2k.send_msg(N2kMsg);
}

bool N2K_router::sendHumidity(const double humidity, unsigned char sid)
{
    tN2kMsg N2kMsg(n2k.get_source());
    SetN2kHumidity(N2kMsg, sid, 0, tN2kHumiditySource::N2khs_InsideHumidity, humidity);
    return n2k.send_msg(N2kMsg);
}

bool N2K_router::sendCabinTemp(const double temperature, unsigned char sid)
{
    tN2kMsg N2kMsg(n2k.get_source());
    SetN2kTemperature(N2kMsg, sid, 0, tN2kTempSource::N2kts_MainCabinTemperature, CToKelvin(temperature));
    return n2k.send_msg(N2kMsg);
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

bool N2K_router::sendSatellites(const sat *sats, uint n, unsigned char sid, GSA &gsa)
{
    //Log::trace("[N2K] Sending sats {%d/%d} %ud\n", gsa.nSat, n, sid);
    int satsInMsg = 0;
    int i = 0;
    while (i<n)
    {
        tN2kMsg m(n2k.get_source());
        SetN2kGNSSSatellitesInView(m, sid);
        while (appendSat(m, (sats[i])) && i<n)
        {
            i++;
            satsInMsg++;
        }
        if (!n2k.send_msg(m))
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

bool N2K_router::sendBattery(unsigned char sid, const double voltage, const double current, const double temperature, const unsigned char instance) {
    tN2kMsg m;
    SetN2kPGN127508(m, instance,
        isnan(voltage)?N2kDoubleNA:voltage, isnan(current)?N2kDoubleNA:current, isnan(temperature)?N2kDoubleNA:(temperature + 273.15), sid);
    return n2k.send_msg(m);
}

bool N2K_router::sendBatteryStatus(unsigned char sid, const double soc, const double capacity, const double ttg, const unsigned char instance) {
    tN2kMsg m;
    SetN2kPGN127506(m, sid, instance, tN2kDCType::N2kDCt_Battery,
        isnan(soc)?N2kDoubleNA:soc, 100, isnan(ttg)?N2kDoubleNA:ttg, N2kDoubleNA, capacity * 3600);
    return n2k.send_msg(m);
}

bool N2K_router::sendEngineRPM(uint8_t instance, uint16_t rpm)
{
    tN2kMsg m;
    SetN2kPGN127488(m, instance, rpm);
    return n2k.send_msg(m);
}

bool N2K_router::sendEngineHours(uint8_t instance, double seconds)
{
    tN2kMsg m;
    SetN2kPGN127489(m, instance, N2kDoubleNA, N2kDoubleNA, N2kDoubleNA, N2kDoubleNA, N2kDoubleNA, seconds, N2kDoubleNA, N2kDoubleNA, 127, 127, 0, 0);
    return n2k.send_msg(m);
}

N2K& N2K_router::get_bus()
{
    return n2k;
}
