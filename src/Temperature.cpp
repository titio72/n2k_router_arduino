#include "Temperature.h"
#include "Data.h"
#include "Constants.h"
#include "Conf.h"
#include <math.h>

#ifdef ARDUINO
#include <Arduino.h>
#else
// Mock Arduino functions for testing
#define INPUT 0
inline int analogRead(int pin) { return 2048; }
inline void pinMode(int pin, int mode) {}
#endif

// NTC Thermistor Constants for 10K ohm at 25°C
// Steinhart-Hart equation coefficients
const double A = 1.009249522e-03;  // Coefficient A
const double B = 2.378405444e-04;  // Coefficient B
const double C = 2.019202697e-07;  // Coefficient C

// Reference resistor value (ohms) - used in voltage divider
// Typically 10K ohm for balanced sensitivity
const double R_REF = 10000.0;

// Voltage divider constants
const double ADC_RESOLUTION = 4095.0;  // ESP32 12-bit ADC
const double V_REF = 3.3;              // Reference voltage

// Smoothing filter alpha (exponential moving average)
const double DEFAULT_ALPHA = 0.1;

// Temperature smoothing and calibration
const double DEFAULT_ADJUSTMENT = 1.0;
const double CELSIUS_OFFSET = 273.15;

WaterTemperature::WaterTemperature(int pin)
    : temperature(25.0),
      last_read_time(0),
      adjustment_factor(DEFAULT_ADJUSTMENT)
{
    // Store pin and will be used in setup
    this->pin = pin;
}

WaterTemperature::~WaterTemperature()
{
}

void WaterTemperature::setup()
{
    // Configure the pin as analog input
    pinMode(pin, INPUT);
}

void WaterTemperature::loop_micros(unsigned long now_micros)
{
    // Not used for temperature - readings happen on-demand in read_data
}

void WaterTemperature::read_data(WaterData &data, Configuration &conf, unsigned long milliseconds)
{
    // Update last read time
    last_read_time = milliseconds;
    
    // Check if temperature measurement is enabled
    if (conf.get_services().is_use_tmp() == 0) {
        // Temperature measurement disabled
        data.temperature = NAN;
        data.temperature_error = TEMP_ERROR_NO_SIGNAL;
        return;
    }
    
    // Read raw ADC value from the thermistor pin
    int adc_value = analogRead(pin);
    
    if (adc_value <= 0) {
        // Invalid reading - no signal
        data.temperature = NAN;
        data.temperature_error = TEMP_ERROR_NO_SIGNAL;
        return;
    }
    
    // Convert ADC value to voltage
    // V = (ADC_value / ADC_RESOLUTION) * V_REF
    double voltage = (adc_value / ADC_RESOLUTION) * V_REF;
    
    // Calculate thermistor resistance using voltage divider formula
    // V_thermistor = V_ref * R_thermistor / (R_ref + R_thermistor)
    // Solving for R_thermistor:
    // R_thermistor = R_ref * V_thermistor / (V_ref - V_thermistor)
    
    if (voltage <= 0 || voltage >= V_REF) {
        // Invalid voltage reading
        data.temperature = NAN;
        data.temperature_error = TEMP_ERROR_NO_SIGNAL;
        return;
    }
    
    double r_thermistor = R_REF * voltage / (V_REF - voltage);
    
    if (r_thermistor <= 0) {
        // Invalid resistance
        data.temperature = NAN;
        data.temperature_error = TEMP_ERROR_NO_SIGNAL;
        return;
    }
    
    // Apply Steinhart-Hart equation to get temperature in Kelvin
    // 1/T = A + B*ln(R) + C*(ln(R))^3
    double ln_r = log(r_thermistor);
    double ln_r_cubed = ln_r * ln_r * ln_r;
    
    double temp_kelvin = 1.0 / (A + B * ln_r + C * ln_r_cubed);
    
    // Convert from Kelvin to Celsius
    double temp_celsius = temp_kelvin - CELSIUS_OFFSET;
    
    // Apply smoothing filter (exponential moving average)
    temperature = DEFAULT_ALPHA * temp_celsius + (1.0 - DEFAULT_ALPHA) * temperature;
    
    // Apply configuration adjustment factor
    double adjusted_temp = temperature * conf.get_sea_temp_alpha();
    
    // Clamp temperature to reasonable range (-10°C to 50°C)
    const double MIN_TEMP = -10.0;
    const double MAX_TEMP = 50.0;
    
    if (adjusted_temp < MIN_TEMP || adjusted_temp > MAX_TEMP) {
        // Temperature out of valid range
        data.temperature = NAN;
        data.temperature_error = TEMP_ERROR_NO_SIGNAL;
        return;
    }
    
    // Set temperature data
    data.temperature = adjusted_temp * adjustment_factor;
    data.temperature_error = TEMP_ERROR_OK;
}