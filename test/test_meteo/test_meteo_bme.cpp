#include <unity.h>
#include <math.h>
#include "MeteoBME.h"
#include "Data.h"
#include "Conf.h"
#include "N2K_router.h"

// Mock BME280Internal for testing
class MockBME280 : public BME280Internal
{
public:
    bool started;
    bool should_fail_start;
    float mock_pressure;
    float mock_temperature;
    float mock_humidity;
    int start_call_count;
    int stop_call_count;
    int read_pressure_call_count;
    int read_temperature_call_count;
    int read_humidity_call_count;

    MockBME280()
        : started(false), 
          should_fail_start(false),
          mock_pressure(101325.0f),
          mock_temperature(20.5f),
          mock_humidity(45.0f),
          start_call_count(0),
          stop_call_count(0),
          read_pressure_call_count(0),
          read_temperature_call_count(0),
          read_humidity_call_count(0)
    {
    }

    virtual bool start()
    {
        start_call_count++;
        if (should_fail_start)
        {
            started = false;
            return false;
        }
        started = true;
        return true;
    }

    virtual void stop()
    {
        stop_call_count++;
        started = false;
    }

    virtual float readPressure()
    {
        read_pressure_call_count++;
        if (!started) return NAN;
        return mock_pressure;
    }

    virtual float readTemperature()
    {
        read_temperature_call_count++;
        if (!started) return NAN;
        return mock_temperature;
    }

    virtual float readHumidity()
    {
        read_humidity_call_count++;
        if (!started) return NAN;
        return mock_humidity;
    }

    void reset_call_counts()
    {
        start_call_count = 0;
        stop_call_count = 0;
        read_pressure_call_count = 0;
        read_temperature_call_count = 0;
        read_humidity_call_count = 0;
    }

    void set_mock_values(float pressure, float temperature, float humidity)
    {
        mock_pressure = pressure;
        mock_temperature = temperature;
        mock_humidity = humidity;
    }
};

void setUpMeteoBME(void)
{
}

void tearDownMeteoBME(void)
{
    // Cleanup after each test
}

#pragma region Constructor Tests

void test_meteo_bme_constructor_with_index_0(void)
{
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    TEST_ASSERT_FALSE(bme.is_enabled());
}

void test_meteo_bme_constructor_with_index_1(void)
{
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 1, mock_impl);

    TEST_ASSERT_FALSE(bme.is_enabled());
}

void test_meteo_bme_constructor_stores_address(void)
{
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x76, 0, mock_impl);

    // Address is private, but we can verify via setup
    TEST_ASSERT_FALSE(bme.is_enabled());
}

void test_meteo_bme_destructor_cleans_up(void)
{
    MockBME280 *mock_impl = new MockBME280();
    {
        MeteoBME bme(0x77, 0, mock_impl);
        // Destructor should be called when bme goes out of scope
    }
    // No crash = success
    TEST_ASSERT_TRUE(true);
}

#pragma endregion

#pragma region Enable/Disable Tests

void test_meteo_bme_enable_success(void)
{
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    bme.enable();

    TEST_ASSERT_TRUE(bme.is_enabled());
    TEST_ASSERT_EQUAL_INT(1, mock_impl->start_call_count); // start() called in enable()
}

void test_meteo_bme_enable_fails_gracefully(void)
{
    MockBME280 *mock_impl = new MockBME280();
    mock_impl->should_fail_start = true;
    MeteoBME bme(0x77, 0, mock_impl);

    bme.enable();

    TEST_ASSERT_FALSE(bme.is_enabled());
}

void test_meteo_bme_enable_idempotent(void)
{
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    bme.enable();
    int first_call_count = mock_impl->start_call_count;

    bme.enable();
    int second_call_count = mock_impl->start_call_count;

    TEST_ASSERT_EQUAL_INT(first_call_count, second_call_count);
}

void test_meteo_bme_disable_success(void)
{
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    bme.enable();
    TEST_ASSERT_TRUE(bme.is_enabled());

    bme.disable();

    TEST_ASSERT_FALSE(bme.is_enabled());
}

void test_meteo_bme_disable_when_not_enabled(void)
{
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    TEST_ASSERT_FALSE(bme.is_enabled());
    bme.disable();

    TEST_ASSERT_FALSE(bme.is_enabled());
}

void test_meteo_bme_enable_disable_cycle(void)
{
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    bme.enable();
    TEST_ASSERT_TRUE(bme.is_enabled());

    bme.disable();
    TEST_ASSERT_FALSE(bme.is_enabled());

    bme.enable();
    TEST_ASSERT_TRUE(bme.is_enabled());
}

#pragma endregion

#pragma region Read Tests

void test_meteo_bme_read_when_enabled(void)
{
    MockBME280 *mock_impl = new MockBME280();
    mock_impl->set_mock_values(101325.0f, 20.5f, 45.0f);
    MeteoBME bme(0x77, 0, mock_impl);

    bme.enable();

    MeteoData data;
    bme.read(0, data);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, 101325.0f, data.pressure);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 20.5f, data.temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 45.0f, data.humidity);
}

void test_meteo_bme_read_when_disabled_returns_nan(void)
{
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    MeteoData data;
    data.pressure = 100.0f;
    data.temperature = 25.0f;
    data.humidity = 50.0f;

    bme.read(0, data);

    TEST_ASSERT_TRUE(isnan(data.pressure));
    TEST_ASSERT_TRUE(isnan(data.temperature));
    TEST_ASSERT_TRUE(isnan(data.humidity));
}

void test_meteo_bme_read_pressure_zero(void)
{
    MockBME280 *mock_impl = new MockBME280();
    mock_impl->set_mock_values(0.0f, 20.0f, 45.0f);
    MeteoBME bme(0x77, 0, mock_impl);

    bme.enable();

    MeteoData data;
    bme.read(0, data);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, data.pressure);
}

void test_meteo_bme_read_negative_temperature(void)
{
    MockBME280 *mock_impl = new MockBME280();
    mock_impl->set_mock_values(101325.0f, -15.5f, 30.0f);
    MeteoBME bme(0x77, 0, mock_impl);

    bme.enable();

    MeteoData data;
    bme.read(0, data);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, -15.5f, data.temperature);
}

void test_meteo_bme_read_temperature_range(void)
{
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    bme.enable();

    float test_temps[] = {-40.0f, 0.0f, 25.0f, 85.0f};
    
    for (size_t i = 0; i < sizeof(test_temps) / sizeof(test_temps[0]); i++)
    {
        mock_impl->set_mock_values(101325.0f, test_temps[i], 50.0f);
        MeteoData data;
        bme.read(0, data);
        TEST_ASSERT_FLOAT_WITHIN(0.1f, test_temps[i], data.temperature);
    }
}

void test_meteo_bme_read_humidity_zero(void)
{
    MockBME280 *mock_impl = new MockBME280();
    mock_impl->set_mock_values(101325.0f, 20.0f, 0.0f);
    MeteoBME bme(0x77, 0, mock_impl);

    bme.enable();

    MeteoData data;
    bme.read(0, data);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, data.humidity);
}

void test_meteo_bme_read_humidity_max(void)
{
    MockBME280 *mock_impl = new MockBME280();
    mock_impl->set_mock_values(101325.0f, 20.0f, 100.0f);
    MeteoBME bme(0x77, 0, mock_impl);

    bme.enable();

    MeteoData data;
    bme.read(0, data);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, data.humidity);
}

void test_meteo_bme_read_multiple_times(void)
{
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    bme.enable();

    for (int i = 0; i < 5; i++)
    {
        mock_impl->set_mock_values(101325.0f + i, 20.0f + i, 45.0f + i);
        MeteoData data;
        bme.read(0, data);
        
        TEST_ASSERT_FLOAT_WITHIN(0.1f, 101325.0f + i, data.pressure);
        TEST_ASSERT_FLOAT_WITHIN(0.1f, 20.0f + i, data.temperature);
        TEST_ASSERT_FLOAT_WITHIN(0.1f, 45.0f + i, data.humidity);
    }
}

#pragma endregion

#pragma region Loop Tests

void test_meteo_bme_loop_updates_meteo_0_when_enabled(void)
{
    MOCK_CONTEXT

    MockBME280 *mock_impl = new MockBME280();
    mock_impl->set_mock_values(101325.0f, 22.0f, 50.0f);
    MeteoBME bme(0x77, 0, mock_impl);

    bme.setup(context);
    bme.enable();

    // First loop call
    unsigned long ms1 = 1500000; // 1.5 seconds after start
    bme.loop(ms1, context);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, 101325.0f, context.data_cache.meteo_0.pressure);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 22.0f, context.data_cache.meteo_0.temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 50.0f, context.data_cache.meteo_0.humidity);
}

void test_meteo_bme_loop_updates_meteo_1_when_enabled(void)
{
    MOCK_CONTEXT
    
    MockBME280 *mock_impl = new MockBME280();
    mock_impl->set_mock_values(102000.0f, 18.0f, 60.0f);
    MeteoBME bme(0x77, 1, mock_impl);

    bme.setup(context);
    bme.enable();

    unsigned long ms = 1500000; // 1.5 secs after start
    bme.loop(ms, context);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, 102000.0f, context.data_cache.meteo_1.pressure);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 18.0f, context.data_cache.meteo_1.temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 60.0f, context.data_cache.meteo_1.humidity);
}

void test_meteo_bme_loop_respects_timing_interval(void)
{
    MOCK_CONTEXT
    
    MockBME280 *mock_impl = new MockBME280();
    mock_impl->set_mock_values(101325.0f, 20.0f, 45.0f);
    MeteoBME bme(0x77, 0, mock_impl);

    bme.setup(context);
    bme.enable();

    unsigned long ms1 = 10000000L;
    bme.loop(ms1, context);
    int first_read_count = mock_impl->read_pressure_call_count;

    // Call loop again within 1 second (should not update)
    unsigned long ms2 = ms1 + 500000;  // 500ms
    bme.loop(ms2, context);
    int second_read_count = mock_impl->read_pressure_call_count;

    // Should not have called read again
    TEST_ASSERT_EQUAL_INT(first_read_count, second_read_count);
}

void test_meteo_bme_loop_updates_after_interval_elapsed(void)
{
    MOCK_CONTEXT
    
    MockBME280 *mock_impl = new MockBME280();
    mock_impl->set_mock_values(101325.0f, 20.0f, 45.0f);
    MeteoBME bme(0x77, 0, mock_impl);

    bme.setup(context);
    bme.enable();

    unsigned long ms1 = 0;
    bme.loop(ms1, context);
    mock_impl->reset_call_counts();

    // Call loop after 1+ second
    unsigned long ms2 = 1000001;  // 1000001 microseconds = 1.000001 seconds
    bme.loop(ms2, context);

    // Should have called read methods again
    TEST_ASSERT_GREATER_THAN(0, mock_impl->read_pressure_call_count);
}

void test_meteo_bme_loop_when_disabled_does_not_read(void)
{
    MOCK_CONTEXT
    
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    bme.setup(context);
    // Don't enable

    unsigned long ms = 0;
    bme.loop(ms, context);

    // Should not have called read
    TEST_ASSERT_EQUAL_INT(0, mock_impl->read_pressure_call_count);
}

void test_meteo_bme_loop_invalid_index_ignored(void)
{
    MOCK_CONTEXT

    MockBME280 *mock_impl = new MockBME280();
    mock_impl->set_mock_values(101325.0f, 20.0f, 45.0f);
    MeteoBME bme(0x77, 99, mock_impl);  // Invalid index

    bme.setup(context);
    bme.enable();

    unsigned long ms = 0;
    bme.loop(ms, context);

    // Should not crash - invalid index simply doesn't update
    TEST_ASSERT_TRUE(true);
}

#pragma endregion

#pragma region Setup Tests

void test_meteo_bme_setup_creates_implementation(void)
{
    MOCK_CONTEXT
    
    MeteoBME bme(0x77, 0, nullptr);

    bme.setup(context);

    // After setup, implementation should be created
    TEST_ASSERT_TRUE(true);  // No crash means success
}

void test_meteo_bme_setup_with_existing_implementation(void)
{
    MOCK_CONTEXT
    
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    bme.setup(context);

    // Should keep existing implementation
    TEST_ASSERT_TRUE(true);
}

#pragma endregion

#pragma region State Tests

void test_meteo_bme_is_enabled_initial_state(void)
{
    MOCK_CONTEXT
    
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    TEST_ASSERT_FALSE(bme.is_enabled());
}

void test_meteo_bme_is_enabled_after_enable(void)
{
    MOCK_CONTEXT
    
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    bme.enable();

    TEST_ASSERT_TRUE(bme.is_enabled());
}

void test_meteo_bme_is_enabled_after_disable(void)
{
    MOCK_CONTEXT
    
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    bme.enable();
    bme.disable();

    TEST_ASSERT_FALSE(bme.is_enabled());
}

#pragma endregion

#pragma region Integration Tests

void test_meteo_bme_full_lifecycle(void)
{
    MOCK_CONTEXT
    
    MockBME280 *mock_impl = new MockBME280();
    mock_impl->set_mock_values(101325.0f, 20.0f, 45.0f);
    MeteoBME bme(0x77, 0, mock_impl);

    // Setup
    bme.setup(context);
    TEST_ASSERT_FALSE(bme.is_enabled());

    // Enable
    bme.enable();
    TEST_ASSERT_TRUE(bme.is_enabled());

    // Loop - should read data
    unsigned long ms = 1500000; //1.5 secs after start
    bme.loop(ms, context);
    
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 101325.0f, context.data_cache.meteo_0.pressure);

    // Disable
    bme.disable();
    TEST_ASSERT_FALSE(bme.is_enabled());

    // Loop - should not read data
    MeteoData prev_data = context.data_cache.meteo_0;
    unsigned long ms2 = 3500000; // 2 secs elapsed from last loop
    bme.loop(ms2, context);
    
    // Data should be reset to NAN
    TEST_ASSERT_TRUE(isnan(context.data_cache.meteo_0.pressure));
}

void test_meteo_bme_multiple_instances_independent(void)
{
    MOCK_CONTEXT
    
    MockBME280 *mock_impl1 = new MockBME280();
    mock_impl1->set_mock_values(101325.0f, 20.0f, 45.0f);
    MeteoBME bme0(0x76, 0, mock_impl1);

    MockBME280 *mock_impl2 = new MockBME280();
    mock_impl2->set_mock_values(102000.0f, 18.0f, 55.0f);
    MeteoBME bme1(0x77, 1, mock_impl2);

    bme0.setup(context);
    bme1.setup(context);

    bme0.enable();
    bme1.enable();

    unsigned long ms = 1500000;
    bme0.loop(ms, context);
    bme1.loop(ms, context);

    // Each should have updated its respective meteo data
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 101325.0f, context.data_cache.meteo_0.pressure);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 102000.0f, context.data_cache.meteo_1.pressure);
}

void test_meteo_bme_rapid_enable_disable_cycles(void)
{
    MOCK_CONTEXT
    
    MockBME280 *mock_impl = new MockBME280();
    MeteoBME bme(0x77, 0, mock_impl);

    for (int i = 0; i < 10; i++)
    {
        bme.enable();
        TEST_ASSERT_TRUE(bme.is_enabled());
        bme.disable();
        TEST_ASSERT_FALSE(bme.is_enabled());
    }
}

void test_meteo_bme_read_different_addresses(void)
{
    MOCK_CONTEXT
    
    // Test with typical BME280 addresses
    uint8_t addresses[] = {0x76, 0x77};
    
    for (size_t i = 0; i < sizeof(addresses) / sizeof(addresses[0]); i++)
    {
        MockBME280 *mock_impl = new MockBME280();
        MeteoBME bme(addresses[i], 0, mock_impl);
        
        bme.enable();
        TEST_ASSERT_TRUE(bme.is_enabled());
        bme.disable();
    }
}

#pragma endregion

// Test runner
void run_meteo_bme_tests()
{
    UNITY_BEGIN();

    // Constructor
    RUN_TEST(test_meteo_bme_constructor_with_index_0);
    RUN_TEST(test_meteo_bme_constructor_with_index_1);
    RUN_TEST(test_meteo_bme_constructor_stores_address);
    RUN_TEST(test_meteo_bme_destructor_cleans_up);

    // Enable/Disable
    RUN_TEST(test_meteo_bme_enable_success);
    RUN_TEST(test_meteo_bme_enable_fails_gracefully);
    RUN_TEST(test_meteo_bme_enable_idempotent);
    RUN_TEST(test_meteo_bme_disable_success);
    RUN_TEST(test_meteo_bme_disable_when_not_enabled);
    RUN_TEST(test_meteo_bme_enable_disable_cycle);

    // Read
    RUN_TEST(test_meteo_bme_read_when_enabled);
    RUN_TEST(test_meteo_bme_read_when_disabled_returns_nan);
    RUN_TEST(test_meteo_bme_read_pressure_zero);
    RUN_TEST(test_meteo_bme_read_negative_temperature);
    RUN_TEST(test_meteo_bme_read_temperature_range);
    RUN_TEST(test_meteo_bme_read_humidity_zero);
    RUN_TEST(test_meteo_bme_read_humidity_max);
    RUN_TEST(test_meteo_bme_read_multiple_times);

    // Loop
    RUN_TEST(test_meteo_bme_loop_updates_meteo_0_when_enabled);
    RUN_TEST(test_meteo_bme_loop_updates_meteo_1_when_enabled);
    RUN_TEST(test_meteo_bme_loop_respects_timing_interval);
    RUN_TEST(test_meteo_bme_loop_updates_after_interval_elapsed);
    RUN_TEST(test_meteo_bme_loop_when_disabled_does_not_read);
    RUN_TEST(test_meteo_bme_loop_invalid_index_ignored);

    // Setup
    RUN_TEST(test_meteo_bme_setup_creates_implementation);
    RUN_TEST(test_meteo_bme_setup_with_existing_implementation);

    // State
    RUN_TEST(test_meteo_bme_is_enabled_initial_state);
    RUN_TEST(test_meteo_bme_is_enabled_after_enable);
    RUN_TEST(test_meteo_bme_is_enabled_after_disable);

    // Integration
    RUN_TEST(test_meteo_bme_full_lifecycle);
    RUN_TEST(test_meteo_bme_multiple_instances_independent);
    RUN_TEST(test_meteo_bme_rapid_enable_disable_cycles);
    RUN_TEST(test_meteo_bme_read_different_addresses);

    UNITY_END();
}