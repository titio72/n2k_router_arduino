#include "Temperature.h"
#include "Data.h"
#include "Constants.h"
#include "Conf.h"
#include "N2K_router.h"
#include <Utils.h>
#include <math.h>

#ifdef ARDUINO
#include <Arduino.h>
#else
// Mock Arduino functions for testing
#define INPUT 0
inline int analogReadMilliVolts(int pin) { return 1650; } // 1.65V mock value
inline void pinMode(int pin, int mode) {}
#endif

#define WATER_TEMP_PERIOD 1000000L // Period for temperature reading

// NTC Thermistor Configuration
const float SERIES_RESISTOR = 4700;  // series resistor in ohms
const float NTC_NOMINAL_R = 10000;   // NTC resistance at 25°C
const float NTC_NOMINAL_T = 25;      // Nominal temperature (25°C)
const float NTC_BETA = 3950;         // Beta coefficient (typical for 10k NTC)
const float REF_VOLTAGE = 5.0;       // Reference voltage (check the PCB!!!!)

// Smoothing filter alpha
const double DEFAULT_ALPHA = 0.1;

// Temperature smoothing and calibration
const double DEFAULT_ADJUSTMENT = 1.0;
const double CELSIUS_OFFSET = 273.15;

WaterTemperature::WaterTemperature(int _pin)
    : temperature(NAN),
      last_read_time(0),
      adjustment_factor(DEFAULT_ADJUSTMENT),
      pin(_pin)
{}

WaterTemperature::~WaterTemperature()
{
}

void WaterTemperature::enable()
{
    if (!enabled)
    {
        enabled = true;
    }
}

void WaterTemperature::disable()
{
    if (enabled)
    {
        enabled = false;
    }
}

bool WaterTemperature::is_enabled()
{
    return enabled;
}

void WaterTemperature::setup()
{
    // Configure the pin as analog input
    if (pin != -1) pinMode(pin, INPUT);
}

void WaterTemperature::loop(unsigned long now_micros, Context &ctx)
{
    WaterData &data = ctx.data_cache.water_data;
    
    // Check if temperature measurement is enabled
    if (!is_enabled() || pin == -1) {
        // Temperature measurement disabled
        data.temperature = NAN;
        data.temperature_error = TEMP_ERROR_NO_SIGNAL;
        return;
    }
    
    // Not used for temperature - readings happen on-demand in read_data
    if (check_elapsed(now_micros, last_read_time, WATER_TEMP_PERIOD) == 0)
        return;

    read_data(data, ctx.conf);
    if (data.temperature_error == TEMP_ERROR_OK)
    {
        ctx.n2k.sendSeaTemperature(data.temperature, 0);
    }
}

double read_ntc(int pin, double &r, double &v) {
  // read ADC value in volts
  v = (double)analogReadMilliVolts(pin) / 1000.0; // in volts
  
  // Calculate NTC resistance using voltage divider formula
  // Vout = Vref * R_ntc / (R_series + R_ntc)
  r = SERIES_RESISTOR * (REF_VOLTAGE - v) / v;
  
  // Use Steinhart-Hart equation to calculate temperature
  // 1/T = 1/T0 + (1/B) * ln(R/R0)
  double tempKelvin = 1.0 / (1.0 / (NTC_NOMINAL_T + 273.15) + 
                             (1.0 / NTC_BETA) * log(r / NTC_NOMINAL_R));
  
  // Convert from Kelvin to Celsius
  return tempKelvin - CELSIUS_OFFSET;
}

void WaterTemperature::read_data(WaterData &data, Configuration &conf)
{
    double r_thermistor;
    double voltage;
    double temp_celsius = read_ntc(pin, r_thermistor, voltage);
    if (isnan(temp_celsius)) {
        // Reading failed
        data.temperature = NAN;
        data.temperature_error = TEMP_ERROR_NO_SIGNAL;
        return;
    }
    
    // Apply smoothing filter (exponential moving average)
    temperature = DEFAULT_ALPHA * temp_celsius + (1.0 - DEFAULT_ALPHA) * temperature;
    
    // Apply configuration adjustment factor
    double smoothed_temp = temperature * conf.get_sea_temp_alpha();
    
    // Clamp temperature to reasonable range (-10°C to 50°C)
    const double MIN_TEMP = -10.0;
    const double MAX_TEMP = 50.0;
    
    if (smoothed_temp < MIN_TEMP || smoothed_temp > MAX_TEMP) {
        // Temperature out of valid range
        data.temperature = NAN;
        data.temperature_error = TEMP_ERROR_NO_SIGNAL;
    }
    else
    {
        // Set temperature data
        data.temperature = smoothed_temp * adjustment_factor;
        data.temperature_error = TEMP_ERROR_OK;
    }
}