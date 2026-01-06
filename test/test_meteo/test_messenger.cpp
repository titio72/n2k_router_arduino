#include <unity.h>
#include <math.h>
#include "EnvMessenger.h"
#include "Conf.h"
#include "Context.h"
#include "Data.h"
#include "N2K_router.h"

class MockConfX: public MockConfiguration
{
public:
    MockConfX()
        : MockConfiguration()
    {
    }

    MeteoSource pressure_source = METEO_BME;
    MeteoSource temperature_source = METEO_BME;
    MeteoSource temperature_el_source = METEO_BME;
    MeteoSource humidity_source = METEO_BME;

    virtual MeteoSource get_pressure_source() const override
    {
        return pressure_source;
    }
    virtual MeteoSource get_temperature_source() const override
    {
        return temperature_source;
    }
    virtual MeteoSource get_temperature_el_source() const override
    {
        return temperature_el_source;
    }
    virtual MeteoSource get_humidity_source() const override
    {
        return humidity_source;
    }
};


#define MOCK_CONTEXT_TEST_MESSENGER \
Data data; \
TestN2KSender n2kSender; \
MockConfX mockConf; \
Context context = {n2kSender, mockConf, data};


// Extended N2KSender to track calls
class TestN2KSender : public NullN2KSender
{
public:
    int pressure_calls = 0;
    int humidity_calls = 0;
    int cabin_temp_calls = 0;
    int electronic_temp_calls = 0;
    int raymar_env_calls = 0;

    bool sendPressure(const double pressure, unsigned char sid = 0xFF) override
    {
        pressure_calls++;
        return NullN2KSender::sendPressure(pressure, sid);
    }

    bool sendHumidity(const double humidity, unsigned char sid = 0xFF) override
    {
        humidity_calls++;
        return NullN2KSender::sendHumidity(humidity, sid);
    }

    bool sendCabinTemp(const double temperature, unsigned char sid = 0xFF) override
    {
        cabin_temp_calls++;
        return NullN2KSender::sendCabinTemp(temperature, sid);
    }

    bool sendElectronicTemperature(const double temp, unsigned char sid = 0xFF) override
    {
        electronic_temp_calls++;
        return NullN2KSender::sendElectronicTemperature(temp, sid);
    }

    bool sendEnvironmentXRaymarine(const double pressure, const double humidity, const double temperature) override
    {
        raymar_env_calls++;
        return NullN2KSender::sendEnvironmentXRaymarine(pressure, humidity, temperature);
    }

    void reset_calls()
    {
        pressure_calls = 0;
        humidity_calls = 0;
        cabin_temp_calls = 0;
        electronic_temp_calls = 0;
        raymar_env_calls = 0;
    }
};

// ============== Constructor Tests ==============

void test_env_constructor_initializes_disabled(void)
{
    EnvMessenger env;
    TEST_ASSERT_FALSE(env.is_enabled());
}

// ============== Enable/Disable Tests ==============

void test_env_enable(void)
{
    EnvMessenger env;
    env.enable();
    TEST_ASSERT_TRUE(env.is_enabled());
}

void test_env_disable(void)
{
    EnvMessenger env;
    env.enable();
    env.disable();
    TEST_ASSERT_FALSE(env.is_enabled());
}

void test_env_disable_when_already_disabled(void)
{
    EnvMessenger env;
    env.disable();
    TEST_ASSERT_FALSE(env.is_enabled());
}

void test_env_multiple_enable_calls(void)
{
    EnvMessenger env;
    env.enable();
    env.enable();
    TEST_ASSERT_TRUE(env.is_enabled());
}

void test_env_multiple_disable_calls(void)
{
    EnvMessenger env;
    env.enable();
    env.disable();
    env.disable();
    TEST_ASSERT_FALSE(env.is_enabled());
}

// ============== Setup Tests ==============

void test_env_setup_when_disabled(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    EnvMessenger env;
    env.setup(context);
    TEST_ASSERT_FALSE(env.is_enabled());
}

void test_env_setup_when_enabled(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    EnvMessenger env;
    env.enable();
    env.setup(context);
    TEST_ASSERT_TRUE(env.is_enabled());
}

// ============== Loop Tests - Period Control ==============

void test_env_loop_does_nothing_when_disabled(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    data.meteo_0.pressure = 101300.0;
    data.meteo_0.temperature = 22.5;
    data.meteo_0.humidity = 65.0;

    EnvMessenger env;
    env.loop(0, context);
    TEST_ASSERT_EQUAL_INT(0, n2kSender.getStats().sent);
}

void test_env_loop_sends_on_period_elapsed(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    data.meteo_0.pressure = 101300.0;
    data.meteo_0.temperature = 22.5;
    data.meteo_0.humidity = 65.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);

    env.loop(0, context);
    unsigned int sent_before = n2kSender.getStats().sent;

    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;
    
    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

void test_env_loop_does_not_send_within_period(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    data.meteo_0.pressure = 101300.0;
    data.meteo_0.temperature = 22.5;
    data.meteo_0.humidity = 65.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);

    env.loop(10000000, context);
    unsigned int sent_before = n2kSender.getStats().sent;

    env.loop(10000000 + PERIOD_MICROS_ENV - 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;
    
    TEST_ASSERT_EQUAL_INT(sent_before, sent_after);
}

void test_env_loop_multiple_periods(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    data.meteo_0.pressure = 101300.0;
    data.meteo_0.temperature = 22.5;
    data.meteo_0.humidity = 65.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);

    env.loop(0, context);
    unsigned int sent_first = n2kSender.getStats().sent;

    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_second = n2kSender.getStats().sent;
    
    TEST_ASSERT_GREATER_THAN_INT(sent_first, sent_second);

    env.loop(PERIOD_MICROS_ENV * 2 + 1, context);
    unsigned int sent_third = n2kSender.getStats().sent;
    
    TEST_ASSERT_GREATER_THAN_INT(sent_second, sent_third);
}

// ============== Pressure Source Tests ==============

void test_env_sends_pressure_from_bme(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.pressure_source = METEO_BME;
    data.meteo_0.pressure = 101325.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

void test_env_skips_pressure_when_source_none(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.pressure_source = METEO_NONE;
    data.meteo_0.pressure = 101325.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;
    
    // Should not send pressure message
    TEST_ASSERT_EQUAL_INT(sent_before, sent_after);
}

void test_env_skips_nan_pressure(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.pressure_source = METEO_BME;
    data.meteo_0.pressure = NAN;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    // Should still send other messages or none, but definitely not pressure
    TEST_ASSERT_EQUAL_INT(sent_before, sent_after);
}

// ============== Temperature Source Tests ==============

void test_env_sends_cabin_temp_from_bme(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.temperature_source = METEO_BME;
    data.meteo_0.temperature = 22.5;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

void test_env_sends_cabin_temp_from_dht(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.temperature_source = METEO_DHT;
    data.meteo_1.temperature = 23.5;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

void test_env_skips_nan_temperature(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.temperature_source = METEO_BME;
    data.meteo_0.temperature = NAN;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    TEST_ASSERT_EQUAL_INT(sent_before, sent_after);
}

void test_env_skips_temperature_when_source_none(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.temperature_source = METEO_NONE;
    mockConf.temperature_el_source = METEO_NONE;
    data.meteo_0.temperature = 22.5;
    data.meteo_1.temperature = 23.5;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    TEST_ASSERT_EQUAL_INT(sent_before, sent_after);
}

// ============== Humidity Source Tests ==============

void test_env_sends_humidity_from_bme(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.humidity_source = METEO_BME;
    data.meteo_0.humidity = 65.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

void test_env_sends_humidity_from_dht(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.humidity_source = METEO_DHT;
    data.meteo_1.humidity = 70.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

void test_env_skips_nan_humidity(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.humidity_source = METEO_BME;
    data.meteo_0.humidity = NAN;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    TEST_ASSERT_EQUAL_INT(sent_before, sent_after);
}

void test_env_skips_humidity_when_source_none(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.humidity_source = METEO_NONE;
    data.meteo_0.humidity = 65.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    TEST_ASSERT_EQUAL_INT(sent_before, sent_after);
}

// ============== Electronic Temperature Tests ==============

void test_env_sends_electronic_temp_from_bme(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.temperature_el_source = METEO_BME;
    data.meteo_0.temperature = 25.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(10000000, context);
    
    unsigned int sent_before = n2kSender.electronic_temp_calls;
    env.loop(10000000 + PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.electronic_temp_calls;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

void test_env_skips_nan_electronic_temp(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.temperature_el_source = METEO_BME;
    data.meteo_0.temperature = NAN;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.electronic_temp_calls;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.electronic_temp_calls;

    TEST_ASSERT_EQUAL_INT(sent_before, sent_after);
}

void test_env_skips_electronic_temp_when_source_none(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.temperature_el_source = METEO_NONE;
    data.meteo_0.temperature = 25.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.electronic_temp_calls;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.electronic_temp_calls;

    TEST_ASSERT_EQUAL_INT(sent_before, sent_after);
}

// ============== Raymarine Environment Tests ==============

void test_env_sends_raymar_env_with_all_valid_values(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    data.meteo_0.pressure = 101300.0;
    data.meteo_0.temperature = 22.5;
    data.meteo_0.humidity = 65.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.raymar_env_calls;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.raymar_env_calls;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

void test_env_sends_raymar_env_with_pressure_only(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    data.meteo_0.pressure = 101300.0;
    data.meteo_0.temperature = NAN;
    data.meteo_0.humidity = NAN;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.raymar_env_calls;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.raymar_env_calls;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

void test_env_skips_raymar_env_when_all_nan(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    data.meteo_0.pressure = NAN;
    data.meteo_0.temperature = NAN;
    data.meteo_0.humidity = NAN;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.raymar_env_calls;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.raymar_env_calls;

    TEST_ASSERT_EQUAL_INT(sent_before, sent_after);
}

void test_env_sends_raymar_env_with_only_humidity_valid(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    data.meteo_0.pressure = NAN;
    data.meteo_0.temperature = NAN;
    data.meteo_0.humidity = 65.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.raymar_env_calls;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.raymar_env_calls;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

void test_env_sends_raymar_env_with_only_temp_valid(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    data.meteo_0.pressure = NAN;
    data.meteo_0.temperature = 22.5;
    data.meteo_0.humidity = NAN;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.raymar_env_calls;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.raymar_env_calls;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

// ============== Integration Tests ==============

void test_env_full_lifecycle_with_one_source(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.pressure_source = METEO_BME;
    mockConf.temperature_source = METEO_BME;
    mockConf.humidity_source = METEO_BME;
    mockConf.temperature_el_source = METEO_NONE;
    data.meteo_0.pressure = 101300.0;
    data.meteo_0.temperature = 22.5;
    data.meteo_0.humidity = 65.0;
    data.meteo_1.temperature = 25.0; // Should be ignored

    EnvMessenger env;
    TEST_ASSERT_FALSE(env.is_enabled());

    env.setup(context);
    TEST_ASSERT_FALSE(env.is_enabled());

    env.enable();
    TEST_ASSERT_TRUE(env.is_enabled());

    env.loop(0, context);
    n2kSender.reset_calls();

    env.loop(PERIOD_MICROS_ENV + 1, context);
    TEST_ASSERT_EQUAL_INT(0, n2kSender.electronic_temp_calls);
    TEST_ASSERT_EQUAL_INT(1, n2kSender.pressure_calls);
    TEST_ASSERT_EQUAL_INT(1, n2kSender.cabin_temp_calls);
    TEST_ASSERT_EQUAL_INT(1, n2kSender.humidity_calls);
    TEST_ASSERT_EQUAL_INT(1, n2kSender.raymar_env_calls);
    n2kSender.reset_calls();

    env.disable();
    TEST_ASSERT_FALSE(env.is_enabled());

    unsigned int sent_disabled = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV * 2 + 1, context);
    TEST_ASSERT_EQUAL_INT(sent_disabled, n2kSender.getStats().sent);
}

void test_env_full_lifecycle_with_two_source(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.pressure_source = METEO_BME;
    mockConf.temperature_source = METEO_BME;
    mockConf.humidity_source = METEO_BME;
    mockConf.temperature_el_source = METEO_DHT;
    data.meteo_0.pressure = 101300.0;
    data.meteo_0.temperature = 22.5;
    data.meteo_0.humidity = 65.0;
    data.meteo_1.temperature = 25.0;

    EnvMessenger env;
    TEST_ASSERT_FALSE(env.is_enabled());

    env.setup(context);
    TEST_ASSERT_FALSE(env.is_enabled());

    env.enable();
    TEST_ASSERT_TRUE(env.is_enabled());

    env.loop(0, context);
    n2kSender.reset_calls();

    env.loop(PERIOD_MICROS_ENV + 1, context);
    TEST_ASSERT_EQUAL_INT(1, n2kSender.electronic_temp_calls);
    TEST_ASSERT_EQUAL_INT(1, n2kSender.pressure_calls);
    TEST_ASSERT_EQUAL_INT(1, n2kSender.cabin_temp_calls);
    TEST_ASSERT_EQUAL_INT(1, n2kSender.humidity_calls);
    TEST_ASSERT_EQUAL_INT(1, n2kSender.raymar_env_calls);
    n2kSender.reset_calls();

    env.disable();
    TEST_ASSERT_FALSE(env.is_enabled());

    unsigned int sent_disabled = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV * 2 + 1, context);
    TEST_ASSERT_EQUAL_INT(sent_disabled, n2kSender.getStats().sent);
}

void test_env_dynamic_meteo_source_changes(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    mockConf.temperature_source = METEO_BME;
    data.meteo_0.temperature = 22.5;
    data.meteo_1.temperature = 25.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after_bme = n2kSender.getStats().sent;

    mockConf.temperature_source = METEO_DHT;
    unsigned int sent_before_dht = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV * 2 + 1, context);
    unsigned int sent_after_dht = n2kSender.getStats().sent;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after_bme);
    TEST_ASSERT_GREATER_THAN_INT(sent_before_dht, sent_after_dht);
}

void test_env_empty_data_update_cycle(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    // All values are NAN by default

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    // Should not send any messages since all values are NAN
    TEST_ASSERT_EQUAL_INT(sent_before, sent_after);
}

void test_env_mixed_nan_values(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    data.meteo_0.pressure = 101300.0;
    data.meteo_0.temperature = NAN;
    data.meteo_0.humidity = 65.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

void test_env_high_pressure_values(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    data.meteo_0.pressure = 110000.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

void test_env_extreme_temperatures(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    data.meteo_0.temperature = -40.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

void test_env_edge_case_humidity_0(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    data.meteo_0.humidity = 0.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

void test_env_edge_case_humidity_100(void)
{
    MOCK_CONTEXT_TEST_MESSENGER
    data.meteo_0.humidity = 100.0;

    EnvMessenger env;
    env.enable();
    env.setup(context);
    env.loop(0, context);
    
    unsigned int sent_before = n2kSender.getStats().sent;
    env.loop(PERIOD_MICROS_ENV + 1, context);
    unsigned int sent_after = n2kSender.getStats().sent;

    TEST_ASSERT_GREATER_THAN_INT(sent_before, sent_after);
}

// ============== Main Test Runner ==============

void setUpEnvMessenger() {}

void tearDownEnvMessenger() {}

void runEnvMessengerTests()
{
    UNITY_BEGIN();

    // Constructor Tests
    RUN_TEST(test_env_constructor_initializes_disabled);

    // Enable/Disable Tests
    RUN_TEST(test_env_enable);
    RUN_TEST(test_env_disable);
    RUN_TEST(test_env_disable_when_already_disabled);
    RUN_TEST(test_env_multiple_enable_calls);
    RUN_TEST(test_env_multiple_disable_calls);

    // Setup Tests
    RUN_TEST(test_env_setup_when_disabled);
    RUN_TEST(test_env_setup_when_enabled);

    // Loop Tests - Period Control
    RUN_TEST(test_env_loop_does_nothing_when_disabled);
    RUN_TEST(test_env_loop_sends_on_period_elapsed);
    RUN_TEST(test_env_loop_does_not_send_within_period);
    RUN_TEST(test_env_loop_multiple_periods);

    // Pressure Source Tests
    RUN_TEST(test_env_sends_pressure_from_bme);
    RUN_TEST(test_env_skips_pressure_when_source_none);
    RUN_TEST(test_env_skips_nan_pressure);

    // Temperature Source Tests
    RUN_TEST(test_env_sends_cabin_temp_from_bme);
    RUN_TEST(test_env_sends_cabin_temp_from_dht);
    RUN_TEST(test_env_skips_nan_temperature);
    RUN_TEST(test_env_skips_temperature_when_source_none);

    // Humidity Source Tests
    RUN_TEST(test_env_sends_humidity_from_bme);
    RUN_TEST(test_env_sends_humidity_from_dht);
    RUN_TEST(test_env_skips_nan_humidity);
    RUN_TEST(test_env_skips_humidity_when_source_none);

    // Electronic Temperature Tests
    RUN_TEST(test_env_sends_electronic_temp_from_bme);
    RUN_TEST(test_env_skips_nan_electronic_temp);
    RUN_TEST(test_env_skips_electronic_temp_when_source_none);

    // RaymarineEnvironment Tests
    RUN_TEST(test_env_sends_raymar_env_with_all_valid_values);
    RUN_TEST(test_env_sends_raymar_env_with_pressure_only);
    RUN_TEST(test_env_skips_raymar_env_when_all_nan);
    RUN_TEST(test_env_sends_raymar_env_with_only_humidity_valid);
    RUN_TEST(test_env_sends_raymar_env_with_only_temp_valid);

    // Integration Tests
    RUN_TEST(test_env_full_lifecycle_with_one_source);
    RUN_TEST(test_env_full_lifecycle_with_two_source);
    RUN_TEST(test_env_dynamic_meteo_source_changes);
    RUN_TEST(test_env_empty_data_update_cycle);
    RUN_TEST(test_env_mixed_nan_values);
    RUN_TEST(test_env_high_pressure_values);
    RUN_TEST(test_env_extreme_temperatures);
    RUN_TEST(test_env_edge_case_humidity_0);
    RUN_TEST(test_env_edge_case_humidity_100);

    UNITY_END();
}