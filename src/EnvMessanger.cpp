#include "EnvMessanger.h"
#include "N2K_router.h"
#include "Conf.h"

#define PERIOD_MICROS_ENV 2000000

EnvMessanger::EnvMessanger(Context& c): enabled(false), ctx(c)
{}

EnvMessanger::~EnvMessanger() {}

void EnvMessanger::enable() { enabled = true; }
void EnvMessanger::disable() { enabled = false; }
bool EnvMessanger::is_enabled() { return enabled; }
void EnvMessanger::setup() {}

void EnvMessanger::loop(unsigned long ms)
{
  static unsigned long t0 = 0;
  if ((ctx.conf.get_services().use_dht || ctx.conf.get_services().use_bmp) && enabled && check_elapsed(ms, t0, PERIOD_MICROS_ENV))
  {
    if (!isnan(ctx.cache.pressure)) ctx.n2k.sendPressure(ctx.cache.pressure);
    if (!isnan(ctx.cache.temperature)) ctx.n2k.sendCabinTemp(ctx.cache.temperature);
    if (!isnan(ctx.cache.humidity)) ctx.n2k.sendHumidity(ctx.cache.humidity);

    double p = isnan(ctx.cache.pressure)?N2kDoubleNA:ctx.cache.pressure;
    double h = isnan(ctx.cache.humidity)?N2kDoubleNA:ctx.cache.humidity;
    double t = isnan(ctx.cache.temperature)?N2kDoubleNA:ctx.cache.temperature;
    if (p!=N2kDoubleNA || h!=N2kDoubleNA || t!=N2kDoubleNA) ctx.n2k.sendEnvironmentXRaymarine(ctx.cache.pressure, ctx.cache.humidity, ctx.cache.temperature);

    if (!isnan(ctx.cache.temperature_el)) ctx.n2k.sendElectronicTemperature(ctx.cache.temperature_el);
    t0 = ms;
  }
}
