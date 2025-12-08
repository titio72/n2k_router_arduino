#include <unity.h>
#include <string.h>
#include <math.h>
#include "MeteoDHT.h"
#include "Conf.h"
#include "Context.h"
#include "Data.h"
#include "N2K_router.h"

// ==================== Mock DHTInternal ====================
class TestDHTInternal: public DHTInternal {
public:
    bool setup_called = false;
    bool setup_success = true;
    double temperature = 22.5;
    double humidity = 65.0;
    unsigned long minimum_sampling_period = 250;  // 0.25 seconds in ms
    int get_temp_humidity_call_count = 0;
    int get_minimum_sampling_period_call_count = 0;
    
    bool setup() {
        setup_called = true;
        return setup_success;
    }
    
    void getTempAndHumidity(double &temp, double &hum) {
        get_temp_humidity_call_count++;
        temp = temperature;
        hum = humidity;
    }
    
    unsigned long getMinimumSamplingPeriod() {
        get_minimum_sampling_period_call_count++;
        return minimum_sampling_period;
    }
    
    void reset() {
        setup_called = false;
        setup_success = true;
        temperature = 22.5;
        humidity = 65.0;
        minimum_sampling_period = 2000;
        get_temp_humidity_call_count = 0;
        get_minimum_sampling_period_call_count = 0;
    }
};

// ==================== Test Fixtures ====================
void setUpMeteoDHT() {
}

void tearDownMeteoDHT() {
}

// ==================== Tests: Constructor & Initialization ====================
void test_constructor_with_dht22() {
    TestDHTInternal test_impl;
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    
    TEST_ASSERT_FALSE(dht.is_enabled());
}

void test_constructor_with_dht11() {
    TestDHTInternal test_impl;
    MeteoDHT dht(5, MeteoDHT::DHT11, 0, &test_impl);
    
    TEST_ASSERT_FALSE(dht.is_enabled());
}

void test_constructor_with_different_pins() {
    TestDHTInternal test_impl;
    MeteoDHT dht_pin4(4, MeteoDHT::DHT22, 0, &test_impl);
    MeteoDHT dht_pin7(7, MeteoDHT::DHT22, 1, &test_impl);
    
    TEST_ASSERT_FALSE(dht_pin4.is_enabled());
    TEST_ASSERT_FALSE(dht_pin7.is_enabled());
}

void test_constructor_with_different_meteo_indices() {
    TestDHTInternal test_impl;
    MeteoDHT dht0(5, MeteoDHT::DHT22, 0, &test_impl);
    MeteoDHT dht1(5, MeteoDHT::DHT22, 1, &test_impl);
    
    TEST_ASSERT_FALSE(dht0.is_enabled());
    TEST_ASSERT_FALSE(dht1.is_enabled());
}

// ==================== Tests: Enable/Disable ====================
void test_enable_calls_dht_setup() {
    TestDHTInternal test_impl;
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    
    dht.enable();
    
    TEST_ASSERT_TRUE(test_impl.setup_called);
    TEST_ASSERT_TRUE(dht.is_enabled());
}

void test_enable_when_setup_fails() {
    TestDHTInternal test_impl;
    test_impl.setup_success = false;
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    
    dht.enable();
    
    TEST_ASSERT_FALSE(dht.is_enabled());
}

void test_enable_when_already_enabled() {
    TestDHTInternal test_impl;
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    
    dht.enable();
    test_impl.reset();  // Reset setup_called flag
    dht.enable();      // Try to enable again
    
    // Setup should not be called again
    TEST_ASSERT_FALSE(test_impl.setup_called);
    TEST_ASSERT_TRUE(dht.is_enabled());
}

void test_disable_clears_enabled_flag() {
    TestDHTInternal test_impl;
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    
    dht.enable();
    TEST_ASSERT_TRUE(dht.is_enabled());
    
    dht.disable();
    TEST_ASSERT_FALSE(dht.is_enabled());
}

void test_disable_when_already_disabled() {
    TestDHTInternal test_impl;
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    
    dht.disable();
    TEST_ASSERT_FALSE(dht.is_enabled());
}

void test_enable_disable_toggle() {
    TestDHTInternal test_impl;
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    
    dht.enable();
    TEST_ASSERT_TRUE(dht.is_enabled());
    
    dht.disable();
    TEST_ASSERT_FALSE(dht.is_enabled());
    
    dht.enable();
    TEST_ASSERT_TRUE(dht.is_enabled());
}

// ==================== Tests: Setup ====================
void test_setup_does_not_enable_dht() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    
    dht.setup(context);
    
    TEST_ASSERT_FALSE(dht.is_enabled());
}

void test_setup_initializes_dht_object() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    
    // Should not crash
    dht.setup(context);
}

// ==================== Tests: Read Temperature and Humidity ====================
void test_read_when_enabled_updates_data() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    test_impl.temperature = 25.5;
    test_impl.humidity = 70.0;
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    dht.enable();
    
    // First loop call - should update on first call
    dht.loop(10000000, context);
    
    TEST_ASSERT_EQUAL_DOUBLE(25.5, context.data_cache.meteo_0.temperature);
    TEST_ASSERT_EQUAL_DOUBLE(70.0, context.data_cache.meteo_0.humidity);
}

void test_read_when_disabled_sets_nan() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    // Don't enable
    
    dht.loop(0, context);
    
    TEST_ASSERT_TRUE(isnan(context.data_cache.meteo_0.temperature));
    TEST_ASSERT_TRUE(isnan(context.data_cache.meteo_0.humidity));
}

void test_read_respects_sampling_period() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    test_impl.minimum_sampling_period = 2000;  // 2 seconds in ms
    test_impl.temperature = 20.0;
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    dht.enable();
    
    // First read at time 0
    dht.loop(10000000, context);
    double first_temp = context.data_cache.meteo_0.temperature;
    
    // Read within sampling period (should not update)
    test_impl.temperature = 25.0;
    dht.loop(11000000, context);  // 1 second later
    
    // Temperature should still be 20.0 (not updated)
    TEST_ASSERT_EQUAL_DOUBLE(first_temp, context.data_cache.meteo_0.temperature);
}

void test_read_after_sampling_period_updates() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    test_impl.minimum_sampling_period = 2000;  // 2 seconds in ms
    test_impl.temperature = 20.0;
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    dht.enable();
    
    // First read at time 0
    dht.loop(10000000, context);
    
    // Read after sampling period (should update)
    test_impl.temperature = 25.0;
    dht.loop(12000001, context);  // 2 seconds + 1 microsecond later
    
    TEST_ASSERT_EQUAL_DOUBLE(25.0, context.data_cache.meteo_0.temperature);
}

void test_read_temperature_multiple_values() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    test_impl.minimum_sampling_period = 100;  // 100 ms for faster test
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    dht.enable();
    
    unsigned long current_time = 10000000;

    // First read
    test_impl.temperature = 15.0;
    dht.loop(current_time, context);
    TEST_ASSERT_EQUAL_DOUBLE(15.0, context.data_cache.meteo_0.temperature);
    
    // Second read after period
    test_impl.temperature = 20.0;
    dht.loop(current_time + 100001, context);
    TEST_ASSERT_EQUAL_DOUBLE(20.0, context.data_cache.meteo_0.temperature);
    
    // Third read after period
    test_impl.temperature = 25.0;
    dht.loop(current_time + 200002, context);
    TEST_ASSERT_EQUAL_DOUBLE(25.0, context.data_cache.meteo_0.temperature);
}

void test_read_humidity_multiple_values() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    test_impl.minimum_sampling_period = 100;
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    dht.enable();
    
    // First read
    test_impl.humidity = 40.0;
    dht.loop(10000000, context);
    TEST_ASSERT_EQUAL_DOUBLE(40.0, context.data_cache.meteo_0.humidity);
    
    // Second read after period
    test_impl.humidity = 60.0;
    dht.loop(100001, context);
    TEST_ASSERT_EQUAL_DOUBLE(60.0, context.data_cache.meteo_0.humidity);
}

// ==================== Tests: Meteo Index Selection ====================
void test_read_to_meteo_index_0() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    test_impl.temperature = 22.5;
    test_impl.humidity = 65.0;
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    dht.enable();
    dht.loop(10000000, context);
    
    // Should write to meteo_0
    TEST_ASSERT_EQUAL_DOUBLE(22.5, context.data_cache.meteo_0.temperature);
    TEST_ASSERT_EQUAL_DOUBLE(65.0, context.data_cache.meteo_0.humidity);
}

void test_read_to_meteo_index_1() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    test_impl.temperature = 25.0;
    test_impl.humidity = 70.0;
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 1, &test_impl);
    dht.enable();
    dht.loop(10000000, context);
    
    // Should write to meteo_1, meteo_0 should be NAN
    TEST_ASSERT_TRUE(isnan(context.data_cache.meteo_0.temperature));
    TEST_ASSERT_EQUAL_DOUBLE(25.0, context.data_cache.meteo_1.temperature);
    TEST_ASSERT_EQUAL_DOUBLE(70.0, context.data_cache.meteo_1.humidity);
}

void test_read_independent_meteo_indices() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    
    MeteoDHT dht0(5, MeteoDHT::DHT22, 0, &test_impl);
    MeteoDHT dht1(6, MeteoDHT::DHT22, 1, &test_impl);
    
    test_impl.temperature = 20.0;
    test_impl.humidity = 50.0;
    dht0.enable();
    dht0.loop(10000000, context);
    
    test_impl.temperature = 30.0;
    test_impl.humidity = 80.0;
    dht1.enable();
    dht1.loop(10500000, context);
    
    TEST_ASSERT_EQUAL_DOUBLE(20.0, context.data_cache.meteo_0.temperature);
    TEST_ASSERT_EQUAL_DOUBLE(30.0, context.data_cache.meteo_1.temperature);
}

// ==================== Tests: Edge Cases ====================
void test_extreme_temperature_values() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    dht.enable();
    
    // Very low temperature
    test_impl.temperature = -40.0;
    dht.loop(10000000, context);
    TEST_ASSERT_EQUAL_DOUBLE(-40.0, context.data_cache.meteo_0.temperature);
    
    // Very high temperature
    test_impl.temperature = 85.0;
    dht.loop(11000000, context);
    TEST_ASSERT_EQUAL_DOUBLE(85.0, context.data_cache.meteo_0.temperature);
}

void test_extreme_humidity_values() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    dht.enable();
    
    // 0% humidity
    test_impl.humidity = 0.0;
    dht.loop(10000000, context);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, context.data_cache.meteo_0.humidity);
    
    // 100% humidity
    test_impl.humidity = 100.0;
    dht.loop(11000000, context);
    TEST_ASSERT_EQUAL_DOUBLE(100.0, context.data_cache.meteo_0.humidity);
}

void test_nan_temperature_values() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    dht.enable();
    
    test_impl.temperature = NAN;
    dht.loop(0, context);
    
    TEST_ASSERT_TRUE(isnan(context.data_cache.meteo_0.temperature));
}

void test_nan_humidity_values() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    dht.enable();
    
    test_impl.humidity = NAN;
    dht.loop(0, context);
    
    TEST_ASSERT_TRUE(isnan(context.data_cache.meteo_0.humidity));
}

void test_very_fast_sampling_period() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    test_impl.minimum_sampling_period = 1;  // 1 ms
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    dht.enable();
    
    // Multiple reads very close together
    test_impl.temperature = 20.0;
    dht.loop(0, context);
    
    test_impl.temperature = 25.0;
    dht.loop(2000, context);  // 2ms later
    
    TEST_ASSERT_EQUAL_DOUBLE(25.0, context.data_cache.meteo_0.temperature);
}

void test_zero_microseconds() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    test_impl.temperature = 22.5;
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    dht.enable();
    
    dht.loop(0, context);
    TEST_ASSERT_TRUE(isnan(context.data_cache.meteo_0.temperature)); // First read at time 0 should not update
}

void test_large_microseconds() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    test_impl.temperature = 22.5;
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    dht.enable();
    
    dht.loop(0xFFFFFFFFUL, context);
    TEST_ASSERT_EQUAL_DOUBLE(22.5, context.data_cache.meteo_0.temperature);
}

// ==================== Tests: Integration ====================
void test_dht22_vs_dht11_models() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    
    MeteoDHT dht22(5, MeteoDHT::DHT22, 0, &test_impl);
    MeteoDHT dht11(6, MeteoDHT::DHT11, 1, &test_impl);
    
    test_impl.temperature = 24.0;
    test_impl.humidity = 60.0;
    
    dht22.enable();
    dht11.enable();
    
    dht22.loop(10000000, context);
    dht11.loop(10000000, context);
    
    // Both should have same values (mock doesn't differentiate)
    TEST_ASSERT_EQUAL_DOUBLE(24.0, context.data_cache.meteo_0.temperature);
    TEST_ASSERT_EQUAL_DOUBLE(24.0, context.data_cache.meteo_1.temperature);
}

void test_full_lifecycle() {
    MOCK_CONTEXT
    TestDHTInternal test_impl;
    test_impl.minimum_sampling_period = 100;
    
    MeteoDHT dht(5, MeteoDHT::DHT22, 0, &test_impl);
    
    // Initially disabled
    TEST_ASSERT_FALSE(dht.is_enabled());
    
    // Setup doesn't enable
    dht.setup(context);
    TEST_ASSERT_FALSE(dht.is_enabled());
    
    // Enable the sensor
    dht.enable();
    TEST_ASSERT_TRUE(dht.is_enabled());

    // Loop should populate data
    test_impl.temperature = 22.0;
    test_impl.humidity = 65.0;
    dht.loop(10000000, context);
    TEST_ASSERT_EQUAL_DOUBLE(22.0, context.data_cache.meteo_0.temperature);
    
    // Disable
    dht.disable();
    TEST_ASSERT_FALSE(dht.is_enabled());
    
    // Loop should clear data
    dht.loop(10500000, context); // .5 seconds later
    TEST_ASSERT_TRUE(isnan(context.data_cache.meteo_0.temperature));
}

void run_meteo_dht_tests() {
    UNITY_BEGIN();
    
    // Constructor & Initialization
    RUN_TEST(test_constructor_with_dht22);
    RUN_TEST(test_constructor_with_dht11);
    RUN_TEST(test_constructor_with_different_pins);
    RUN_TEST(test_constructor_with_different_meteo_indices);
    
    // Enable/Disable
    RUN_TEST(test_enable_calls_dht_setup);
    RUN_TEST(test_enable_when_setup_fails);
    RUN_TEST(test_enable_when_already_enabled);
    RUN_TEST(test_disable_clears_enabled_flag);
    RUN_TEST(test_disable_when_already_disabled);
    RUN_TEST(test_enable_disable_toggle);
    
    // Setup
    RUN_TEST(test_setup_does_not_enable_dht);
    RUN_TEST(test_setup_initializes_dht_object);
    
    // Read Temperature and Humidity
    RUN_TEST(test_read_when_enabled_updates_data);
    RUN_TEST(test_read_when_disabled_sets_nan);
    RUN_TEST(test_read_respects_sampling_period);
    RUN_TEST(test_read_after_sampling_period_updates);
    RUN_TEST(test_read_temperature_multiple_values);
    RUN_TEST(test_read_humidity_multiple_values);
    
    // Meteo Index Selection
    RUN_TEST(test_read_to_meteo_index_0);
    RUN_TEST(test_read_to_meteo_index_1);
    RUN_TEST(test_read_independent_meteo_indices);
    
    // Edge Cases
    RUN_TEST(test_extreme_temperature_values);
    RUN_TEST(test_extreme_humidity_values);
    RUN_TEST(test_nan_temperature_values);
    RUN_TEST(test_nan_humidity_values);
    RUN_TEST(test_very_fast_sampling_period);
    RUN_TEST(test_zero_microseconds);
    RUN_TEST(test_large_microseconds);
    
    // Integration
    RUN_TEST(test_dht22_vs_dht11_models);
    RUN_TEST(test_full_lifecycle);
    
    UNITY_END();
}