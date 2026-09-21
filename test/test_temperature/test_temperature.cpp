#include <unity.h>
#include <math.h>
#include "Temperature.h"
#include "Data.h"
#include "Conf.h"

#define WATER_TEMP_PIN 25

void setUp(void) {}
void tearDown(void) {}

#pragma region Sea Temp Adjustment Tests

// Configuration::save_sea_temp_adjustment/save_sea_temp_alpha exist and are exposed over
// BLE telemetry, but until WaterTemperature actually reads them back, calibrating sea
// temperature via those settings has no effect on the reported value. These tests pin
// down that read_data() must apply conf.get_sea_temp_adjustment()/get_sea_temp_alpha().

void test_water_temperature_applies_configured_adjustment(void)
{
    WaterTemperature::set_mock_millivolts(1650);

    MockConfiguration baseline_conf;
    baseline_conf.save_sea_temp_adjustment(1.0);
    WaterTemperature baseline(WATER_TEMP_PIN);
    WaterData baseline_data;
    baseline.read_data(baseline_data, baseline_conf);

    MockConfiguration scaled_conf;
    scaled_conf.save_sea_temp_adjustment(2.0);
    WaterTemperature scaled(WATER_TEMP_PIN);
    WaterData scaled_data;
    scaled.read_data(scaled_data, scaled_conf);

    TEST_ASSERT_EQUAL_INT(TEMP_ERROR_OK, baseline_data.temperature_error);
    TEST_ASSERT_EQUAL_INT(TEMP_ERROR_OK, scaled_data.temperature_error);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, baseline_data.temperature * 2.0, scaled_data.temperature);
}

#pragma endregion

#pragma region Sea Temp Alpha Tests

void test_water_temperature_applies_configured_alpha(void)
{
    // alpha = 1.0 means "fully trust the new reading" (no smoothing).
    MockConfiguration full_trust_conf;
    full_trust_conf.save_sea_temp_alpha(1.0);
    full_trust_conf.save_sea_temp_adjustment(1.0);
    WaterTemperature full_trust(WATER_TEMP_PIN);
    WaterData full_trust_data;

    WaterTemperature::set_mock_millivolts(1650);
    full_trust.read_data(full_trust_data, full_trust_conf); // seeds temperature (first reading always adopted as-is)
    double first_reading = full_trust_data.temperature;

    WaterTemperature::set_mock_millivolts(1400); // sensor voltage changes -> different raw temperature
    full_trust.read_data(full_trust_data, full_trust_conf);
    double full_trust_second_reading = full_trust_data.temperature;

    // alpha = 0.0 means "ignore the new reading, keep the previous smoothed value".
    MockConfiguration no_trust_conf;
    no_trust_conf.save_sea_temp_alpha(0.0);
    no_trust_conf.save_sea_temp_adjustment(1.0);
    WaterTemperature no_trust(WATER_TEMP_PIN);
    WaterData no_trust_data;

    WaterTemperature::set_mock_millivolts(1650);
    no_trust.read_data(no_trust_data, no_trust_conf); // same seed reading as above

    WaterTemperature::set_mock_millivolts(1400); // same voltage change as above
    no_trust.read_data(no_trust_data, no_trust_conf);

    // With alpha=1.0 the second reading should have moved (tracks the new voltage).
    TEST_ASSERT_TRUE(fabs(full_trust_second_reading - first_reading) > 0.01);
    // With alpha=0.0 the second reading should be unchanged from the seed (ignores the new voltage).
    TEST_ASSERT_DOUBLE_WITHIN(0.001, first_reading, no_trust_data.temperature);

    WaterTemperature::set_mock_millivolts(1650); // restore default for other test suites
}

#pragma endregion

#pragma region Sensor fault tests

void test_water_temperature_fault_does_not_poison_the_filter(void)
{
    // A disconnected sensor reads ~-273 C. With a smoothing alpha < 1 that sample used to enter the filter and
    // keep the output out of range for a long time after the sensor came back.
    MockConfiguration conf;
    conf.save_sea_temp_alpha(0.1);
    WaterTemperature wt(WATER_TEMP_PIN);
    WaterData data;

    WaterTemperature::set_mock_millivolts(1650);
    wt.read_data(data, conf);
    TEST_ASSERT_EQUAL_INT(TEMP_ERROR_OK, data.temperature_error);
    double good = data.temperature;

    WaterTemperature::set_mock_millivolts(0); // unplugged: floating input reads 0 V
    wt.read_data(data, conf);
    TEST_ASSERT_EQUAL_INT(TEMP_ERROR_NO_SIGNAL, data.temperature_error);
    TEST_ASSERT_TRUE(isnan(data.temperature));

    WaterTemperature::set_mock_millivolts(1650); // plugged back in: first sample must be trusted straight away
    wt.read_data(data, conf);
    TEST_ASSERT_EQUAL_INT(TEMP_ERROR_OK, data.temperature_error);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, good, data.temperature);

    WaterTemperature::set_mock_millivolts(1650);
}

void test_water_temperature_out_of_range_sample_is_rejected(void)
{
    MockConfiguration conf;
    WaterTemperature wt(WATER_TEMP_PIN);
    WaterData data;
    WaterTemperature::set_mock_millivolts(4900); // r ~ 100 ohm: far hotter than 70 C
    wt.read_data(data, conf);
    TEST_ASSERT_EQUAL_INT(TEMP_ERROR_NO_SIGNAL, data.temperature_error);
    TEST_ASSERT_TRUE(isnan(data.temperature));
    WaterTemperature::set_mock_millivolts(1650);
}

#pragma endregion

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_water_temperature_applies_configured_adjustment);
    RUN_TEST(test_water_temperature_applies_configured_alpha);
    RUN_TEST(test_water_temperature_fault_does_not_poison_the_filter);
    RUN_TEST(test_water_temperature_out_of_range_sample_is_rejected);

    return UNITY_END();
}
