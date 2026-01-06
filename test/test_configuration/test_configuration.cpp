#include <unity.h>
#include <string.h>
#include <MockEEPROM.h>
#include "Conf.h"
#include <Log.h>

#ifndef TEST_CONFIGURATION
#define TEST_CONFIGURATION
// Helper function to reset configuration state
void reset_configuration()
{
    mockEEPROM.begin(512);
    mockEEPROM.write(0, 0);
    mockEEPROM.commit();
}

void setUpConfiguration(void)
{
    reset_configuration();
}

void tearDownConfiguration(void)
{
    reset_configuration();
}

#pragma region Initialization Tests

void test_configuration_constructor(void)
{
    Configuration conf;
    TEST_ASSERT_FALSE(conf.is_initialized());
}

void test_configuration_init_sets_initialized_flag(void)
{
    Configuration conf;
    TEST_ASSERT_FALSE(conf.is_initialized());
    
    TEST_ASSERT_EQUAL_INT(CONFIG_RES_VERSION_MISMATCH, conf.init());
    
    TEST_ASSERT_TRUE(conf.is_initialized());
}

void test_configuration_init_idempotent(void)
{
    Configuration conf;
    conf.init();
    TEST_ASSERT_TRUE(conf.is_initialized());
    
    // Second init should return early
    TEST_ASSERT_EQUAL_INT(CONFIG_RES_ALREADY_INITIALIZED, conf.init());
    TEST_ASSERT_TRUE(conf.is_initialized());
}

#pragma endregion

#pragma region N2KServices Tests

void test_configuration_get_services_returns_services_object(void)
{
    Configuration conf;
    conf.init();
    
    const N2KServices &svc = conf.get_services();
    TEST_ASSERT_TRUE(svc.size() > 0);
}

void test_configuration_save_services_updates_configuration(void)
{
    Configuration conf;
    conf.init();
    
    N2KServices new_svc;
    new_svc.set_use_gps(true);
    new_svc.set_use_bme(false);
    new_svc.set_use_dht(true);
    
    TEST_ASSERT_TRUE(conf.save_services(new_svc));
    
    const N2KServices &retrieved = conf.get_services();
    TEST_ASSERT_TRUE(retrieved.is_use_gps());
    TEST_ASSERT_FALSE(retrieved.is_use_bme());
    TEST_ASSERT_TRUE(retrieved.is_use_dht());
}

void test_configuration_services_persisted_after_save(void)
{
    Configuration conf1;
    conf1.init();
    
    N2KServices svc;
    svc.set_use_gps(true);
    svc.set_use_bme(true);
    svc.set_use_dht(false);
    
    TEST_ASSERT_TRUE(conf1.save_services(svc));
    
    // Create new configuration and verify persistence
    Configuration conf2;
    TEST_ASSERT_EQUAL_INT(CONFIG_RES_OK, conf2.init());
    
    const N2KServices &retrieved = conf2.get_services();
    TEST_ASSERT_TRUE(retrieved.is_use_gps());
    TEST_ASSERT_TRUE(retrieved.is_use_bme());
    TEST_ASSERT_FALSE(retrieved.is_use_dht());
}

#pragma endregion

#pragma region N2K Source Tests

void test_configuration_get_n2k_source_default(void)
{
    Configuration conf;
    conf.init();
    
    unsigned char source = conf.get_n2k_source();
    TEST_ASSERT_EQUAL_INT(DEFAULT_N2K_SOURCE, source);  // Default value
}

void test_configuration_save_n2k_source(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_n2k_source(42);
    
    unsigned char source = conf.get_n2k_source();
    TEST_ASSERT_EQUAL_INT(42, source);
}

void test_configuration_n2k_source_persisted(void)
{
    Configuration conf1;
    conf1.init();
    conf1.save_n2k_source(99);
        
    Configuration conf2;   
    conf2.init();

    unsigned char source = conf2.get_n2k_source();
    TEST_ASSERT_EQUAL_INT(99, source);
}

void test_configuration_n2k_source_boundary_values(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_n2k_source(0);
    TEST_ASSERT_EQUAL_INT(0, conf.get_n2k_source());
    
    conf.save_n2k_source(255);
    TEST_ASSERT_EQUAL_INT(255, conf.get_n2k_source());
}

#pragma endregion

#pragma region RPM Adjustment Tests

void test_configuration_get_rpm_adjustment_default(void)
{
    Configuration conf;
    conf.init();
    
    double rpm_adj = conf.get_rpm_adjustment();
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1.0, rpm_adj);
}

void test_configuration_save_rpm_adjustment_positive(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_rpm_adjustment(1.5);
    
    double rpm_adj = conf.get_rpm_adjustment();
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1.5, rpm_adj);
}

void test_configuration_save_rpm_adjustment_negative(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_rpm_adjustment(-2.25);
    
    double rpm_adj = conf.get_rpm_adjustment();
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, -2.25, rpm_adj);
}

void test_configuration_save_rpm_adjustment_zero(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_rpm_adjustment(0.0);
    
    double rpm_adj = conf.get_rpm_adjustment();
    TEST_ASSERT_EQUAL_DOUBLE(0.0, rpm_adj);
}

void test_configuration_rpm_adjustment_precision(void)
{
    Configuration conf;
    conf.init();
    
    double original = 3.14159;
    conf.save_rpm_adjustment(original);
    
    double retrieved = conf.get_rpm_adjustment();
    // Factor of 1000 means precision to 4 decimal places
    TEST_ASSERT_DOUBLE_WITHIN(0.005, original, retrieved);
}

void test_configuration_rpm_adjustment_large_values(void)
{
    Configuration conf;
    conf.init();
    
    TEST_ASSERT_TRUE(conf.save_rpm_adjustment(100.5));
    
    double rpm_adj = conf.get_rpm_adjustment();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 100.5, rpm_adj);
}

void test_configuration_rpm_adjustment_persisted(void)
{
    Configuration conf1;
    conf1.init();
    conf1.save_rpm_adjustment(2.75);
    
    Configuration conf2;
    conf2.init();
    
    double rpm_adj = conf2.get_rpm_adjustment();
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 2.75, rpm_adj);
}

#pragma endregion

#pragma region Device Name Tests

void test_configuration_get_device_name_default(void)
{
    Configuration conf;
    conf.init();
    
    const char *name = conf.get_device_name();
    TEST_ASSERT_NOT_NULL(name);
}

void test_configuration_save_device_name_short(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_device_name("MyDevice");
    
    const char *name = conf.get_device_name();
    TEST_ASSERT_EQUAL_STRING("MyDevice", name);
}

void test_configuration_save_device_name_single_char(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_device_name("A");
    
    const char *name = conf.get_device_name();
    TEST_ASSERT_EQUAL_STRING("A", name);
}

void test_configuration_save_device_name_empty_string(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_device_name("");
    
    const char *name = conf.get_device_name();
    TEST_ASSERT_EQUAL_STRING("", name);
}

void test_configuration_save_device_name_max_length(void)
{
    Configuration conf;
    conf.init();
    
    // Assuming device_name buffer is 16 bytes (typical)
    const char *long_name = "VeryLongDevice1";  // 15 chars
    conf.save_device_name(long_name);
    
    const char *name = conf.get_device_name();
    TEST_ASSERT_EQUAL_STRING(long_name, name);
}

void test_configuration_save_device_name_truncated_if_too_long(void)
{
    Configuration conf;
    conf.init();
    
    // This tests that names longer than buffer are truncated gracefully
    const char *very_long_name = "ThisIsAVeryLongDeviceNameThatExceedsTheBufferLimitAndShouldBeTruncated";
    conf.save_device_name(very_long_name);
    
    const char *name = conf.get_device_name();
    // Should be truncated to buffer size - 1
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_TRUE(strlen(name) < strlen(very_long_name));
}

void test_configuration_device_name_persisted(void)
{
    Configuration conf1;
    conf1.init();
    conf1.save_device_name("StoredDevice");
    
    Configuration conf2;
    conf2.init();
    
    const char *name = conf2.get_device_name();
    TEST_ASSERT_EQUAL_STRING("StoredDevice", name);
}

void test_configuration_device_name_with_special_chars(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_device_name("Device-123_v2");
    
    const char *name = conf.get_device_name();
    TEST_ASSERT_EQUAL_STRING("Device-123_v2", name);
}

#pragma endregion

#pragma region Battery Capacity Tests

void test_configuration_get_battery_capacity_default(void)
{
    Configuration conf;
    conf.init();
    
    uint32_t capacity = conf.get_batter_capacity();
    TEST_ASSERT_EQUAL_UINT32(DEFAULT_BATTERY_CAPACITY, capacity);
}

void test_configuration_save_battery_capacity_small(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_batter_capacity(50);
    
    uint32_t capacity = conf.get_batter_capacity();
    TEST_ASSERT_EQUAL_UINT32(50, capacity);
}

void test_configuration_save_battery_capacity_large(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_batter_capacity(10000);
    
    uint32_t capacity = conf.get_batter_capacity();
    TEST_ASSERT_EQUAL_UINT32(10000, capacity);
}

void test_configuration_save_battery_capacity_zero(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_batter_capacity(0);
    
    uint32_t capacity = conf.get_batter_capacity();
    TEST_ASSERT_EQUAL_UINT32(0, capacity);
}

void test_configuration_battery_capacity_max_uint32(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_batter_capacity(UINT16_MAX);
    
    uint32_t capacity = conf.get_batter_capacity();
    TEST_ASSERT_EQUAL_UINT32(UINT16_MAX, capacity);
}

void test_configuration_battery_capacity_persisted(void)
{
    Configuration conf1;
    conf1.init();
    conf1.save_batter_capacity(12000);
    
    Configuration conf2;
    conf2.init();
    
    uint32_t capacity = conf2.get_batter_capacity();
    TEST_ASSERT_EQUAL_UINT32(12000, capacity);
}

void test_configuration_battery_capacity_typical_values(void)
{
    Configuration conf;
    conf.init();
    
    // Test typical marine battery capacities
    uint32_t test_values[] = {100, 200, 500, 1000, 5000};
    
    for (size_t i = 0; i < sizeof(test_values)/sizeof(test_values[0]); i++)
    {
        conf.save_batter_capacity(test_values[i]);
        TEST_ASSERT_EQUAL_UINT32(test_values[i], conf.get_batter_capacity());
    }
}

#pragma endregion

#pragma region Meteo Source Tests

void test_configuration_get_pressure_source_bme_enabled(void)
{
    Configuration conf;
    conf.init();
    
    N2KServices svc = conf.get_services();
    svc.set_use_bme(true);
    conf.save_services(svc);
    
    MeteoSource source = conf.get_pressure_source();
    TEST_ASSERT_EQUAL_INT(METEO_BME, source);
}

void test_configuration_get_pressure_source_bme_disabled(void)
{
    Configuration conf;
    conf.init();
    
    N2KServices svc = conf.get_services();
    svc.set_use_bme(false);
    conf.save_services(svc);
    
    MeteoSource source = conf.get_pressure_source();
    TEST_ASSERT_EQUAL_INT(METEO_NONE, source);
}

void test_configuration_get_temperature_source_dht_enabled(void)
{
    Configuration conf;
    conf.init();
    
    N2KServices svc = conf.get_services();
    svc.set_use_dht(true);
    svc.set_use_bme(false);
    conf.save_services(svc);
    
    MeteoSource source = conf.get_temperature_source();
    TEST_ASSERT_EQUAL_INT(METEO_DHT, source);
}

void test_configuration_get_temperature_source_bme_fallback(void)
{
    Configuration conf;
    conf.init();
    
    N2KServices svc = conf.get_services();
    svc.set_use_dht(false);
    svc.set_use_bme(true);
    conf.save_services(svc);
    
    MeteoSource source = conf.get_temperature_source();
    TEST_ASSERT_EQUAL_INT(METEO_BME, source);
}

void test_configuration_get_temperature_source_priority_dht_over_bme(void)
{
    Configuration conf;
    conf.init();
    
    N2KServices svc = conf.get_services();
    svc.set_use_dht(true);
    svc.set_use_bme(true);
    conf.save_services(svc);
    
    MeteoSource source = conf.get_temperature_source();
    TEST_ASSERT_EQUAL_INT(METEO_DHT, source);  // DHT has priority
}

void test_configuration_get_temperature_source_none(void)
{
    Configuration conf;
    conf.init();
    
    N2KServices svc = conf.get_services();
    svc.set_use_dht(false);
    svc.set_use_bme(false);
    conf.save_services(svc);
    
    MeteoSource source = conf.get_temperature_source();
    TEST_ASSERT_EQUAL_INT(METEO_NONE, source);
}

void test_configuration_get_humidity_source_dht_enabled(void)
{
    Configuration conf;
    conf.init();
    
    N2KServices svc = conf.get_services();
    svc.set_use_dht(true);
    svc.set_use_bme(false);
    conf.save_services(svc);
    
    MeteoSource source = conf.get_humidity_source();
    TEST_ASSERT_EQUAL_INT(METEO_DHT, source);
}

void test_configuration_get_humidity_source_bme_fallback(void)
{
    Configuration conf;
    conf.init();
    
    N2KServices svc = conf.get_services();
    svc.set_use_dht(false);
    svc.set_use_bme(true);
    conf.save_services(svc);
    
    MeteoSource source = conf.get_humidity_source();
    TEST_ASSERT_EQUAL_INT(METEO_BME, source);
}

void test_configuration_get_humidity_source_priority_dht_over_bme(void)
{
    Configuration conf;
    conf.init();
    
    N2KServices svc = conf.get_services();
    svc.set_use_dht(true);
    svc.set_use_bme(true);
    conf.save_services(svc);
    
    MeteoSource source = conf.get_humidity_source();
    TEST_ASSERT_EQUAL_INT(METEO_DHT, source);  // DHT has priority
}

void test_configuration_get_temperature_el_source_both_enabled(void)
{
    Configuration conf;
    conf.init();
    
    N2KServices svc = conf.get_services();
    svc.set_use_dht(true);
    svc.set_use_bme(true);
    conf.save_services(svc);
    
    MeteoSource source = conf.get_temperature_el_source();
    TEST_ASSERT_EQUAL_INT(METEO_BME, source);
}

void test_configuration_get_temperature_el_source_only_dht(void)
{
    Configuration conf;
    conf.init();
    
    N2KServices svc = conf.get_services();
    svc.set_use_dht(true);
    svc.set_use_bme(false);
    conf.save_services(svc);
    
    MeteoSource source = conf.get_temperature_el_source();
    TEST_ASSERT_EQUAL_INT(METEO_NONE, source);
}

void test_configuration_get_temperature_el_source_only_bme(void)
{
    Configuration conf;
    conf.init();
    
    N2KServices svc = conf.get_services();
    svc.set_use_dht(false);
    svc.set_use_bme(true);
    conf.save_services(svc);
    
    MeteoSource source = conf.get_temperature_el_source();
    TEST_ASSERT_EQUAL_INT(METEO_NONE, source);
}

void test_configuration_get_temperature_el_source_none_enabled(void)
{
    Configuration conf;
    conf.init();
    
    N2KServices svc = conf.get_services();
    svc.set_use_dht(false);
    svc.set_use_bme(false);
    conf.save_services(svc);
    
    MeteoSource source = conf.get_temperature_el_source();
    TEST_ASSERT_EQUAL_INT(METEO_NONE, source);
}

#pragma endregion

#pragma region Integration Tests

void test_configuration_save_multiple_values(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_device_name("TestDevice");
    conf.save_rpm_adjustment(1.5);
    conf.save_n2k_source(50);
    conf.save_batter_capacity(5000);
    
    TEST_ASSERT_EQUAL_STRING("TestDevice", conf.get_device_name());
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1.5, conf.get_rpm_adjustment());
    TEST_ASSERT_EQUAL_INT(50, conf.get_n2k_source());
    TEST_ASSERT_EQUAL_UINT32(5000, conf.get_batter_capacity());
}

void test_configuration_persistence_roundtrip(void)
{
    // First configuration instance - set values
    {
        Configuration conf1;
        conf1.init();
        
        conf1.save_device_name("StoredDevice");
        conf1.save_rpm_adjustment(2.25);
        conf1.save_n2k_source(75);
        conf1.save_batter_capacity(8000);
        
        N2KServices svc;
        svc.set_use_gps(true);
        svc.set_use_bme(true);
        svc.set_use_dht(false);
        conf1.save_services(svc);
    }
    
    // Second configuration instance - verify persistence
    {
        Configuration conf2;
        conf2.init();
        
        TEST_ASSERT_EQUAL_STRING("StoredDevice", conf2.get_device_name());
        TEST_ASSERT_DOUBLE_WITHIN(0.0001, 2.25, conf2.get_rpm_adjustment());
        TEST_ASSERT_EQUAL_INT(75, conf2.get_n2k_source());
        TEST_ASSERT_EQUAL_UINT32(8000, conf2.get_batter_capacity());
        
        const N2KServices &svc = conf2.get_services();
        TEST_ASSERT_TRUE(svc.is_use_gps());
        TEST_ASSERT_TRUE(svc.is_use_bme());
        TEST_ASSERT_FALSE(svc.is_use_dht());
    }
}

void test_configuration_independent_instances(void)
{
    Configuration conf1;
    conf1.init();
    conf1.save_device_name("Device1");
    
    Configuration conf2;
    conf2.init();
    conf2.save_device_name("Device2");
    
    // Both should reflect their own saves
    TEST_ASSERT_EQUAL_STRING("Device1", conf1.get_device_name());
    TEST_ASSERT_EQUAL_STRING("Device2", conf2.get_device_name());
}

void test_configuration_overwrite_values(void)
{
    Configuration conf;
    conf.init();
    
    conf.save_device_name("FirstName");
    TEST_ASSERT_EQUAL_STRING("FirstName", conf.get_device_name());
    
    conf.save_device_name("SecondName");
    TEST_ASSERT_EQUAL_STRING("SecondName", conf.get_device_name());
    
    conf.save_rpm_adjustment(1.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1.0, conf.get_rpm_adjustment());
    
    conf.save_rpm_adjustment(-1.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, -1.0, conf.get_rpm_adjustment());
}

#pragma endregion

// Test runner
void run_configuration_tests(void)
{
    // Initialization
    RUN_TEST(test_configuration_constructor);
    RUN_TEST(test_configuration_init_sets_initialized_flag);
    RUN_TEST(test_configuration_init_idempotent);
    
    // N2KServices
    RUN_TEST(test_configuration_get_services_returns_services_object);
    RUN_TEST(test_configuration_save_services_updates_configuration);
    RUN_TEST(test_configuration_services_persisted_after_save);
    
    // N2K Source
    RUN_TEST(test_configuration_get_n2k_source_default);
    RUN_TEST(test_configuration_save_n2k_source);
    RUN_TEST(test_configuration_n2k_source_persisted);
    RUN_TEST(test_configuration_n2k_source_boundary_values);
    
    // RPM Adjustment
    RUN_TEST(test_configuration_get_rpm_adjustment_default);
    RUN_TEST(test_configuration_save_rpm_adjustment_positive);
    RUN_TEST(test_configuration_save_rpm_adjustment_negative);
    RUN_TEST(test_configuration_save_rpm_adjustment_zero);
    RUN_TEST(test_configuration_rpm_adjustment_precision);
    RUN_TEST(test_configuration_rpm_adjustment_large_values);
    RUN_TEST(test_configuration_rpm_adjustment_persisted);
    
    // Device Name
    RUN_TEST(test_configuration_get_device_name_default);
    RUN_TEST(test_configuration_save_device_name_short);
    RUN_TEST(test_configuration_save_device_name_single_char);
    RUN_TEST(test_configuration_save_device_name_empty_string);
    RUN_TEST(test_configuration_save_device_name_max_length);
    RUN_TEST(test_configuration_save_device_name_truncated_if_too_long);
    RUN_TEST(test_configuration_device_name_persisted);
    RUN_TEST(test_configuration_device_name_with_special_chars);
    
    // Battery Capacity
    RUN_TEST(test_configuration_get_battery_capacity_default);
    RUN_TEST(test_configuration_save_battery_capacity_small);
    RUN_TEST(test_configuration_save_battery_capacity_large);
    RUN_TEST(test_configuration_save_battery_capacity_zero);
    RUN_TEST(test_configuration_battery_capacity_max_uint32);
    RUN_TEST(test_configuration_battery_capacity_persisted);
    RUN_TEST(test_configuration_battery_capacity_typical_values);

    // Meteo Sources
    RUN_TEST(test_configuration_get_pressure_source_bme_enabled);
    RUN_TEST(test_configuration_get_pressure_source_bme_disabled);
    RUN_TEST(test_configuration_get_temperature_source_dht_enabled);
    RUN_TEST(test_configuration_get_temperature_source_bme_fallback);
    RUN_TEST(test_configuration_get_temperature_source_priority_dht_over_bme);
    RUN_TEST(test_configuration_get_temperature_source_none);
    RUN_TEST(test_configuration_get_humidity_source_dht_enabled);
    RUN_TEST(test_configuration_get_humidity_source_bme_fallback);
    RUN_TEST(test_configuration_get_humidity_source_priority_dht_over_bme);
    RUN_TEST(test_configuration_get_temperature_el_source_both_enabled);
    RUN_TEST(test_configuration_get_temperature_el_source_only_dht);
    RUN_TEST(test_configuration_get_temperature_el_source_only_bme);
    RUN_TEST(test_configuration_get_temperature_el_source_none_enabled);
    
    // Integration Tests
    RUN_TEST(test_configuration_save_multiple_values);
    RUN_TEST(test_configuration_persistence_roundtrip);
    RUN_TEST(test_configuration_independent_instances);
    RUN_TEST(test_configuration_overwrite_values);
}
#endif