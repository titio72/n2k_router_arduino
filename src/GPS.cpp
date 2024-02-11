#include "GPS.h"
#include "NMEA.h"
#include "Utils.h"
#include "Log.h"
#include "N2K.h"
#include "Conf.h"

NMEAUtils nmea;


GPS::GPS(Context _ctx):
    ctx(_ctx), p(NULL), enabled(false), device_name(NULL), delta_time(0), gps_time_set(false)
{
}

GPS::~GPS()
{
  if (p) delete p;
  if (device_name) delete device_name;
}

void GPS::set_port_name(const char* _port)
{
  device_name = strdup(_port);
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
  static unsigned char sid = 0;
  sid++;
  static ulong last_sent = 0;
  if ((ms - last_sent) >= 900)
  {
    last_sent = ms;
    ctx.n2k.sendGNNSStatus(ctx.cache.gsa, sid);
    ctx.n2k.sendSatellites(nmea.get_satellites(), nmea.get_n_satellites(), sid, ctx.cache.gsa);
  }
}

int GPS::on_line_read(const char *sentence)
{
  static unsigned char sid = 0;
  sid++;
  //Log::trace("[GPS] Process Sentence {%s}\n", sentence);
  if (NMEAUtils::is_sentence(sentence, "RMC"))
  {
    if (NMEAUtils::parseRMC(sentence, ctx.cache.rmc) == 0)
    {
      set_system_time(sid, ctx.cache.rmc, gps_time_set);
      if (ctx.cache.rmc.valid)
      {
        ctx.n2k.sendCOGSOG(ctx.cache.gsa, ctx.cache.rmc, sid);
        ctx.n2k.sendPosition(ctx.cache.gsa, ctx.cache.rmc);
        static ulong t0 = millis();
        ulong t = millis();
        if ((t - t0) > 900)
        {
          ctx.n2k.sendGNSSPosition(ctx.cache.gsa, ctx.cache.rmc, sid);
          t0 = t;
        }
        ctx.stats.valid_rmc++;
        return 0;
      }
      else
      {
        ctx.stats.invalid_rmc++;
      }
    }
  }
  else if (NMEAUtils::is_sentence(sentence, "GSA"))
  {
    if (nmea.parseGSA(sentence, ctx.cache.gsa) == 0)
    {
      if (ctx.cache.gsa.valid)
      {
        ctx.stats.valid_gsa++;
        ctx.stats.gps_fix = ctx.cache.gsa.fix;
      }
      else
      {
        ctx.stats.invalid_gsa++;
        ctx.stats.gps_fix = 0;
      }
      return 0;
    }
  }
  else if (NMEAUtils::is_sentence(sentence, "GSV"))
  {
    if (nmea.parseGSV(sentence) == 0)
    {
      ctx.stats.valid_gsv++;
    }
    else
    {
      ctx.stats.invalid_gsv++;
    }
  }
  return -1;
}

void GPS::loop(unsigned long ms)
{
    if (enabled && p)
    {
        p->set_speed(atoi(UART_SPEED[ctx.conf.uart_speed]));
        p->listen(250);
        send_gsv(ms);
    }
}

void GPS::setup()
{
  if (p==NULL)
  {
    p = new Port(device_name==NULL?"":device_name, &(ctx.stats.bytes_uart));
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
    enabled = false;
    p->close();
  }
}