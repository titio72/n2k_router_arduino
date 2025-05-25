#include "Data.h"
#include "Conf.h"

double Data::get_humidity(Configuration& conf)
{
    if (conf.get_humidity_source() == METEO_BME)
    {
        return meteo_0.humidity;
    }
    else if (conf.get_humidity_source() == METEO_DHT)
    {
        return meteo_1.humidity;
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
        return meteo_0.pressure;
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
        return meteo_0.temperature;
    }
    else if (conf.get_temperature_source() == METEO_DHT)
    {
        return meteo_1.temperature;
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
        return meteo_0.temperature;
    }
    else if (conf.get_temperature_el_source() == METEO_DHT)
    {
        return meteo_1.temperature;
    }
    else
    {
        return NAN;
    }
}