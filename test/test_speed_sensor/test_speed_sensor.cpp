#include <unity.h>
#include <math.h>
#include "SpeedSensor.h"

#ifndef LOW
#define LOW 0
#endif
#ifndef HIGH
#define HIGH 1
#endif

void setUp(void)
{
    // Reset before each test
}

void tearDown(void)
{
    // Cleanup after each test
}

#pragma region Constructor Tests

void test_speed_sensor_constructor_default(void)
{
    SpeedSensor sensor(25);

    TEST_ASSERT_EQUAL_INT(25, sensor.get_pin());
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 1.0, sensor.get_alpha());
}

void test_speed_sensor_constructor_negative_pin(void)
{
    SpeedSensor sensor(-1);

    TEST_ASSERT_EQUAL_INT(-1, sensor.get_pin());
}

void test_speed_sensor_constructor_different_pins(void)
{
    SpeedSensor sensor1(10);
    SpeedSensor sensor2(20);
    SpeedSensor sensor3(30);

    TEST_ASSERT_EQUAL_INT(10, sensor1.get_pin());
    TEST_ASSERT_EQUAL_INT(20, sensor2.get_pin());
    TEST_ASSERT_EQUAL_INT(30, sensor3.get_pin());
}

void test_speed_sensor_destructor_cleans_up(void)
{
    {
        SpeedSensor sensor(25);
    }
    // No crash = success
    TEST_ASSERT_TRUE(true);
}

#pragma endregion

#pragma region Setup Tests

void test_speed_sensor_setup_no_crash(void)
{
    SpeedSensor sensor(25);
    sensor.setup();
    // No crash = success
    TEST_ASSERT_TRUE(true);
}

void test_speed_sensor_setup_negative_pin_no_crash(void)
{
    SpeedSensor sensor(-1);
    sensor.setup();
    // Should not crash even with negative pin
    TEST_ASSERT_TRUE(true);
}

#pragma endregion

#pragma region Alpha Tests

void test_speed_sensor_set_alpha(void)
{
    SpeedSensor sensor(25);

    sensor.set_alpha(0.5);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.5, sensor.get_alpha());
}

void test_speed_sensor_set_alpha_zero(void)
{
    SpeedSensor sensor(25);

    sensor.set_alpha(0.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, sensor.get_alpha());
}

void test_speed_sensor_set_alpha_one(void)
{
    SpeedSensor sensor(25);

    sensor.set_alpha(1.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 1.0, sensor.get_alpha());
}

void test_speed_sensor_set_alpha_fractional(void)
{
    SpeedSensor sensor(25);

    sensor.set_alpha(0.25);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.25, sensor.get_alpha());

    sensor.set_alpha(0.75);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.75, sensor.get_alpha());
}

#pragma endregion

#pragma region Signal Reading Tests

void test_speed_sensor_read_signal_low_to_high(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    // Initialize timing first
    sensor.read_data(0, frequency, counter_out);

    // Initial state is LOW, so transition to HIGH should count
    sensor.read_signal(HIGH);

    // Read 100ms later
    bool result = sensor.read_data(100, frequency, counter_out);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 10.0, frequency);  // 1 transition / 100ms = 10 Hz
}

void test_speed_sensor_read_signal_high_to_low(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize timing

    // Transition LOW -> HIGH -> LOW = 2 counts
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);

    bool result = sensor.read_data(100, frequency, counter_out);  // 100ms later
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 20.0, frequency);  // 2 transitions / 100ms = 20 Hz
}

void test_speed_sensor_read_signal_no_change(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    // No transitions (LOW -> LOW)
    sensor.read_signal(LOW);
    sensor.read_signal(LOW);
    sensor.read_signal(LOW);

    sensor.read_data(0, frequency, counter_out);  // Initialize timing
    bool result = sensor.read_data(100, frequency, counter_out);  // 100ms later
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, frequency);  // No transitions = 0 Hz
}

void test_speed_sensor_read_signal_multiple_transitions(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize timing

    // 4 transitions
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);

    bool result = sensor.read_data(100, frequency, counter_out);  // 100ms later
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 40.0, frequency);  // 4 transitions / 100ms = 40 Hz
}

void test_speed_sensor_read_signal_rapid_transitions(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize timing

    // Many rapid transitions
    for (int i = 0; i < 10; i++)
    {
        sensor.read_signal(HIGH);
        sensor.read_signal(LOW);
    }

    bool result = sensor.read_data(100, frequency, counter_out);  // 100ms later
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 200.0, frequency);  // 20 transitions / 100ms = 200 Hz
}

#pragma endregion

#pragma region Read Data Tests

void test_speed_sensor_read_data_too_soon(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize
    sensor.read_signal(HIGH);

    // Read too soon (less than 50ms) - returns true but frequency is 0
    bool result = sensor.read_data(30, frequency, counter_out);
    TEST_ASSERT_TRUE(result);  // Now returns true for valid pin
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, frequency);  // But frequency is 0 when dt <= 50
}

void test_speed_sensor_read_data_after_minimum_interval(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_signal(HIGH);
    sensor.read_data(0, frequency, counter_out);  // Initialize

    // Read at exactly 51ms should work
    bool result = sensor.read_data(51, frequency, counter_out);
    TEST_ASSERT_TRUE(result);
}

void test_speed_sensor_read_data_resets_counter(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    // Generate some transitions
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);

    sensor.read_data(0, frequency, counter_out);  // Initialize
    sensor.read_data(100, frequency, counter_out);  // Read and reset counter

    // No new transitions
    bool result = sensor.read_data(200, frequency, counter_out);  // 100ms later
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, frequency);  // Should be 0 as counter was reset
}

void test_speed_sensor_read_data_negative_pin_returns_false(void)
{
    SpeedSensor sensor(-1);
    double frequency = 999.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize
    bool result = sensor.read_data(100, frequency, counter_out);  // Try to read

    TEST_ASSERT_FALSE(result);  // Should return false for negative pin
}

void test_speed_sensor_read_data_accumulates_between_reads(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize

    // Add transitions over time
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);

    // Wait more than 50ms but don't read yet
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);

    // Now read
    bool result = sensor.read_data(100, frequency, counter_out);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 40.0, frequency);  // 4 transitions / 100ms = 40 Hz
}

#pragma endregion

#pragma region Smoothing Tests (Alpha Filter)

void test_speed_sensor_smoothing_alpha_1_no_smoothing(void)
{
    SpeedSensor sensor(25);
    sensor.set_alpha(1.0);  // No smoothing
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize

    // First reading: 2 transitions in 100ms = 20 Hz
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);
    sensor.read_data(100, frequency, counter_out);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 20.0, frequency);

    // Second reading: 4 transitions in 100ms = 40 Hz (no smoothing)
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);
    sensor.read_data(200, frequency, counter_out);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 40.0, frequency);
}

void test_speed_sensor_smoothing_alpha_0_full_smoothing(void)
{
    SpeedSensor sensor(25);
    sensor.set_alpha(0.0);  // Full smoothing (keeps previous value)
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize

    // First reading: 2 transitions in 100ms = 20 Hz
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);
    sensor.read_data(100, frequency, counter_out);
    // With alpha=0, smooth_counter = counter * 0 + smooth_counter * 1 = 0
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 0.0, frequency);

    // Second reading: even with new transitions, frequency stays at 0
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);
    sensor.read_data(200, frequency, counter_out);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 0.0, frequency);
}

void test_speed_sensor_smoothing_alpha_05(void)
{
    SpeedSensor sensor(25);
    sensor.set_alpha(0.5);  // 50% smoothing
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize

    // First reading: 2 transitions in 100ms
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);
    sensor.read_data(100, frequency, counter_out);
    // smooth_counter = 2 * 0.5 + 0 * 0.5 = 1
    // frequency = 1 * 1000 / 100 = 10 Hz
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 10.0, frequency);

    // Second reading: 2 transitions in 100ms
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);
    sensor.read_data(200, frequency, counter_out);
    // smooth_counter = 2 * 0.5 + 1 * 0.5 = 1.5
    // frequency = 1.5 * 1000 / 100 = 15 Hz
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 15.0, frequency);
}

#pragma endregion

#pragma region Sample Age Tests

void test_speed_sensor_sample_age_initial(void)
{
    SpeedSensor sensor(25);

    TEST_ASSERT_EQUAL_UINT32(0, sensor.get_sample_age());
}

void test_speed_sensor_sample_age_after_read(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(100, frequency, counter_out);

    TEST_ASSERT_EQUAL_UINT32(100, sensor.get_sample_age());
}

void test_speed_sensor_sample_age_updates(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(100, frequency, counter_out);
    TEST_ASSERT_EQUAL_UINT32(100, sensor.get_sample_age());

    sensor.read_data(200, frequency, counter_out);  // Too soon, but still updates last_read_time? Let's check
    // Actually read_data only updates when dt > 50
    // So at 200ms (dt=100) it should update
    TEST_ASSERT_EQUAL_UINT32(200, sensor.get_sample_age());
}

#pragma endregion

#pragma region Frequency Calculation Tests

void test_speed_sensor_frequency_calculation_1hz(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize

    // 1 complete cycle = 2 transitions (LOW->HIGH->LOW)
    // For 1 Hz, we need 2 transitions per second
    // In 1000ms, we should have 2 transitions
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);

    sensor.read_data(1000, frequency, counter_out);
    // 2 transitions / 1000ms = 2 Hz (because we count transitions, not cycles)
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 2.0, frequency);
}

void test_speed_sensor_frequency_calculation_10hz(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize

    // For 10 Hz frequency (transitions), we need 10 transitions in 1000ms
    for (int i = 0; i < 5; i++)
    {
        sensor.read_signal(HIGH);
        sensor.read_signal(LOW);
    }

    sensor.read_data(1000, frequency, counter_out);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 10.0, frequency);
}

void test_speed_sensor_frequency_calculation_high_frequency(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize

    // High frequency: 100 transitions in 100ms = 1000 Hz
    for (int i = 0; i < 50; i++)
    {
        sensor.read_signal(HIGH);
        sensor.read_signal(LOW);
    }

    sensor.read_data(100, frequency, counter_out);
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 1000.0, frequency);
}

void test_speed_sensor_frequency_zero_when_no_signal(void)
{
    SpeedSensor sensor(25);
    double frequency = 999.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize

    // No transitions
    sensor.read_data(100, frequency, counter_out);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, frequency);
}

#pragma endregion

#pragma region Edge Cases

void test_speed_sensor_very_long_interval(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize

    // Some transitions
    sensor.read_signal(HIGH);
    sensor.read_signal(LOW);

    // Very long interval (10 seconds)
    sensor.read_data(10000, frequency, counter_out);
    // 2 transitions / 10000ms = 0.2 Hz
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.2, frequency);
}

void test_speed_sensor_consecutive_same_state_no_count(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize

    // Multiple HIGH readings should only count the first transition
    sensor.read_signal(HIGH);
    sensor.read_signal(HIGH);
    sensor.read_signal(HIGH);
    sensor.read_signal(HIGH);

    sensor.read_data(100, frequency, counter_out);
    // Only 1 transition (LOW -> HIGH)
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 10.0, frequency);
}

void test_speed_sensor_alternating_readings(void)
{
    SpeedSensor sensor(25);
    double frequency = 0.0;
    int counter_out = 0;

    sensor.read_data(0, frequency, counter_out);  // Initialize

    // Perfect alternating pattern
    for (int i = 0; i < 100; i++)
    {
        sensor.read_signal(i % 2);
    }

    sensor.read_data(1000, frequency, counter_out);
    // 100 transitions in 1000ms = 100 Hz
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 100.0, frequency);
}

#pragma endregion

// Test runner
void setup()
{
    UNITY_BEGIN();

    // Constructor
    RUN_TEST(test_speed_sensor_constructor_default);
    RUN_TEST(test_speed_sensor_constructor_negative_pin);
    RUN_TEST(test_speed_sensor_constructor_different_pins);
    RUN_TEST(test_speed_sensor_destructor_cleans_up);

    // Setup
    RUN_TEST(test_speed_sensor_setup_no_crash);
    RUN_TEST(test_speed_sensor_setup_negative_pin_no_crash);

    // Alpha
    RUN_TEST(test_speed_sensor_set_alpha);
    RUN_TEST(test_speed_sensor_set_alpha_zero);
    RUN_TEST(test_speed_sensor_set_alpha_one);
    RUN_TEST(test_speed_sensor_set_alpha_fractional);

    // Signal Reading
    RUN_TEST(test_speed_sensor_read_signal_low_to_high);
    RUN_TEST(test_speed_sensor_read_signal_high_to_low);
    RUN_TEST(test_speed_sensor_read_signal_no_change);
    RUN_TEST(test_speed_sensor_read_signal_multiple_transitions);
    RUN_TEST(test_speed_sensor_read_signal_rapid_transitions);

    // Read Data
    RUN_TEST(test_speed_sensor_read_data_too_soon);
    RUN_TEST(test_speed_sensor_read_data_after_minimum_interval);
    RUN_TEST(test_speed_sensor_read_data_resets_counter);
    RUN_TEST(test_speed_sensor_read_data_negative_pin_returns_false);
    RUN_TEST(test_speed_sensor_read_data_accumulates_between_reads);

    // Smoothing
    RUN_TEST(test_speed_sensor_smoothing_alpha_1_no_smoothing);
    RUN_TEST(test_speed_sensor_smoothing_alpha_0_full_smoothing);
    RUN_TEST(test_speed_sensor_smoothing_alpha_05);

    // Sample Age
    RUN_TEST(test_speed_sensor_sample_age_initial);
    RUN_TEST(test_speed_sensor_sample_age_after_read);
    RUN_TEST(test_speed_sensor_sample_age_updates);

    // Frequency Calculation
    RUN_TEST(test_speed_sensor_frequency_calculation_1hz);
    RUN_TEST(test_speed_sensor_frequency_calculation_10hz);
    RUN_TEST(test_speed_sensor_frequency_calculation_high_frequency);
    RUN_TEST(test_speed_sensor_frequency_zero_when_no_signal);

    // Edge Cases
    RUN_TEST(test_speed_sensor_very_long_interval);
    RUN_TEST(test_speed_sensor_consecutive_same_state_no_count);
    RUN_TEST(test_speed_sensor_alternating_readings);

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
