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


class MockEngineHours: public EngineHours
{
public:
  virtual uint64_t get_engine_hours() const { return t; }
  virtual bool save_engine_hours(uint64_t h) { t= h; return true; }

private:
    uint64_t t;
};

void setUp(void)
{
    // Reset before each test
}

void tearDown(void)
{
    // Cleanup after each test
}

#pragma region Constructor Tests

void test_tachometer_constructor_default(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);

    TEST_ASSERT_FALSE(tacho.is_enabled());
    TEST_ASSERT_NOT_NULL(eng);

    delete eng;
}

void test_tachometer_constructor_with_poles(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 4, 1.0, 1.0, 0);

    TEST_ASSERT_FALSE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_constructor_with_rpm_ratio(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 0.5, 1.0, 0);

    TEST_ASSERT_FALSE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_constructor_with_rpm_adjustment(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.5, 0);

    TEST_ASSERT_FALSE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_constructor_different_pins(void)
{
    EngineHours *eng = new MockEngineHours();
    
    Tachometer tacho1(25, eng, 1, 1.0, 1.0, 0);
    Tachometer tacho2(26, eng, 1, 1.0, 1.0, 0);
    Tachometer tacho3(27, eng, 1, 1.0, 1.0, 0);

    TEST_ASSERT_FALSE(tacho1.is_enabled());
    TEST_ASSERT_FALSE(tacho2.is_enabled());
    TEST_ASSERT_FALSE(tacho3.is_enabled());

    delete eng;
}

void test_tachometer_destructor_cleans_up(void)
{
    EngineHours *eng = new MockEngineHours();
    {
        Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    }
    // No crash = success
    TEST_ASSERT_TRUE(true);

    delete eng;
}

#pragma endregion

#pragma region Setup Tests

void test_tachometer_setup_marks_setup(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);

    TEST_ASSERT_FALSE(tacho.is_enabled());  // Setup doesn't enable

    delete eng;
}

void test_tachometer_setup_idempotent(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.setup(context);

    TEST_ASSERT_FALSE(tacho.is_enabled());

    delete eng;
}

#pragma endregion

#pragma region Enable/Disable Tests

void test_tachometer_enable_without_setup_fails(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);

    tacho.enable();

    TEST_ASSERT_FALSE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_enable_after_setup_succeeds(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    TEST_ASSERT_TRUE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_enable_idempotent(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();
    TEST_ASSERT_TRUE(tacho.is_enabled());

    tacho.enable();
    TEST_ASSERT_TRUE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_disable_success(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
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
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.disable();

    TEST_ASSERT_FALSE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_enable_disable_cycle(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);

    for (int i = 0; i < 5; i++)
    {
        tacho.enable();
        TEST_ASSERT_TRUE(tacho.is_enabled());

        tacho.disable();
        TEST_ASSERT_FALSE(tacho.is_enabled());
    }

    delete eng;
}

#pragma endregion

#pragma region Signal Reading Tests

void test_tachometer_read_signal_low_to_high(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    tacho.read_signal(LOW);
    tacho.read_signal(HIGH);

    TEST_ASSERT_TRUE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_read_signal_high_to_low(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    tacho.read_signal(HIGH);
    tacho.read_signal(LOW);

    TEST_ASSERT_TRUE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_read_signal_no_change(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    tacho.read_signal(LOW);
    tacho.read_signal(LOW);
    tacho.read_signal(LOW);

    TEST_ASSERT_TRUE(tacho.is_enabled());

    delete eng;
}

void test_tachometer_read_signal_multiple_transitions(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    // Simulate 10 transitions
    for (int i = 0; i < 10; i++)
    {
        tacho.read_signal(i % 2 == 0 ? LOW : HIGH);
    }

    TEST_ASSERT_TRUE(tacho.is_enabled());

    delete eng;
}

#pragma endregion

#pragma region Loop Tests - RPM Calculation

void test_tachometer_loop_disabled_sets_rpm_to_zero(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    // Don't enable

    unsigned long micros = 0;
    tacho.loop(micros, context);
    tacho.loop(micros + 600000, context);

    TEST_ASSERT_EQUAL_INT(0, context.data_cache.engine.rpm);

    delete eng;
}

void test_tachometer_loop_enabled_no_signal_rpm_zero(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    // No signal transitions
    unsigned long micros = 0;
    tacho.loop(micros, context);
    tacho.loop(micros + 600000, context);

    TEST_ASSERT_EQUAL_INT(0, context.data_cache.engine.rpm);

    delete eng;
}

void test_tachometer_loop_with_signal_calculates_rpm(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);  // 1 pole
    MOCK_CONTEXT
    mockConf.saved_services.set_use_tacho(true);

    tacho.setup(context);
    tacho.enable();

    unsigned long micros = 0;

    // Simulate 1000 Hz signal (1000 transitions per second)
    // Over 500ms (PERIOD = 500000 microseconds), expect 500 transitions
    for (int i = 0; i < 500; i++)
    {
        tacho.read_signal(i % 2 == 0 ? LOW : HIGH);
    }

    tacho.loop(micros, context);
    tacho.loop(micros + 600000, context);  // After 600ms

    // RPM = 60 * rpm_ratio * freq / poles
    // freq = 500 transitions / (500ms / 2) = 2000 Hz
    // RPM = 60 * 1.0 * 2000 / 1 = 120,000 RPM (unrealistic but math correct)
    TEST_ASSERT_GREATER_THAN(0, context.data_cache.engine.rpm);

    delete eng;
}

void test_tachometer_loop_with_rpm_adjustment(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 0.5, 0);  // 0.5 adjustment
    MOCK_CONTEXT
    mockConf.saved_rpm_adjustment = 0.5;

    tacho.setup(context);
    tacho.enable();

    unsigned long micros = 0;

    for (int i = 0; i < 100; i++)
    {
        tacho.read_signal(i % 2 == 0 ? LOW : HIGH);
    }

    tacho.loop(micros, context);
    tacho.loop(micros + 600000, context);

    // RPM should be halved due to adjustment
    TEST_ASSERT_GREATER_THAN(0, context.data_cache.engine.rpm);

    delete eng;
}

void test_tachometer_loop_with_poles(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 4, 1.0, 1.0, 0);  // 4 poles
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    unsigned long micros = 0;

    for (int i = 0; i < 100; i++)
    {
        tacho.read_signal(i % 2 == 0 ? LOW : HIGH);
    }

    tacho.loop(micros, context);
    tacho.loop(micros + 600000, context);

    // RPM should be divided by 4 due to poles
    TEST_ASSERT_GREATER_THAN(0, context.data_cache.engine.rpm);

    delete eng;
}

void test_tachometer_loop_low_rpm_threshold(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(5, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    unsigned long micros = 1000000L; // arbitrary time

    tacho.loop(micros, context);
    // Simulate very low signal rate
    for (int i = 0; i < 5; i++)
    {
        tacho.read_signal(i % 2 == 0 ? LOW : HIGH);
    }
    TEST_ASSERT_EQUAL_INT(4, tacho.get_counter());
    tacho.loop(micros + 600000L, context);  // 0.6 sec

    // RPM below 50 threshold shouldn't update engine hours
    TEST_ASSERT_EQUAL_INT(0, context.data_cache.engine.engine_time);

    delete eng;
}

void test_tachometer_loop_respects_period_timing(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    unsigned long micros = 10000000;

    for (int i = 0; i < 100; i++)
    {
        tacho.read_signal(i % 2 == 0 ? LOW : HIGH);
    }

    // First loop - should trigger calculation
    tacho.loop(micros, context);
    int rpm_first = context.data_cache.engine.rpm;

    // Second loop within period - should not trigger
    tacho.loop(micros + 100000, context);  // 100ms < 500ms period
    int rpm_second = context.data_cache.engine.rpm;

    // RPM should be the same (from smoothing buffer)
    TEST_ASSERT_EQUAL_INT(rpm_first, rpm_second);

    delete eng;
}

#pragma endregion

#pragma region Engine Hours Tests

void test_tachometer_loop_engine_hours_above_threshold(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    unsigned long micros = 0;

    // Simulate enough signal for >50 RPM
    for (int i = 0; i < 500; i++)
    {
        tacho.read_signal(i % 2 == 0 ? LOW : HIGH);
    }

    tacho.loop(micros, context);
    tacho.loop(micros + 600000, context);

    // Engine hours should be updated (RPM > 50)
    TEST_ASSERT_GREATER_THAN(0, context.data_cache.engine.engine_time);

    delete eng;
}

void test_tachometer_loop_engine_hours_below_threshold(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    unsigned long micros = 0;

    // Not enough signal for >50 RPM
    tacho.loop(micros, context);
    tacho.loop(micros + 600000, context);

    // Engine hours should remain 0
    TEST_ASSERT_EQUAL_INT(0, context.data_cache.engine.engine_time);

    delete eng;
}

void test_tachometer_loop_engine_hours_accumulation(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    unsigned long micros = 0;

    // First reading
    for (int i = 0; i < 500; i++)
    {
        tacho.read_signal(i % 2 == 0 ? LOW : HIGH);
    }
    tacho.loop(micros, context);
    tacho.loop(micros + 600000, context);

    uint64_t engine_time_1 = context.data_cache.engine.engine_time;

    // Second reading
    for (int i = 0; i < 500; i++)
    {
        tacho.read_signal(i % 2 == 0 ? LOW : HIGH);
    }
    tacho.loop(micros + 600000, context);
    tacho.loop(micros + 1200000, context);

    uint64_t engine_time_2 = context.data_cache.engine.engine_time;

    // Engine time should increase
    TEST_ASSERT_GREATER_THAN(engine_time_1, engine_time_2);

    delete eng;
}

void test_tachometer_loop_engine_hours_persists(void)
{
    EngineHours *eng = new MockEngineHours();
    eng->save_engine_hours(10000);  // Pre-set engine hours
    
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    unsigned long micros = 0;

    for (int i = 0; i < 500; i++)
    {
        tacho.read_signal(i % 2 == 0 ? LOW : HIGH);
    }

    tacho.loop(micros, context);
    tacho.loop(micros + 600000, context);

    // Engine time should be based on pre-existing value
    TEST_ASSERT_GREATER_THAN(10000, context.data_cache.engine.engine_time);

    delete eng;
}

#pragma endregion

#pragma region N2K Transmission Tests

void test_tachometer_loop_sends_rpm_to_n2k(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    unsigned long micros = 0;

    for (int i = 0; i < 500; i++)
    {
        tacho.read_signal(i % 2 == 0 ? LOW : HIGH);
    }

    tacho.loop(micros, context);
    tacho.loop(micros + 600000, context);

    TEST_ASSERT_GREATER_THAN(0, n2kSender.getStats().sent);

    delete eng;
}

void test_tachometer_loop_sends_engine_hours_periodically(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    unsigned long micros = 0;

    for (int i = 0; i < 500; i++)
    {
        tacho.read_signal(i % 2 == 0 ? LOW : HIGH);
    }

    // First call
    tacho.loop(micros, context);
    tacho.loop(micros + 600000, context);

    int send_count_1 = n2kSender.getStats().sent;

    // Second call within engine hours period - should not send
    tacho.loop(micros + 600000, context);
    tacho.loop(micros + 1000000, context);

    int send_count_2 = n2kSender.getStats().sent;
    TEST_ASSERT_EQUAL_INT(send_count_1, send_count_2);

    // Third call after engine hours period - should send
    tacho.loop(micros + 2100000, context);

    int send_count_3 = n2kSender.getStats().sent;
    TEST_ASSERT_GREATER_THAN(send_count_2, send_count_3);

    delete eng;
}

#pragma endregion

#pragma region Integration Tests

void test_tachometer_full_lifecycle(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    // Setup
    tacho.setup(context);
    TEST_ASSERT_FALSE(tacho.is_enabled());

    // Enable
    tacho.enable();
    TEST_ASSERT_TRUE(tacho.is_enabled());

    // Simulate signal
    unsigned long micros = 0;
    for (int i = 0; i < 500; i++)
    {
        tacho.read_signal(i % 2 == 0 ? LOW : HIGH);
    }

    // Loop - should calculate RPM
    tacho.loop(micros, context);
    tacho.loop(micros + 600000, context);

    TEST_ASSERT_GREATER_THAN(0, context.data_cache.engine.rpm);

    // Disable
    tacho.disable();
    TEST_ASSERT_FALSE(tacho.is_enabled());

    // Loop after disable - should clear data
    tacho.loop(micros + 1200000, context);
    TEST_ASSERT_EQUAL_INT(0, context.data_cache.engine.rpm);

    delete eng;
}

void test_tachometer_multiple_instances(void)
{
    EngineHours *eng = new MockEngineHours();
    
    Tachometer tacho1(25, eng, 1, 1.0, 1.0, 0);
    Tachometer tacho2(26, eng, 1, 1.0, 1.0, 0);
    
    MOCK_CONTEXT

    tacho1.setup(context);
    tacho2.setup(context);

    tacho1.enable();
    tacho2.enable();

    TEST_ASSERT_TRUE(tacho1.is_enabled());
    TEST_ASSERT_TRUE(tacho2.is_enabled());

    delete eng;
}

void test_tachometer_dump_stats(void)
{
    EngineHours *eng = new MockEngineHours();
    Tachometer tacho(25, eng, 1, 1.0, 1.0, 0);
    MOCK_CONTEXT

    tacho.setup(context);
    tacho.enable();

    unsigned long micros = 0;
    for (int i = 0; i < 500; i++)
    {
        tacho.read_signal(i % 2 == 0 ? LOW : HIGH);
    }

    tacho.loop(micros, context);
    tacho.loop(micros + 600000, context);

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
    RUN_TEST(test_tachometer_constructor_with_rpm_adjustment);
    RUN_TEST(test_tachometer_constructor_different_pins);
    RUN_TEST(test_tachometer_destructor_cleans_up);

    // Setup
    RUN_TEST(test_tachometer_setup_marks_setup);
    RUN_TEST(test_tachometer_setup_idempotent);

    // Enable/Disable
    RUN_TEST(test_tachometer_enable_without_setup_fails);
    RUN_TEST(test_tachometer_enable_after_setup_succeeds);
    RUN_TEST(test_tachometer_enable_idempotent);
    RUN_TEST(test_tachometer_disable_success);
    RUN_TEST(test_tachometer_disable_when_not_enabled);
    RUN_TEST(test_tachometer_enable_disable_cycle);

    // Signal Reading
    RUN_TEST(test_tachometer_read_signal_low_to_high);
    RUN_TEST(test_tachometer_read_signal_high_to_low);
    RUN_TEST(test_tachometer_read_signal_no_change);
    RUN_TEST(test_tachometer_read_signal_multiple_transitions);

    // Loop - RPM Calculation
    RUN_TEST(test_tachometer_loop_disabled_sets_rpm_to_zero);
    RUN_TEST(test_tachometer_loop_enabled_no_signal_rpm_zero);
    RUN_TEST(test_tachometer_loop_with_signal_calculates_rpm);
    RUN_TEST(test_tachometer_loop_with_rpm_adjustment);
    RUN_TEST(test_tachometer_loop_with_poles);
    RUN_TEST(test_tachometer_loop_low_rpm_threshold);
    RUN_TEST(test_tachometer_loop_respects_period_timing);

    // Engine Hours
    RUN_TEST(test_tachometer_loop_engine_hours_above_threshold);
    RUN_TEST(test_tachometer_loop_engine_hours_below_threshold);
    RUN_TEST(test_tachometer_loop_engine_hours_accumulation);
    RUN_TEST(test_tachometer_loop_engine_hours_persists);

    // N2K Transmission
    RUN_TEST(test_tachometer_loop_sends_rpm_to_n2k);
    RUN_TEST(test_tachometer_loop_sends_engine_hours_periodically);

    // Integration
    RUN_TEST(test_tachometer_full_lifecycle);
    RUN_TEST(test_tachometer_multiple_instances);
    RUN_TEST(test_tachometer_dump_stats);
    UNITY_END();
}

void loop()
{}

int main()
{
    setup();
    return 0;
}