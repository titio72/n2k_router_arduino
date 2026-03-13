#include "N2K_router.h"
#include "Data.h"
#include <N2kMessages.h>
#include <N2kMsg.h>

#include <Utils.h>
#include <Log.h>
#include <time.h>
#include <math.h>

#pragma region N2K_Router
N2K_router::N2K_router(n2k_source_change_handler sh)
    : n2k(*N2K::get_instance(nullptr, sh))
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

void N2K_router::setup(Context &ctx)
{
    Log::tracex("N2K", "Setup", "Initializing N2K Router");
    n2k_device_info info;
    info.ModelSerialCode = "0.1.0";
    info.ProductCode = 100;
    info.ModelID = "AB_GPS_METEO";
    info.SwCode = "AB 0.1.0";
    info.ModelVersion = "0001";

    info.UniqueNumber = 1;     // Unique number. Use e.g. Serial number.
    info.DeviceFunction = 145; // Device function=Analog to NMEA 2000 Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
    info.DeviceClass = 60;     // Device class=Inter/Intranetwork Device. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
    info.ManufacturerCode = 2046;   // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf   

    n2k.setup(info);

#ifndef NATIVE
    _send_queue = xQueueCreate(256, sizeof(tN2kMsg));
    xTaskCreatePinnedToCore(n2k_task_fn, "N2KTask", 4096, this, 10, &_n2k_task, 0);
    Log::tracex("N2K", "Setup", "N2K task pinned to core 0");
#endif
}

void N2K_router::loop(unsigned long time, Context &ctx)
{
    n2k.loop(time);
}

void N2K_router::set_desired_source(unsigned char src)
{
    n2k.set_desired_source(src);
}

unsigned char N2K_router::get_source()
{
    return n2k.get_source();
}

bool N2K_router::send_it(tN2kMsg &N2kMsg)
{
#ifndef NATIVE
    if (_send_queue)
    {
        xQueueSend(_send_queue, &N2kMsg, pdMS_TO_TICKS(10));
        return true;
    }
    return false;
#else
    return n2k.send_msg(N2kMsg);
#endif
}

#ifndef NATIVE
void N2K_router::n2k_task_fn(void *param)
{
    N2K_router *self = static_cast<N2K_router *>(param);
    for (;;)
    {
        self->n2k.loop(micros());
        tN2kMsg msg;
        while (xQueueReceive(self->_send_queue, &msg, 0) == pdTRUE)
        {
            self->n2k.send_msg(msg);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
#endif
#pragma endregion

#pragma region N2KSenderAbstract

bool N2KSenderAbstract::sendSTW(double stw)
{
    if (isnan(stw))
        return false;
    else
    {
        tN2kMsg N2kMsg(get_source());
        SetN2kBoatSpeed(N2kMsg, 0, stw * 1852.0 / 3600.0, stw * 1852.0 / 3600.0, tN2kSpeedWaterReferenceType::N2kSWRT_Paddle_wheel);
        return send_it(N2kMsg);
    }
}

bool N2KSenderAbstract::sendCOGSOG(double sog, double cog, unsigned char sid)
{
    if (isnan(sog) && isnan(cog))
        return false;
    else
    {
        tN2kMsg N2kMsg(get_source());
        SetN2kCOGSOGRapid(N2kMsg, sid, N2khr_true, DegToRad(isnan(cog) ? 0.0 : cog), sog * 1852.0 / 3600.0);
        return send_it(N2kMsg);
    }
}

bool N2KSenderAbstract::sendGNNSStatus(const GPSData &data, unsigned char sid)
{
    if (data.isValid())
    {
        tN2kMsg m(get_source());
        tN2kGNSSDOPmode mode;
        switch (data.fix)
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
        SetN2kPGN129539(m, sid, tN2kGNSSDOPmode::N2kGNSSdm_2D, mode, data.hdop, data.vdop, data.tdop);
        return send_it(m);
    }
    return false;
}

bool N2KSenderAbstract::sendGNSSPosition(const GPSData &gps, unsigned char sid)
{
    if (gps.isValid())
    {
        tN2kMsg N2kMsg(get_source());
        int days_since_1970 = gps.gps_unix_time / 86400;
        double second_since_midnight = gps.gps_unix_time % 86400 + gps.gps_unix_time_ms / 1000.0;
        SetN2kPGN129029(N2kMsg, sid, days_since_1970, second_since_midnight, gps.latitude_signed, gps.longitude_signed, N2kDoubleNA, tN2kGNSStype::N2kGNSSt_GPS,
                        tN2kGNSSmethod::N2kGNSSm_GNSSfix, gps.nSat, gps.hdop);
        return send_it(N2kMsg);
    }
    return false;
}

bool N2KSenderAbstract::sendMagneticVariation(double variation, uint16_t days_since_1970)
{
    if (isnan(variation))
        return false;
    else
    {
        tN2kMsg N2kMsg(get_source());
        SetN2kMagneticVariation(N2kMsg, 0, tN2kMagneticVariation::N2kmagvar_WMM2025, days_since_1970, DegToRad(variation));
        return send_it(N2kMsg);
    }
}

bool N2KSenderAbstract::sendSystemTime(uint32_t _now, unsigned char sid, uint16_t _now_ms)
{
    int days_since_1970 = _now / 86400;
    double second_since_midnight =_now % 86400 + _now_ms / 1000.0;
    tN2kMsg N2kMsg(get_source());
    SetN2kSystemTime(N2kMsg, sid, days_since_1970, second_since_midnight);
    return send_it(N2kMsg);
}

bool N2KSenderAbstract::sendPosition(double lat, double lon)
{
    if (isnan(lat) || isnan(lon))
        return false;
    else
    {
        tN2kMsg N2kMsg(get_source());
        SetN2kPGN129025(N2kMsg, lat, lon);
        return send_it(N2kMsg);
    }
}

bool N2KSenderAbstract::sendElectronicTemperature(const double temp, unsigned char sid)
{
    tN2kMsg m(get_source());
    m.SetPGN(130312L);
    m.Priority = 5;
    m.AddByte((unsigned char)sid);
    m.AddByte((unsigned char)1);
    m.AddByte((unsigned char)32 /* custom defined */);
    m.Add2ByteUDouble(CToKelvin(temp), 0.01);
    m.Add2ByteUDouble(CToKelvin(60.0), 0.01);
    m.AddByte(0xff); // Reserved
    return send_it(m);
}

bool N2KSenderAbstract::sendSeaTemperature(const double temp, unsigned char sid)
{
    if (isnan(temp))
        return false;

    tN2kMsg m(get_source());
    SetN2kTemperature(m, sid, 0, tN2kTempSource::N2kts_SeaTemperature, CToKelvin(temp));
    return send_it(m);
}

bool N2KSenderAbstract::sendEnvironmentXRaymarine(const double pressure, const double humidity, const double temperature)
{
    //Log::trace("[N2K] Send {%f} {%f} {%f}\n", CToKelvin(temperature), humidity, pressure);
    tN2kMsg N2kMsg(get_source());
    SetN2kEnvironmentalParameters(N2kMsg, 1,
        tN2kTempSource::N2kts_OutsideTemperature, CToKelvin(temperature),
        tN2kHumiditySource::N2khs_OutsideHumidity, humidity,
        pressure);
    return send_it(N2kMsg);
}

bool N2KSenderAbstract::sendPressure(const double pressurePA, unsigned char sid)
{
    tN2kMsg N2kMsg(get_source());
    SetN2kPressure(N2kMsg, sid, 0, tN2kPressureSource::N2kps_Atmospheric, pressurePA);
    return send_it(N2kMsg);
}

bool N2KSenderAbstract::sendHumidity(const double humidity, unsigned char sid)
{
    tN2kMsg N2kMsg(get_source());
    SetN2kHumidity(N2kMsg, sid, 0, tN2kHumiditySource::N2khs_InsideHumidity, humidity);
    return send_it(N2kMsg);
}

bool N2KSenderAbstract::sendCabinTemp(const double temperature, unsigned char sid)
{
    tN2kMsg N2kMsg(get_source());
    SetN2kTemperature(N2kMsg, sid, 0, tN2kTempSource::N2kts_MainCabinTemperature, CToKelvin(temperature));
    return send_it(N2kMsg);
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

bool N2KSenderAbstract::sendSatellites(const GPSData &data, unsigned char sid)
{
    //Log::trace("[N2K] Sending sats {%d/%d} %ud\n", gsa.nSat, n, sid);
    int satsInMsg = 0;
    int i = 0;
    while (i<data.nSat)
    {
        tN2kMsg m(get_source());
        SetN2kGNSSSatellitesInView(m, sid);
        while (appendSat(m, (data.satellites[i])) && i<data.nSat)
        {
            i++;
            satsInMsg++;
        }
        if (!send_it(m))
        {
            //Log::trace("[N2K] Failed sending sats {%d/%d/%d} %ud\n", satsInMsg, i, data.nSat, sid);
            return false;
        }
        else
        {
            //Log::trace("[N2K] Sent sats {%d/%d/%d} %ud\n", satsInMsg, i, data.nSat, sid);
        }
        satsInMsg = 0;
    }
    return i>0;
}

bool N2KSenderAbstract::sendBattery(unsigned char sid, const double voltage, const double current, const double temperature, const unsigned char instance) {
    tN2kMsg m;
    SetN2kPGN127508(m, instance,
        isnan(voltage)?N2kDoubleNA:voltage, isnan(current)?N2kDoubleNA:current, isnan(temperature)?N2kDoubleNA:(temperature + 273.15), sid);
    return send_it(m);
}

bool N2KSenderAbstract::sendBatteryStatus(unsigned char sid, const double soc, const double capacity, const double ttg, const unsigned char instance) {
    tN2kMsg m;
    SetN2kPGN127506(m, sid, instance, tN2kDCType::N2kDCt_Battery,
        isnan(soc)?N2kDoubleNA:soc, 100, isnan(ttg)?N2kDoubleNA:ttg, N2kDoubleNA, capacity * 3600);
    return send_it(m);
}

bool N2KSenderAbstract::sendEngineRPM(uint8_t instance, uint16_t rpm)
{
    tN2kMsg m;
    SetN2kPGN127488(m, instance, rpm);
    return send_it(m);
}

bool N2KSenderAbstract::sendEngineHours(uint8_t instance, double seconds)
{
    tN2kMsg m;
    SetN2kPGN127489(m, instance, N2kDoubleNA, N2kDoubleNA, N2kDoubleNA, N2kDoubleNA, N2kDoubleNA, seconds, N2kDoubleNA, N2kDoubleNA, 127, 127, 0, 0);
    return send_it(m);
}

bool N2KSenderAbstract::sendMagneticHeading(double heading)
{
    if (isnan(heading))
        return false;
    else
    {
        tN2kMsg N2kMsg(get_source());
        SetN2kMagneticHeading(N2kMsg, 0, DegToRad(heading));
        return send_it(N2kMsg);
    }
}

N2KStats N2K_router::getStats()
{
    return n2k.getStats();
}
#pragma endregion