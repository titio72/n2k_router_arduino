#ifdef ENABLE_NMEA_GPS
#include "GPS.h"
#include "NMEA.h"
#include "Utils.h"
#include "Log.h"
#include "N2K_router.h"
#include "Conf.h"

NMEAUtils nmea;

GPS::GPS(Context _ctx, Port& _p):
    ctx(_ctx), p(_p), enabled(false), delta_time(0), gps_time_set(false)
{
}

GPS::~GPS()
{
}

bool GPS::set_system_time(int sid, RMC &rmc, bool &time_set_flag)
{
  if (rmc.unix_time)
  {
    time_t _gps_time_t = rmc.unix_time;
    short _gps_ms = rmc.unix_time_ms;
    if (ctx.conf.get_services().is_send_time())
    {
      ctx.n2k.sendSystemTime(rmc, sid);
    }
    if (!time_set_flag)
    {
      delta_time = _gps_time_t - time(0);
      Log::tracex("GPS", "Setting time", "new time {%s}}", time_to_ISO(_gps_time_t, _gps_ms));
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
    ctx.n2k.sendGNNSStatus(ctx.data_cache.gps.gsa, sid);
    ctx.n2k.sendSatellites(nmea.get_satellites(), nmea.get_n_satellites(), sid, ctx.data_cache.gps.gsa);
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
    if (NMEAUtils::parseRMC(sentence, ctx.data_cache.gps.rmc) == 0)
    {
      set_system_time(sid, ctx.data_cache.gps.rmc, gps_time_set);
      if (ctx.data_cache.gps.rmc.valid)
      {
        ctx.n2k.sendCOGSOG(ctx.data_cache.gps.rmc, sid);
        ctx.n2k.sendPosition(ctx.data_cache.gps.rmc);
        static ulong t0 = millis();
        ulong t = millis();
        if ((t - t0) > 900)
        {
          ctx.n2k.sendGNSSPosition(ctx.data_cache.gps.gsa, ctx.data_cache.gps.rmc, sid);
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
    if (nmea.parseGSA(sentence, ctx.data_cache.gps.gsa) == 0)
    {
      if (ctx.data_cache.gps.gsa.valid)
      {
        stats.valid_gsa++;
        stats.gps_fix = ctx.data_cache.gps.gsa.fix;
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
    if (nmea.parseGSV(sentence, ctx.data_cache.gps.gsv) == 0)
    {
      stats.valid_gsv++;
    }
    else
    {
      stats.invalid_gsv++;
    }
  }
}

void GPS::loop(unsigned long ms, Context &ctx)
{
    if (enabled)
    {
        p.set_speed(UART_SPEED[ ctx.conf.get_uart_speed()]);
        p.listen(250);
        send_gsv(ms);
    }
}

void GPS::setup(Context &ctx)
{
  p.set_speed(UART_SPEED[ ctx.conf.get_uart_speed()]);
  p.set_handler(this);
}

bool GPS::is_enabled()
{
    return enabled;
}

void GPS::enable()
{
  if (!enabled)
  {
    enabled = true;
  }
}

void GPS::disable()
{
  if (enabled)
  {

    RMC& rmc = ctx.data_cache.gps.rmc;
    rmc.unix_time = 0;
    rmc.valid = 0xFFFF;
    rmc.cog = NAN;
    rmc.sog = NAN;
    rmc.lat = NAN;
    rmc.lon = NAN;
    ctx.data_cache.gps.latitude = NAN;
    ctx.data_cache.gps.latitude_NS = 'N';
    ctx.data_cache.gps.longitude = NAN;
    ctx.data_cache.gps.longitude_EW = 'E';

    enabled = false;
    p.close();
  }
}

void GPS::dumpStats()
{
  if (enabled)
  {
    Log::tracex("GPS", "Stats", "Time {%s} Pos {%.4f %.4f} SOG {%.2f} COG {%.2f} Sats {%d/%d} hDOP {%.2f} pDOP {%.2f} Fix {%d}",
      time_to_ISO(ctx.data_cache.gps.rmc.unix_time, ctx.data_cache.gps.rmc.unix_time_ms),
      ctx.data_cache.gps.rmc.lat, ctx.data_cache.gps.rmc.lon, ctx.data_cache.gps.rmc.sog, ctx.data_cache.gps.rmc.cog,
      ctx.data_cache.gps.gsv.nSat, ctx.data_cache.gps.gsa.nSat, ctx.data_cache.gps.gsa.hdop, ctx.data_cache.gps.gsa.pdop, ctx.data_cache.gps.gsa.fix);
    stats.dump();
    stats.reset();
  }
}

void GPS_stats::dump()
{
    Log::tracex("GPS", "Stats", "RMC {%d/%d} GSA {%d/%d} GSV {%d/%d} fix {%d}",
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
#endif