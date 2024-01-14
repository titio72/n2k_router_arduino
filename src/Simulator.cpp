#include "Simulator.h"
#include "Conf.h"
#include "Utils.h"
#include "N2K.h"
#include <math.h>
#include <N2kMessages.h>
#include <time.h>
#include <stdio.h>

const double R = 6371008.8; // m

Simulator::Simulator(Context _ctx): ctx(_ctx), enabled(false)
{}

Simulator::~Simulator()
{}

void Simulator::enable()
{
    enabled = true;
}

void Simulator::disable()
{
    enabled = false;
}

double distance(double lat1, double lon1, double lat2, double lon2)
{
    static const double R = 6371008.8; // metres
    double dLat = (lat2-lat1);
    double dLon = (lon2-lon1);

    double a = sin(dLat/2) * (dLat/2) +
        cos(lat1) * cos(lat2) *
        sin(dLon/2) * sin(dLon/2);
    double c = 2.0 * atan2(sqrt(a), sqrt(1-a));

    return R * c;
}

double bearing(double lat1, double lon1, double lat2, double lon2) {
    double y = sin(lon2 - lon1) * cos(lat2);
    double x = cos(lat1) * sin(lat2) -
            sin(lat1) * cos(lat2) * cos(lon2 - lon1);
    return atan2(y, x);
}

void calc(double lat, double lon, double brng, double d, double* lat1, double* lon1) {
    *lat1 = asin(
        sin(lat) * cos(d/R) +
        cos(lat) * sin(d/R) * cos(brng));
    *lon1 = lon + atan2(
        sin(brng) * sin(d/R) * cos(lat),
        cos(d/R) - sin(lat) * sin(*lat1));
}

inline double to_degrees(double radians) {
    return radians * 180.0 / M_PI;
}

inline double to_radians(double degrees) {
    return degrees / 180.0 * M_PI;
}

inline double to_meters_per_second(double kn) {
    return kn / 3.6 * 1.852;
}

inline double to_knots(double ms) {
    return ms * 3.6 / 1.852;
}

void Simulator::loop(unsigned long ms) {

    unsigned long t = _millis();
    if (t0==0) t0 = t;
    if (t0_0100==0) t0_0100 = t;
    if (t0_0250==0) t0_0250 = t;
    if (t0_1000==0) t0_1000 = t;

    bool expired_0100 = ((t-t0_0100)>=100); if (expired_0100) t0_0100 = t;
    bool expired_0250 = ((t-t0_0250)>=250); if (expired_0250) t0_0250 = t;
    bool expired_1000 = ((t-t0_1000)>=1000); if (expired_1000) t0_1000 = t;

    if (expired_0100 || expired_0250 || expired_1000)  {

        unsigned long dT = (t-t0);
        t0 = t;

        float random = (float)rand() / RAND_MAX;
        double _speed = speed + 1.0 * (random - 0.5);
        double _heading = heading + 3.0 * (random - 0.5);
        double _wind_speed = wind_speed + 2.0 * (random - 0.5);
        double _wind_direction = wind_direction + 5.0 * (random - 0.5);

        double _lat, _lon;
        calc(to_radians(lat), to_radians(lon), to_radians(_heading), to_meters_per_second(_speed) * (dT / 1000.0), &_lat, &_lon);
        lat = to_degrees(_lat);
        lon = to_degrees(_lon);

        double _dtw = distance(_lat, _lon, to_radians(dest_lat), to_radians(dest_lon)); // in m
        double _btw = bearing(_lat, _lon, to_radians(dest_lat), to_radians(dest_lon)); // in radians
        double speedToW = to_meters_per_second(_speed) * cos(to_radians(_heading) - _btw); // in m/s

        double etaTime = N2kDoubleNA;
        int etaDate = N2kInt16NA;

        if (enabled)
        {
            if (speedToW>0.2) {
                double _timeToDest = _dtw/speedToW + time(0);
                etaDate = (int)(_timeToDest / (60 * 60 * 24));
                etaTime = _timeToDest - (etaDate * 60 * 60 * 24);
            }

            if (ctx.conf.sim_attitude    && expired_0250) {
                int d = ((int)(_wind_direction - _heading + 360.0) % 360);
                double roll = d>195 ? 15 : (d<175 ? -15 : 0);
                tN2kMsg msg(22);
                SetN2kAttitude(msg, 0, to_radians(_heading), 0.0, to_radians(roll));
                ctx.n2k.send_msg(msg);
            }
            if (ctx.conf.sim_nav         && expired_1000) {
                tN2kMsg N2KNav(22);
                SetN2kPGN129284(N2KNav, 0, _dtw, tN2kHeadingReference::N2khr_true, false, false, tN2kDistanceCalculationType::N2kdct_GreatCircle,
                etaTime, etaDate, N2kDoubleNA, _btw, N2kInt8NA, N2kInt8NA, dest_lat, dest_lon, speedToW);
                ctx.n2k.send_msg(N2KNav);
            }
            if (ctx.conf.sim_sogcog      && expired_0250) {
                tN2kMsg n2kSOGCOG(22); SetN2kCOGSOGRapid(n2kSOGCOG, 0, N2khr_true, to_radians(_heading), to_meters_per_second(_speed)); ctx.n2k.send_msg(n2kSOGCOG);
            }
            if (ctx.conf.sim_position    && expired_0100) {
                tN2kMsg N2kPosition(22); SetN2kPGN129025(N2kPosition, lat, lon); ctx.n2k.send_msg(N2kPosition);
            }
            if (ctx.conf.sim_position && ctx.conf.sim_sogcog
                                && expired_1000) {
                tN2kMsg N2kMsg(22);
                time_t t = time(0);
                int days_since_1970 = t / 86400L;
                double second_since_midnight = t - (days_since_1970 * 86400);
                SetN2kPGN129029(N2kMsg, 0, days_since_1970, second_since_midnight, lat, lon, N2kDoubleNA, tN2kGNSStype::N2kGNSSt_GPS,
                    tN2kGNSSmethod::N2kGNSSm_GNSSfix, 6, 0.95);
                ctx.n2k.send_msg(N2kMsg);

                tN2kMsg N2kMsgTime(22);
                SetN2kPGN126992(N2kMsgTime, 0, days_since_1970, second_since_midnight, tN2kTimeSource::N2ktimes_GPS);
                ctx.n2k.send_msg(N2kMsgTime);

            }
            if (ctx.conf.sim_heading     && expired_0100) {
                tN2kMsg N2KTrueHeading(22); SetN2kTrueHeading(N2KTrueHeading, 0, to_radians(_heading)); ctx.n2k.send_msg(N2KTrueHeading);
                tN2kMsg N2KMagnHeading(22); SetN2kMagneticHeading(N2KMagnHeading, 0, to_radians(_heading)); ctx.n2k.send_msg(N2KMagnHeading);
            }
            if (ctx.conf.sim_speed       && expired_1000) {
                tN2kMsg N2KSpeed(22); SetN2kPGN128259(N2KSpeed, 0, to_meters_per_second(_speed), N2kDoubleNA, tN2kSpeedWaterReferenceType::N2kSWRT_Paddle_wheel); ctx.n2k.send_msg(N2KSpeed);
            }
            if (ctx.conf.sim_temperature && expired_1000) {
                ctx.n2k.sendCabinTemp(23.3, 0);
            }
            if (ctx.conf.sim_water_temperature
                                && expired_1000) {
                tN2kMsg N2kMsg(22);
                SetN2kTemperature(N2kMsg, 0, 0, tN2kTempSource::N2kts_SeaTemperature, CToKelvin(19.2));
                ctx.n2k.send_msg(N2kMsg);
            }
            if (ctx.conf.sim_pressure    && expired_1000) {
                ctx.n2k.sendPressure(1021.2, 0);
            }
            if (ctx.conf.sim_wind        && expired_0250) {
                tN2kMsg windMsg(22);
                double wd = _wind_direction - _heading;
                if (wd<0) wd += 360;
                SetN2kWindSpeed(windMsg, 0, to_meters_per_second(_wind_speed), to_radians(wd), tN2kWindReference::N2kWind_Apparent);
                ctx.n2k.send_msg(windMsg);
            }
            if (ctx.conf.sim_humidity    && expired_1000) {
                tN2kMsg N2kMsg(22);
                SetN2kHumidity(N2kMsg, 0, 0, tN2kHumiditySource::N2khs_InsideHumidity, 68.1);
                ctx.n2k.send_msg(N2kMsg);
            }
            if (ctx.conf.sim_depth       && expired_1000) {
                tN2kMsg N2kMsg(22);
                SetN2kWaterDepth(N2kMsg, 0, 24.2, 0.2);
                ctx.n2k.send_msg(N2kMsg);
            }
        }
    }
}

void Simulator::setup()
{}

bool Simulator::is_enabled()
{
    return enabled;
}