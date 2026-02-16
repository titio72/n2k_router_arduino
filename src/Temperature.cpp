#include "Temperature.h"
#include "Data.h"
#include "Constants.h"
#include "Conf.h"
#include "N2K_router.h"
#include <Utils.h>
#include <math.h>
#include <Log.h>

#ifdef ARDUINO
#include <Arduino.h>
#else
// Mock Arduino functions for testing
#define INPUT 0
inline int analogReadMilliVolts(int pin) { return 1650; } // 1.65V mock value
//inline void pinMode(int pin, int mode) {}
#endif

#define WATER_TEMP_PERIOD 1000000L // Period for temperature reading

// NTC Thermistor Configuration
const float SERIES_RESISTOR = 4700;  // series resistor in ohms
const float NTC_NOMINAL_R = 10000;   // NTC resistance at 25°C
const float NTC_NOMINAL_T = 25;      // Nominal temperature (25°C)
const float NTC_BETA = 3950;         // Beta coefficient (typical for 10k NTC)
const float REF_VOLTAGE = 5.0;       // Reference voltage (check the PCB!!!!)

// Smoothing filter alpha
const double DEFAULT_ALPHA = 0.67; // Smoothing factor for exponential moving average (0.0 - 1.0)

// Temperature smoothing and calibration
const double DEFAULT_ADJUSTMENT = 1.0;
const double CELSIUS_OFFSET = 273.15;

static void reset_water_temp_data(WaterData &data, uint8_t error_code = TEMP_ERROR_OK)
{
    data.temperature = NAN;
    data.temperature_error = error_code;
}

WaterTemperature::WaterTemperature(int _pin)
    : temperature(NAN),
      last_read_time(0),
      adjustment_factor(DEFAULT_ADJUSTMENT),
      pin(_pin),
      enabled(false)
{}

WaterTemperature::~WaterTemperature()
{
}

void WaterTemperature::enable(Context &ctx)
{
    if (!enabled)
    {
        Log::tracex("WTT", "Enabling", "Pin {%d}", pin);
        enabled = true;
    }
}

void WaterTemperature::disable(Context &ctx)
{
    if (enabled)
    {
        reset_water_temp_data(ctx.data_cache.water_data);
        Log::tracex("WTT", "Disabling", "Pin {%d}", pin);
        enabled = false;
    }
}

bool WaterTemperature::is_enabled()
{
    return enabled;
}

void WaterTemperature::setup(Context &ctx)
{
    // Configure the pin as analog input
    Log::tracex("WTT", "Setup", "Pin {%d}", pin);
    if (pin != -1) pinMode(pin, INPUT);
}

void WaterTemperature::loop(unsigned long now_micros, Context &ctx)
{
    WaterData &data = ctx.data_cache.water_data;
    
    // Check if temperature measurement is enabled
    if (!is_enabled() || pin == -1) {
        // Temperature measurement disabled
        reset_water_temp_data(data, TEMP_ERROR_NO_SIGNAL);
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
  int i = analogRead(pin);
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
    //Log::tracex("WTT", "Reading temperature", "Pin {%d} Voltage {%.2f} Ohms {%.2f} Temp {%.2f}", pin, voltage, r_thermistor, temp_celsius);
    if (isnan(temp_celsius)) {
        // Reading failed
        reset_water_temp_data(data, TEMP_ERROR_NO_SIGNAL);
        return;
    }
    
    // Apply smoothing filter (exponential moving average)
    if (isnan(temperature)) {
        // First reading, initialize temperature
        temperature = temp_celsius;
    }
    temperature = DEFAULT_ALPHA * temp_celsius + (1.0 - DEFAULT_ALPHA) * temperature;
    
        // Clamp temperature to reasonable range (-10°C to 70°C)
    const double MIN_TEMP = -10.0;
    const double MAX_TEMP = 70.0;
    
    if (temperature < MIN_TEMP || temperature > MAX_TEMP) {
        // Temperature out of valid range
        reset_water_temp_data(data, TEMP_ERROR_NO_SIGNAL);
    }
    else
    {
        //Serial.printf("---- %f", temperature);
        // Set temperature data
        data.temperature = temperature * adjustment_factor;
        data.temperature_error = TEMP_ERROR_OK;
    }
}