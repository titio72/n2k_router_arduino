#include <unity.h>
#include <math.h>
#include "SpeedSensorInterrupt.h"

void setUp(void)
{
    // Reset before each test
}

void tearDown(void)
{
    // Cleanup after each test
}

#pragma region Constructor Tests

void test_speed_sensor_interrupt_constructor_default(void)
{
    SpeedSensorInterrupt sensor(25, 0);

    TEST_ASSERT_EQUAL_INT(25, sensor.get_pin());
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 1.0, sensor.get_alpha());
}

void test_speed_sensor_interrupt_constructor_negative_pin(void)
{
    SpeedSensorInterrupt sensor(-1, 0);

    TEST_ASSERT_EQUAL_INT(-1, sensor.get_pin());
}

void test_speed_sensor_interrupt_constructor_different_pins(void)
{
    SpeedSensorInterrupt sensor1(10, 0);
    SpeedSensorInterrupt sensor2(20, 1);
    SpeedSensorInterrupt sensor3(30, 2);

    TEST_ASSERT_EQUAL_INT(10, sensor1.get_pin());
    TEST_ASSERT_EQUAL_INT(20, sensor2.get_pin());
    TEST_ASSERT_EQUAL_INT(30, sensor3.get_pin());
}

void test_speed_sensor_interrupt_constructor_different_instances(void)
{
    SpeedSensorInterrupt sensor0(10, 0);
    SpeedSensorInterrupt sensor1(11, 1);
    SpeedSensorInterrupt sensor2(12, 2);
    SpeedSensorInterrupt sensor3(13, 3);

    TEST_ASSERT_EQUAL_INT(10, sensor0.get_pin());
    TEST_ASSERT_EQUAL_INT(11, sensor1.get_pin());
    TEST_ASSERT_EQUAL_INT(12, sensor2.get_pin());
    TEST_ASSERT_EQUAL_INT(13, sensor3.get_pin());
}

void test_speed_sensor_interrupt_destructor_cleans_up(void)
{
    {
        SpeedSensorInterrupt sensor(25, 0);
    }
    // No crash = success
    TEST_ASSERT_TRUE(true);
}

#pragma endregion

#pragma region Setup Tests

void test_speed_sensor_interrupt_setup_no_crash(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    sensor.setup();
    // No crash = success
    TEST_ASSERT_TRUE(true);
}

void test_speed_sensor_interrupt_setup_negative_pin_no_crash(void)
{
    SpeedSensorInterrupt sensor(-1, 0);
    sensor.setup();
    // Should not crash even with negative pin
    TEST_ASSERT_TRUE(true);
}

void test_speed_sensor_interrupt_setup_all_instances(void)
{
    SpeedSensorInterrupt sensor0(10, 0);
    SpeedSensorInterrupt sensor1(11, 1);
    SpeedSensorInterrupt sensor2(12, 2);
    SpeedSensorInterrupt sensor3(13, 3);
    
    sensor0.setup();
    sensor1.setup();
    sensor2.setup();
    sensor3.setup();
    
    // No crash = success
    TEST_ASSERT_TRUE(true);
}

#pragma endregion

#pragma region Alpha Tests

void test_speed_sensor_interrupt_set_alpha(void)
{
    SpeedSensorInterrupt sensor(25, 0);

    sensor.set_alpha(0.5);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.5, sensor.get_alpha());
}

void test_speed_sensor_interrupt_set_alpha_zero(void)
{
    SpeedSensorInterrupt sensor(25, 0);

    sensor.set_alpha(0.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, sensor.get_alpha());
}

void test_speed_sensor_interrupt_set_alpha_one(void)
{
    SpeedSensorInterrupt sensor(25, 0);

    sensor.set_alpha(1.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 1.0, sensor.get_alpha());
}

void test_speed_sensor_interrupt_set_alpha_fractional(void)
{
    SpeedSensorInterrupt sensor(25, 0);

    sensor.set_alpha(0.25);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.25, sensor.get_alpha());

    sensor.set_alpha(0.75);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.75, sensor.get_alpha());
}

#pragma endregion

#pragma region Signal Tests

void test_speed_sensor_interrupt_signal_increments_counter(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    // Initialize timing first
    sensor.read_data(0, frequency, counter_out);
    
    // Call signal once AFTER initialization
    sensor.signal();

    // Read data to get the count
    bool result = sensor.read_data(100, frequency, counter_out);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 10.0, frequency);  // 1 signal / 100ms = 10 Hz
}

void test_speed_sensor_interrupt_multiple_signals(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    // Initialize timing first
    sensor.read_data(0, frequency, counter_out);

    // Call signal 5 times AFTER initialization
    for (int i = 0; i < 5; i++)
    {
        sensor.signal();
    }

    bool result = sensor.read_data(100, frequency, counter_out);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 50.0, frequency);  // 5 signals / 100ms = 50 Hz
}

void test_speed_sensor_interrupt_no_signals(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    // No signals
    sensor.read_data(0, frequency, counter_out);
    bool result = sensor.read_data(100, frequency, counter_out);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, frequency);  // 0 signals = 0 Hz
}

void test_speed_sensor_interrupt_rapid_signals(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    // Initialize timing first
    sensor.read_data(0, frequency, counter_out);

    // Many rapid signals AFTER initialization
    for (int i = 0; i < 100; i++)
    {
        sensor.signal();
    }

    bool result = sensor.read_data(100, frequency, counter_out);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 1000.0, frequency);  // 100 signals / 100ms = 1000 Hz
}

#pragma endregion

#pragma region Read Data Tests

void test_speed_sensor_interrupt_read_data_returns_counter(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.signal();
    sensor.signal();
    sensor.signal();

    sensor.read_data(0, frequency, counter_out);
    TEST_ASSERT_EQUAL_INT(3, counter_out);
}

void test_speed_sensor_interrupt_read_data_too_soon(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.signal();
    sensor.read_data(0, frequency, counter_out);

    // Read at 30ms - should return true but frequency is 0 because dt <= 50
    bool result = sensor.read_data(30, frequency, counter_out);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, frequency);  // frequency is 0 when dt <= 50
}

void test_speed_sensor_interrupt_read_data_after_minimum_interval(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);
    sensor.signal();

    // Read at exactly 51ms should work and calculate frequency
    bool result = sensor.read_data(51, frequency, counter_out);
    TEST_ASSERT_TRUE(result);
    // 1 signal in 51ms = ~19.6 Hz
    TEST_ASSERT_TRUE(frequency > 0.0);
}

void test_speed_sensor_interrupt_read_data_resets_counter(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    // Generate some signals
    sensor.signal();
    sensor.signal();

    sensor.read_data(0, frequency, counter_out);
    sensor.read_data(100, frequency, counter_out);  // Read and reset counter

    // No new signals
    bool result = sensor.read_data(200, frequency, counter_out);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, counter_out);  // Counter should be 0
}

void test_speed_sensor_interrupt_read_data_negative_pin_returns_false(void)
{
    SpeedSensorInterrupt sensor(-1, 0);
    double frequency = 999.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);
    bool result = sensor.read_data(100, frequency, counter_out);

    TEST_ASSERT_FALSE(result);  // Should return false for negative pin
}

void test_speed_sensor_interrupt_read_data_accumulates_between_reads(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);

    // Add signals over time
    sensor.signal();
    sensor.signal();
    sensor.signal();
    sensor.signal();

    // Now read
    bool result = sensor.read_data(100, frequency, counter_out);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(4, counter_out);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 40.0, frequency);  // 4 signals / 100ms = 40 Hz
}

#pragma endregion

#pragma region Smoothing Tests (Alpha Filter)

void test_speed_sensor_interrupt_smoothing_alpha_1_no_smoothing(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    sensor.set_alpha(1.0);  // No smoothing
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);

    // First reading: 2 signals in 100ms = 20 Hz
    sensor.signal();
    sensor.signal();
    sensor.read_data(100, frequency, counter_out);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 20.0, frequency);

    // Second reading: 4 signals in 100ms = 40 Hz (immediate jump with alpha=1)
    sensor.signal();
    sensor.signal();
    sensor.signal();
    sensor.signal();
    sensor.read_data(200, frequency, counter_out);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 40.0, frequency);
}

void test_speed_sensor_interrupt_smoothing_alpha_0_full_smoothing(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    sensor.set_alpha(0.0);  // Full smoothing (no change)
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);

    // First reading
    sensor.signal();
    sensor.signal();
    sensor.read_data(100, frequency, counter_out);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, frequency);  // Should stay at 0

    // Second reading with more signals
    sensor.signal();
    sensor.signal();
    sensor.signal();
    sensor.signal();
    sensor.read_data(200, frequency, counter_out);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, frequency);  // Should still be 0
}

void test_speed_sensor_interrupt_smoothing_alpha_05(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    sensor.set_alpha(0.5);  // 50% smoothing
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);

    // First reading: 2 signals in 100ms
    sensor.signal();
    sensor.signal();
    sensor.read_data(100, frequency, counter_out);
    double first_frequency = frequency;
    
    // smooth_counter = 2 * 0.5 + 0 * 0.5 = 1
    // frequency = 1 * 1000 / 100 = 10 Hz
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 10.0, first_frequency);

    // Second reading: 2 signals in 100ms
    sensor.signal();
    sensor.signal();
    sensor.read_data(200, frequency, counter_out);
    
    // smooth_counter = 2 * 0.5 + 1 * 0.5 = 1.5
    // frequency = 1.5 * 1000 / 100 = 15 Hz
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 15.0, frequency);
}

#pragma endregion

#pragma region Sample Age Tests

void test_speed_sensor_interrupt_sample_age_initial(void)
{
    SpeedSensorInterrupt sensor(25, 0);

    TEST_ASSERT_EQUAL_UINT32(0, sensor.get_sample_age());
}

void test_speed_sensor_interrupt_sample_age_after_read(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(1000, frequency, counter_out);

    TEST_ASSERT_EQUAL_UINT32(1000, sensor.get_sample_age());
}

void test_speed_sensor_interrupt_sample_age_updates(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(100, frequency, counter_out);
    TEST_ASSERT_EQUAL_UINT32(100, sensor.get_sample_age());

    sensor.read_data(500, frequency, counter_out);
    TEST_ASSERT_EQUAL_UINT32(500, sensor.get_sample_age());

    sensor.read_data(1500, frequency, counter_out);
    TEST_ASSERT_EQUAL_UINT32(1500, sensor.get_sample_age());
}

#pragma endregion

#pragma region Frequency Calculation Tests

void test_speed_sensor_interrupt_frequency_calculation_1hz(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);

    // 1 signal in 1000ms = 1 Hz
    sensor.signal();
    sensor.read_data(1000, frequency, counter_out);

    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, frequency);
}

void test_speed_sensor_interrupt_frequency_calculation_10hz(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);

    // 10 signals in 1000ms = 10 Hz
    for (int i = 0; i < 10; i++)
    {
        sensor.signal();
    }
    sensor.read_data(1000, frequency, counter_out);

    TEST_ASSERT_DOUBLE_WITHIN(0.1, 10.0, frequency);
}

void test_speed_sensor_interrupt_frequency_calculation_high_frequency(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);

    // 500 signals in 1000ms = 500 Hz
    for (int i = 0; i < 500; i++)
    {
        sensor.signal();
    }
    sensor.read_data(1000, frequency, counter_out);

    TEST_ASSERT_DOUBLE_WITHIN(1.0, 500.0, frequency);
}

void test_speed_sensor_interrupt_frequency_zero_when_no_signal(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);
    sensor.read_data(1000, frequency, counter_out);

    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, frequency);
}

#pragma endregion

#pragma region Edge Cases

void test_speed_sensor_interrupt_very_long_interval(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);

    sensor.signal();
    sensor.read_data(10000, frequency, counter_out);  // 10 second interval

    // 1 signal in 10000ms = 0.1 Hz
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.1, frequency);
}

void test_speed_sensor_interrupt_counter_output_before_reset(void)
{
    SpeedSensorInterrupt sensor(25, 0);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.signal();
    sensor.signal();
    sensor.signal();

    sensor.read_data(0, frequency, counter_out);
    TEST_ASSERT_EQUAL_INT(3, counter_out);

    sensor.read_data(100, frequency, counter_out);
    TEST_ASSERT_EQUAL_INT(0, counter_out);  // After second read, counter should be reset
}

void test_speed_sensor_interrupt_multiple_sensors_independent(void)
{
    SpeedSensorInterrupt sensor0(10, 0);
    SpeedSensorInterrupt sensor1(11, 1);
    double frequency0 = 0.0;
    double frequency1 = 0.0;
    int counter_out0 = 0;
    int counter_out1 = 0;

    sensor0.read_data(0, frequency0, counter_out0);
    sensor1.read_data(0, frequency1, counter_out1);

    // Different number of signals for each sensor
    sensor0.signal();
    sensor0.signal();
    
    sensor1.signal();
    sensor1.signal();
    sensor1.signal();
    sensor1.signal();
    sensor1.signal();

    sensor0.read_data(100, frequency0, counter_out0);
    sensor1.read_data(100, frequency1, counter_out1);

    TEST_ASSERT_EQUAL_INT(2, counter_out0);
    TEST_ASSERT_EQUAL_INT(5, counter_out1);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 20.0, frequency0);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 50.0, frequency1);
}

#pragma endregion

// Test runner
void setup()
{
    UNITY_BEGIN();

    // Constructor
    RUN_TEST(test_speed_sensor_interrupt_constructor_default);
    RUN_TEST(test_speed_sensor_interrupt_constructor_negative_pin);
    RUN_TEST(test_speed_sensor_interrupt_constructor_different_pins);
    RUN_TEST(test_speed_sensor_interrupt_constructor_different_instances);
    RUN_TEST(test_speed_sensor_interrupt_destructor_cleans_up);

    // Setup
    RUN_TEST(test_speed_sensor_interrupt_setup_no_crash);
    RUN_TEST(test_speed_sensor_interrupt_setup_negative_pin_no_crash);
    RUN_TEST(test_speed_sensor_interrupt_setup_all_instances);

    // Alpha
    RUN_TEST(test_speed_sensor_interrupt_set_alpha);
    RUN_TEST(test_speed_sensor_interrupt_set_alpha_zero);
    RUN_TEST(test_speed_sensor_interrupt_set_alpha_one);
    RUN_TEST(test_speed_sensor_interrupt_set_alpha_fractional);

    // Signal
    RUN_TEST(test_speed_sensor_interrupt_signal_increments_counter);
    RUN_TEST(test_speed_sensor_interrupt_multiple_signals);
    RUN_TEST(test_speed_sensor_interrupt_no_signals);
    RUN_TEST(test_speed_sensor_interrupt_rapid_signals);

    // Read Data
    RUN_TEST(test_speed_sensor_interrupt_read_data_returns_counter);
    RUN_TEST(test_speed_sensor_interrupt_read_data_too_soon);
    RUN_TEST(test_speed_sensor_interrupt_read_data_after_minimum_interval);
    RUN_TEST(test_speed_sensor_interrupt_read_data_resets_counter);
    RUN_TEST(test_speed_sensor_interrupt_read_data_negative_pin_returns_false);
    RUN_TEST(test_speed_sensor_interrupt_read_data_accumulates_between_reads);

    // Smoothing
    RUN_TEST(test_speed_sensor_interrupt_smoothing_alpha_1_no_smoothing);
    RUN_TEST(test_speed_sensor_interrupt_smoothing_alpha_0_full_smoothing);
    RUN_TEST(test_speed_sensor_interrupt_smoothing_alpha_05);

    // Sample Age
    RUN_TEST(test_speed_sensor_interrupt_sample_age_initial);
    RUN_TEST(test_speed_sensor_interrupt_sample_age_after_read);
    RUN_TEST(test_speed_sensor_interrupt_sample_age_updates);

    // Frequency Calculation
    RUN_TEST(test_speed_sensor_interrupt_frequency_calculation_1hz);
    RUN_TEST(test_speed_sensor_interrupt_frequency_calculation_10hz);
    RUN_TEST(test_speed_sensor_interrupt_frequency_calculation_high_frequency);
    RUN_TEST(test_speed_sensor_interrupt_frequency_zero_when_no_signal);

    // Edge Cases
    RUN_TEST(test_speed_sensor_interrupt_very_long_interval);
    RUN_TEST(test_speed_sensor_interrupt_counter_output_before_reset);
    RUN_TEST(test_speed_sensor_interrupt_multiple_sensors_independent);

    UNITY_END();
}

void loop()
{
}

int main()
{
    setup();
    return 0;
}
