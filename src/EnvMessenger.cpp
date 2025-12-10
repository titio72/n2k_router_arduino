#include "EnvMessenger.h"
#include "N2K_router.h"
#include "Conf.h"
#include "Data.h"

#ifndef PERIOD_MICROS_ENV
#define PERIOD_MICROS_ENV 2000000
#endif

EnvMessenger::EnvMessenger(): enabled(false), t0(0)
{}

EnvMessenger::~EnvMessenger() {}

void EnvMessenger::enable() { enabled = true; }
void EnvMessenger::disable() { enabled = false; }
bool EnvMessenger::is_enabled() { return enabled; }
void EnvMessenger::setup(Context& ctx) {}


inline double to_n2k(double value)
{
  if (isnan(value))
    return N2kDoubleNA;
  else
    return value;
}

void EnvMessenger::loop(unsigned long ms, Context &ctx)
{
  if (enabled && check_elapsed(ms, t0, PERIOD_MICROS_ENV))
  {
    double p = ctx.data_cache.get_pressure(ctx.conf);
    double h = ctx.data_cache.get_humidity(ctx.conf);
    double t = ctx.data_cache.get_temperature(ctx.conf);
    double t_el = ctx.data_cache.get_temperature_el(ctx.conf);

    if (!isnan(p)) ctx.n2k.sendPressure(p);
    if (!isnan(t)) ctx.n2k.sendCabinTemp(t);
    if (!isnan(h)) ctx.n2k.sendHumidity(h);
    if (!isnan(t_el)) ctx.n2k.sendElectronicTemperature(t_el);

    if (!(isnan(p) && isnan(t) && isnan(h))) ctx.n2k.sendEnvironmentXRaymarine(to_n2k(p), to_n2k(h), to_n2k(t));
  }
}
