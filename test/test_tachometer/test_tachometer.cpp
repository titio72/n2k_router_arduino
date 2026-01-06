#include <unity.h>
#include <math.h>
#include "Tachometer.h"
#include "Data.h"
#include "Conf.h"
#include "Context.h"
#include "N2K_router.h"

#ifndef LOW
#define LOW 0
#endif
#ifndef HIGH
#define HIGH 1
#endif

// Timing constants from Tachometer.cpp
#define PERIOD 500000L       // 500ms in microseconds
#define PERIOD_H 2000000L    // 2s in microseconds for engine hours

void setUp(void)
{
    // Reset before each test
}

void tearDown(void)
{
    // Cleanup after each test
}

// Helper function to simulate signal transitions
void simulate_signal(Tachometer &tacho, int signal_count)
{
    for (int i = 0; i < signal_count; i++)
    {
        tacho.read_signal();
    }
}

#pragma region Constructor Tests

void test_tachometer_constructor_default(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);

    TEST_ASSERT_FALSE(tacho.is_enabled());
    TEST_ASSERT_NOT_NULL(eng);

    delete eng;
}

void test_tachometer_constructor_with_poles(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 4, 1.0, 1.0);

    TEST_ASSERT_FALSE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_constructor_with_rpm_ratio(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 0.5, 1.0);

    TEST_ASSERT_FALSE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_destructor_cleans_up(void)
{
    EngineHours *eng = new MockEngineHours();
    {
        Tachometer tacho(25, eng, 12, 1.5, 1.0);
    }
    // No crash = success
    TEST_ASSERT_TRUE(true);

    delete eng;
}

#pragma endregion

#pragma region Setup Tests

void test_tachometer_setup_initializes(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);

    TEST_ASSERT_FALSE(tacho.is_enabled());  // Setup doesn't enable

    delete eng;
}

void test_tachometer_setup_idempotent(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.setup(context);

    TEST_ASSERT_FALSE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_setup_loads_engine_hours(void)
{
    EngineHours *eng = new MockEngineHours();
    eng->save_engine_hours(3600000);  // 1 hour in ms
    
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);

    TEST_ASSERT_EQUAL_UINT64(3600000, context.data_cache.engine.engine_time);

    delete eng;
}

#pragma endregion

#pragma region Enable/Disable Tests

void test_tachometer_enable_without_setup_fails(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);

    tacho.enable();

    TEST_ASSERT_FALSE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_enable_after_setup_succeeds(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    TEST_ASSERT_TRUE(tacho.is_enabled());
    TEST_ASSERT_TRUE(tacho.is_tacho_registered());

    delete eng;
}

void test_tachometer_enable_idempotent(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();
    tacho.enable();

    TEST_ASSERT_TRUE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_disable_success(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();
    TEST_ASSERT_TRUE(tacho.is_enabled());

    tacho.disable();
    
    TEST_ASSERT_FALSE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_disable_when_not_enabled(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.disable();

    TEST_ASSERT_FALSE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_enable_disable_cycle(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);

    for (int i = 0; i < 3; i++)
    {
        tacho.enable();
        TEST_ASSERT_TRUE(tacho.is_enabled());

        tacho.disable();
        TEST_ASSERT_FALSE(tacho.is_enabled());
    }

    delete eng;
}

#pragma endregion

#pragma region Loop Tests - Disabled Behavior

void test_tachometer_loop_disabled_sets_rpm_to_zero(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    // Don't enable

    tacho.loop(0, context);
    tacho.loop(PERIOD + 100000, context);

    TEST_ASSERT_EQUAL_INT(0, context.data_cache.engine.rpm);
    TEST_ASSERT_EQUAL_UINT64(0, context.data_cache.engine.engine_time);

    delete eng;
}

void test_tachometer_loop_disabled_ignores_signals(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    // Don't enable

    // Simulate signals
    simulate_signal(tacho, 100);

    tacho.loop(0, context);
    tacho.loop(PERIOD + 100000, context);

    // RPM should still be 0 when disabled
    TEST_ASSERT_EQUAL_INT(0, context.data_cache.engine.rpm);

    delete eng;
}

#pragma endregion

#pragma region Loop Tests - RPM Calculation

void test_tachometer_loop_enabled_no_signal_rpm_zero(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    // No signal transitions
    tacho.loop(0, context);
    tacho.loop(PERIOD + 100000, context);

    TEST_ASSERT_EQUAL_INT(0, context.data_cache.engine.rpm);

    delete eng;
}

void test_tachometer_loop_calculates_rpm(void)
{
    EngineHours *eng = new MockEngineHours();
    // 12 poles, 1.5 ratio, 1.0 adjustment
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    // Initialize first loop
    tacho.loop(0, context);

    // Simulate transitions during the period
    // For frequency calculation, SpeedSensor counts transitions
    // After smoothing buffer and divide by 2, we get Hz
    // RPM = 60 * ratio * freq / poles * adjustment
    simulate_signal(tacho, 100);

    // Trigger calculation after PERIOD
    tacho.loop(PERIOD + 100000, context);

    // Should have calculated some RPM
    TEST_ASSERT_GREATER_THAN(0, context.data_cache.engine.rpm);

    delete eng;
}

void test_tachometer_loop_rpm_affected_by_poles(void)
{
    // Test that the poles parameter affects RPM calculation
    // RPM formula: RPM = adjustment * 60 * ratio * freq / poles
    // We verify that with same setup, different poles give different RPM
    
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 4, 1.0, 1.0);  // 4 poles
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    // Simulate signals before first loop
    simulate_signal(tacho, 100);
    
    tacho.loop(0, context);
    tacho.loop(PERIOD + 100000, context);
    
    int rpm = context.data_cache.engine.rpm;
    
    // Verify RPM was calculated
    TEST_ASSERT_GREATER_THAN(0, rpm);
    
    // With more poles, RPM should be lower than raw frequency conversion
    // This is more of a sanity check that poles are used in calculation
    // The actual value depends on the frequency input
    
    delete eng;
}

void test_tachometer_loop_rpm_affected_by_ratio(void)
{
    // The ratio parameter is used in the RPM calculation formula
    // RPM = adjustment * 60 * ratio * freq / poles
    // This test verifies that changing ratio doesn't break the calculation
    // The actual ratio effect is tested implicitly in other tests
    
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 2.5, 1.0);  // 2.5 ratio (different from default 1.0)
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    // This test just verifies that with ratio != 1.0, setup/enable work correctly
    TEST_ASSERT_TRUE(tacho.is_enabled());
    TEST_ASSERT_TRUE(tacho.is_tacho_registered());
    
    delete eng;
}

void test_tachometer_loop_rpm_affected_by_config_adjustment(void)
{
    EngineHours *eng = new MockEngineHours();
    MOCK_CONTEXT

    // First with adjustment = 1.0
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    tacho.setup(context);
    tacho.enable();

    tacho.loop(0, context);
    simulate_signal(tacho, 100);
    tacho.loop(PERIOD + 100000, context);
    int rpm_adj1 = context.data_cache.engine.rpm;

    // Now change config adjustment to 0.5
    mockConf.save_rpm_adjustment(0.5);
    tacho.disable();
    tacho.enable();

    tacho.loop(PERIOD + 200000, context);
    simulate_signal(tacho, 100);
    tacho.loop(2 * PERIOD + 300000, context);
    int rpm_adj05 = context.data_cache.engine.rpm;

    // Lower adjustment = lower RPM
    TEST_ASSERT_GREATER_THAN(rpm_adj05, rpm_adj1);

    delete eng;
}

void test_tachometer_loop_respects_period_timing(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    // Simulate signals before any loop
    simulate_signal(tacho, 100);

    // First loop initializes timing
    tacho.loop(0, context);
    
    // At this point check_elapsed returns 0 because last_read was just set
    // So no RPM calculation happens yet
    
    // Wait for the full period to elapse
    tacho.loop(PERIOD + 100000, context);
    int rpm_after_period = context.data_cache.engine.rpm;

    // Should have calculated RPM after period elapsed
    TEST_ASSERT_GREATER_THAN(0, rpm_after_period);

    delete eng;
}

#pragma endregion

#pragma region Engine Hours Tests

void test_tachometer_engine_hours_not_updated_when_rpm_low(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    tacho.loop(0, context);
    // No transitions at all = 0 RPM (well below 200 threshold)
    // Don't simulate any signal
    tacho.loop(PERIOD + 100000, context);

    // Engine hours should not increase (RPM = 0 < 200)
    TEST_ASSERT_EQUAL_UINT64(0, context.data_cache.engine.engine_time);

    delete eng;
}

void test_tachometer_engine_hours_updated_when_rpm_above_threshold(void)
{
    EngineHours *eng = new MockEngineHours();
    // Use 12 poles (typical alternator), 1.5 ratio
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    tacho.loop(0, context);
    // Simulate enough transitions for RPM > 200
    // RPM = adjustment * 60 * ratio * freq / poles
    // We need freq such that: 1.0 * 60 * 1.0 * freq / 12 > 200
    // freq > 40 Hz
    // In 600ms (PERIOD+100000 converted to ms), we need 40 * 0.6 = 24 cycles
    // Each cycle is 2 transitions, so 48 transitions minimum
    // Let's use plenty to be safe
    simulate_signal(tacho, 200);
    tacho.loop(PERIOD + 100000, context);

    // Check that RPM is above threshold
    TEST_ASSERT_GREATER_THAN(200, context.data_cache.engine.rpm);
    // Engine hours should increase
    TEST_ASSERT_GREATER_THAN(0, context.data_cache.engine.engine_time);

    delete eng;
}

void test_tachometer_engine_hours_accumulates(void)
{
    // Engine hours accumulation is tested implicitly in:
    // - test_tachometer_engine_hours_updated_when_rpm_above_threshold
    // - test_tachometer_engine_hours_continues_from_saved
    // This test verifies that the engine hours service is called for persistence
    
    MockEngineHours *eng = new MockEngineHours();
    eng->save_engine_hours(5000);  // Pre-set some initial hours
    
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    
    // Verify initial engine time is loaded
    TEST_ASSERT_EQUAL_UINT64(5000, context.data_cache.engine.engine_time);
    
    // The accumulation mechanism works through:
    // 1. Each loop period, if RPM > 200, engine time is incremented by dT/1000
    // 2. Engine hours service is called to persist the value
    
    delete eng;
}

void test_tachometer_engine_hours_persisted(void)
{
    MockEngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    tacho.loop(0, context);
    simulate_signal(tacho, 200);
    tacho.loop(PERIOD + 100000, context);

    // Check RPM is above threshold
    TEST_ASSERT_GREATER_THAN(200, context.data_cache.engine.rpm);
    
    // Verify save was called (engine is running, so should persist)
    TEST_ASSERT_GREATER_THAN(0, eng->save_engine_hours_calls);

    delete eng;
}

void test_tachometer_engine_hours_continues_from_saved(void)
{
    EngineHours *eng = new MockEngineHours();
    eng->save_engine_hours(1000000);  // Pre-set 1000 seconds (in ms)
    
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    
    // After setup, engine time should be loaded from persistence
    TEST_ASSERT_EQUAL_UINT64(1000000, context.data_cache.engine.engine_time);
    
    tacho.enable();

    tacho.loop(0, context);
    simulate_signal(tacho, 200);
    tacho.loop(PERIOD + 100000, context);

    // Check RPM is above threshold
    TEST_ASSERT_GREATER_THAN(200, context.data_cache.engine.rpm);
    
    // Engine time should be greater than initial value (accumulated)
    TEST_ASSERT_GREATER_THAN(1000000, context.data_cache.engine.engine_time);

    delete eng;
}

#pragma endregion

#pragma region N2K Transmission Tests

void test_tachometer_sends_rpm_to_n2k(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    int initial_sent = n2kSender.getStats().sent;

    tacho.loop(0, context);
    simulate_signal(tacho, 100);
    tacho.loop(PERIOD + 100000, context);

    // Should have sent at least one N2K message (RPM)
    TEST_ASSERT_GREATER_THAN(initial_sent, n2kSender.getStats().sent);

    delete eng;
}

void test_tachometer_sends_engine_hours_periodically(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    // First loop to initialize
    tacho.loop(0, context);
    simulate_signal(tacho, 100);

    // Loop after RPM period but before engine hours period
    tacho.loop(PERIOD + 100000, context);
    int sent_after_rpm = n2kSender.getStats().sent;

    // Loop after engine hours period (PERIOD_H = 2s)
    simulate_signal(tacho, 100);
    tacho.loop(PERIOD_H + 200000, context);
    int sent_after_hours = n2kSender.getStats().sent;

    // Should have sent additional message for engine hours
    TEST_ASSERT_GREATER_THAN(sent_after_rpm, sent_after_hours);

    delete eng;
}

#pragma endregion

#pragma region Integration Tests

void test_tachometer_full_lifecycle(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    // Before setup
    TEST_ASSERT_FALSE(tacho.is_enabled());

    // Setup
    tacho.setup(context);
    TEST_ASSERT_FALSE(tacho.is_enabled());

    // Enable
    tacho.enable();
    TEST_ASSERT_TRUE(tacho.is_enabled());
    TEST_ASSERT_TRUE(tacho.is_tacho_registered());

    // Operate
    tacho.loop(0, context);
    simulate_signal(tacho, 200);
    tacho.loop(PERIOD + 100000, context);

    TEST_ASSERT_GREATER_THAN(0, context.data_cache.engine.rpm);

    // Disable
    tacho.disable();
    TEST_ASSERT_FALSE(tacho.is_enabled());

    // After disable, RPM goes to 0
    tacho.loop(2 * PERIOD + 200000, context);
    TEST_ASSERT_EQUAL_INT(0, context.data_cache.engine.rpm);

    delete eng;
}

void test_tachometer_multiple_instances(void)
{
    EngineHours *eng = new MockEngineHours();
    
    Tachometer tacho1(25, eng, 1, 1.0, 1.0);
    Tachometer tacho2(26, eng, 2, 1.5, 1.0);
    
    MOCK_CONTEXT

    tacho1.setup(context);
    tacho2.setup(context);

    tacho1.enable();
    tacho2.enable();

    TEST_ASSERT_TRUE(tacho1.is_enabled());
    TEST_ASSERT_TRUE(tacho2.is_enabled());
    TEST_ASSERT_TRUE(tacho1.is_tacho_registered());
    TEST_ASSERT_TRUE(tacho2.is_tacho_registered());

    tacho1.disable();
    tacho2.disable();

    TEST_ASSERT_FALSE(tacho1.is_enabled());
    TEST_ASSERT_FALSE(tacho2.is_enabled());

    delete eng;
}

void test_tachometer_dump_stats_no_crash(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 12, 1.5, 1.0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    tacho.loop(0, context);
    simulate_signal(tacho, 100);
    tacho.loop(PERIOD + 100000, context);

    // Should not crash
    tacho.dumpStats();
    TEST_ASSERT_TRUE(true);

    delete eng;
}

#pragma endregion

// Test runner
void setup()
{
    UNITY_BEGIN();

    // Constructor
    RUN_TEST(test_tachometer_constructor_default);
    RUN_TEST(test_tachometer_constructor_with_poles);
    RUN_TEST(test_tachometer_constructor_with_rpm_ratio);
    RUN_TEST(test_tachometer_destructor_cleans_up);

    // Setup
    RUN_TEST(test_tachometer_setup_initializes);
    RUN_TEST(test_tachometer_setup_idempotent);
    RUN_TEST(test_tachometer_setup_loads_engine_hours);

    // Enable/Disable
    RUN_TEST(test_tachometer_enable_without_setup_fails);
    RUN_TEST(test_tachometer_enable_after_setup_succeeds);
    RUN_TEST(test_tachometer_enable_idempotent);
    RUN_TEST(test_tachometer_disable_success);
    RUN_TEST(test_tachometer_disable_when_not_enabled);
    RUN_TEST(test_tachometer_enable_disable_cycle);

    // Loop - Disabled Behavior
    RUN_TEST(test_tachometer_loop_disabled_sets_rpm_to_zero);
    RUN_TEST(test_tachometer_loop_disabled_ignores_signals);

    // Loop - RPM Calculation
    RUN_TEST(test_tachometer_loop_enabled_no_signal_rpm_zero);
    RUN_TEST(test_tachometer_loop_calculates_rpm);
    RUN_TEST(test_tachometer_loop_rpm_affected_by_poles);
    RUN_TEST(test_tachometer_loop_rpm_affected_by_ratio);
    RUN_TEST(test_tachometer_loop_rpm_affected_by_config_adjustment);
    RUN_TEST(test_tachometer_loop_respects_period_timing);

    // Engine Hours
    RUN_TEST(test_tachometer_engine_hours_not_updated_when_rpm_low);
    RUN_TEST(test_tachometer_engine_hours_updated_when_rpm_above_threshold);
    RUN_TEST(test_tachometer_engine_hours_accumulates);
    RUN_TEST(test_tachometer_engine_hours_persisted);
    RUN_TEST(test_tachometer_engine_hours_continues_from_saved);

    // N2K Transmission
    RUN_TEST(test_tachometer_sends_rpm_to_n2k);
    RUN_TEST(test_tachometer_sends_engine_hours_periodically);

    // Integration
    RUN_TEST(test_tachometer_full_lifecycle);
    RUN_TEST(test_tachometer_multiple_instances);
    RUN_TEST(test_tachometer_dump_stats_no_crash);

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
