#include "Data.h"
#include "Conf.h"

double Data::get_humidity(Configuration& conf)
{
    if (conf.get_humidity_source() == METEO_BME)
    {
        return humidity_0;
    }
    else if (conf.get_humidity_source() == METEO_DHT)
    {
        return humidity_1;
    }
    else
    {
        return NAN;
    }
}

double Data::get_pressure(Configuration& conf)
{
    if (conf.get_pressure_source() == METEO_BME)
    {
        return pressure_0;
    }
    else
    {
        return NAN;
    }
}

double Data::get_temperature(Configuration& conf)
{
    if (conf.get_temperature_source() == METEO_BME)
    {
        return temperature_0;
    }
    else if (conf.get_temperature_source() == METEO_DHT)
    {
        return temperature_1;
    }
    else
    {
        return NAN;
    }
}

double Data::get_temperature_el(Configuration& conf)
{
    if (conf.get_temperature_el_source() == METEO_BME)
    {
        return temperature_0;
    }
    else if (conf.get_temperature_el_source() == METEO_DHT)
    {
        return temperature_1;
    }
    else
    {
        return NAN;
    }
}