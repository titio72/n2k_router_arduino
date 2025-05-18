#include "EnvMessanger.h"
#include "N2K_router.h"
#include "Conf.h"
#include "Data.h"

#define PERIOD_MICROS_ENV 2000000

EnvMessanger::EnvMessanger(Context& c): enabled(false), ctx(c), t0(0)
{}

EnvMessanger::~EnvMessanger() {}

void EnvMessanger::enable() { enabled = true; }
void EnvMessanger::disable() { enabled = false; }
bool EnvMessanger::is_enabled() { return enabled; }
void EnvMessanger::setup() {}


inline double to_n2k(double value)
{
  if (isnan(value))
    return N2kDoubleNA;
  else
    return value;
}

void EnvMessanger::loop(unsigned long ms)
{
  if (enabled && check_elapsed(ms, t0, PERIOD_MICROS_ENV))
  {
    double p = ctx.cache.get_pressure(ctx.conf);
    double h = ctx.cache.get_humidity(ctx.conf);
    double t = ctx.cache.get_temperature(ctx.conf);
    double t_el = ctx.cache.get_temperature_el(ctx.conf);

    if (!isnan(p)) ctx.n2k.sendPressure(p);
    if (!isnan(t)) ctx.n2k.sendCabinTemp(t);
    if (!isnan(h)) ctx.n2k.sendHumidity(h);
    if (!isnan(t_el)) ctx.n2k.sendElectronicTemperature(t_el);

    if (!(isnan(p) && isnan(t) && isnan(h))) ctx.n2k.sendEnvironmentXRaymarine(to_n2k(p), to_n2k(h), to_n2k(t));

    t0 = ms;
  }
}
