#include "GPS.h"
#include "NMEA.h"
#include "Utils.h"
#include "Log.h"
#include "N2K_router.h"
#include "Conf.h"

NMEAUtils nmea;

GPS::GPS(Context _ctx, Port* _p):
    ctx(_ctx), p(_p), enabled(false), delta_time(0), gps_time_set(false)
{
}

GPS::~GPS()
{
}

time_t get_time(RMC &rmc, time_t &t, short &ms)
{
  tm _gps_time;
  _gps_time.tm_hour = rmc.h;
  _gps_time.tm_min = rmc.m;
  _gps_time.tm_sec = rmc.s;
  _gps_time.tm_year = rmc.y - 1900;
  _gps_time.tm_mon = rmc.M - 1;
  _gps_time.tm_mday = rmc.d;
  t = mktime(&_gps_time);
  ms = rmc.ms;
  return t;
}

bool GPS::set_system_time(int sid, RMC &rmc, bool &time_set_flag)
{
  if (rmc.y > 0)
  {
    time_t _gps_time_t;
    short _gps_ms;
    get_time(rmc, _gps_time_t, _gps_ms);
    if (ctx.conf.send_time)
    {
      ctx.n2k.sendTime(rmc, sid);
    }
    if (!time_set_flag)
    {
      delta_time = _gps_time_t - time(0);
      Log::trace("[GPS] Setting time to {%d-%d-%d %d:%d:%d.%d}}\n", rmc.y, rmc.M, rmc.d, rmc.h, rmc.m, rmc.s, rmc.ms);
      time_set_flag = true;
    }
  }
  return false;
}

void GPS::send_gsv(ulong ms)
{
  static N2KSid _sid;
  unsigned char sid = _sid.getNew();
  static ulong last_sent = 0;
  if ((ms - last_sent) >= 1000000)
  {
    last_sent = ms;
    ctx.n2k.sendGNNSStatus(ctx.cache.gsa, sid);
    ctx.n2k.sendSatellites(nmea.get_satellites(), nmea.get_n_satellites(), sid, ctx.cache.gsa);
  }
}

void GPS::on_line_read(const char *sentence)
{
  static N2KSid _sid;
  unsigned char sid = _sid.getNew();
  sid++;
  //Log::trace("[GPS] Process Sentence {%s}\n", sentence);
  if (NMEAUtils::is_sentence(sentence, "RMC"))
  {
    if (NMEAUtils::parseRMC(sentence, ctx.cache.rmc) == 0)
    {
      set_system_time(sid, ctx.cache.rmc, gps_time_set);
      if (ctx.cache.rmc.valid)
      {
        ctx.n2k.sendCOGSOG(ctx.cache.rmc, sid);
        ctx.n2k.sendPosition(ctx.cache.rmc);
        static ulong t0 = millis();
        ulong t = millis();
        if ((t - t0) > 900)
        {
          ctx.n2k.sendGNSSPosition(ctx.cache.gsa, ctx.cache.rmc, sid);
          t0 = t;
        }
        stats.valid_rmc++;
      }
      else
      {
        stats.invalid_rmc++;
      }
    }
  }
  else if (NMEAUtils::is_sentence(sentence, "GSA"))
  {
    if (nmea.parseGSA(sentence, ctx.cache.gsa) == 0)
    {
      if (ctx.cache.gsa.valid)
      {
        stats.valid_gsa++;
        stats.gps_fix = ctx.cache.gsa.fix;
      }
      else
      {
        stats.invalid_gsa++;
        stats.gps_fix = 0;
      }
    }
  }
  else if (NMEAUtils::is_sentence(sentence, "GSV"))
  {
    if (nmea.parseGSV(sentence) == 0)
    {
      stats.valid_gsv++;
    }
    else
    {
      stats.invalid_gsv++;
    }
  }
}

void GPS::loop(unsigned long ms)
{
    if (enabled && p)
    {
        p->set_speed(UART_SPEED[ctx.conf.uart_speed]);
        p->listen(250);
        send_gsv(ms);
    }
}

void GPS::setup()
{
  if (p!=NULL)
  {
    p->set_handler(this);
  }
}

bool GPS::is_enabled()
{
    return enabled;
}

void GPS::enable()
{
  if (!enabled && p)
  {
    enabled = true;
  }
}

void GPS::disable()
{
  if (enabled)
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

    enabled = false;
    p->close();
  }
}

void GPS::dumpStats()
{
  if (enabled)
  {
    Log::trace("[STATS] Time {%04d-%02d-%02dT%02d:%02d:%02d} ", ctx.cache.rmc.y, ctx.cache.rmc.M, ctx.cache.rmc.d, ctx.cache.rmc.h, ctx.cache.rmc.m, ctx.cache.rmc.s);
    Log::trace("Pos {%.4f %.4f} SOG {%.2f} COG {%.2f} ", ctx.cache.rmc.lat, ctx.cache.rmc.lon, ctx.cache.rmc.sog, ctx.cache.rmc.cog);
    Log::trace("Sats {%d/%d} hDOP {%.2f} pDOP {%.2f} Fix {%d}\n", ctx.cache.gsv.nSat, ctx.cache.gsa.nSat, ctx.cache.gsa.hdop, ctx.cache.gsa.pdop, ctx.cache.gsa.fix);
    stats.dump();
    stats.reset();
  }
}

void GPS_stats::dump()
{
    Log::trace("[STATS] GPS: RMC {%d/%d} GSA {%d/%d} GSV {%d/%d} fix {%d}\n",
                  valid_rmc, invalid_rmc,
                  valid_gsa, invalid_gsa,
                  valid_gsv, invalid_gsv, gps_fix);
}

void GPS_stats::reset()
{
  valid_gsa = 0;
  invalid_gsa = 0;
  valid_gsv = 0;
  invalid_gsv = 0;
  valid_rmc = 0;
  invalid_rmc = 0;
  gps_fix = 0;
}