#include <unity.h>
#include <string.h>
#include "CommandHandler.hpp"
#include "Conf.h"
#include "Context.h"
#include "Data.h"
#include "N2K_router.h"
#include <Log.h>

// ============== Mock ConfigurationRW for testing ==============
// Extends MockConfiguration to track save operations

#define MOCK_CONTEXT_TEST \
Data data; \
NullN2KSender n2kSender; \
MockConfiguration conf; \
MockEngineHours engineHours; \
Context context = {n2kSender, conf, data};

// ============== Tests: Command 'S' (Switch Services) ==============

void test_command_S_single_service_enabled(void)
{
    MOCK_CONTEXT_TEST

    // Enable GPS only
    CommandHandler::on_command('S', "1000000", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_services_calls);
    TEST_ASSERT_TRUE(conf.get_services().is_use_gps());
    TEST_ASSERT_FALSE(conf.get_services().is_use_dht());
    TEST_ASSERT_FALSE(conf.get_services().is_use_bme());
}

void test_command_S_multiple_services_enabled(void)
{
    MOCK_CONTEXT_TEST

    // Enable GPS, DHT, and BME
    CommandHandler::on_command('S', "1110000", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_services_calls);
    TEST_ASSERT_TRUE(conf.get_services().is_use_gps());
    TEST_ASSERT_TRUE(conf.get_services().is_use_dht());
    TEST_ASSERT_TRUE(conf.get_services().is_use_bme());
}

void test_command_S_all_services_enabled(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('S', "1111111", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_services_calls);
    TEST_ASSERT_TRUE(conf.get_services().is_use_gps());
    TEST_ASSERT_TRUE(conf.get_services().is_use_dht());
    TEST_ASSERT_TRUE(conf.get_services().is_use_bme());
    TEST_ASSERT_TRUE(conf.get_services().is_send_time());
    TEST_ASSERT_TRUE(conf.get_services().is_use_tacho());
    TEST_ASSERT_TRUE(conf.get_services().is_sog_2_stw());
    TEST_ASSERT_TRUE(conf.get_services().is_use_vedirect());
}

void test_command_S_all_services_disabled(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('S', "0000000", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_services_calls);
    TEST_ASSERT_FALSE(conf.get_services().is_use_gps());
    TEST_ASSERT_FALSE(conf.get_services().is_use_dht());
    TEST_ASSERT_FALSE(conf.get_services().is_use_bme());
}

void test_command_S_mixed_services(void)
{
    MOCK_CONTEXT_TEST

    // GPS, BME, Time, VE.Direct enabled; DHT, Tacho, STW disabled
    CommandHandler::on_command('S', "1011101", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_services_calls);
    TEST_ASSERT_TRUE(conf.get_services().is_use_gps());
    TEST_ASSERT_FALSE(conf.get_services().is_use_dht());
    TEST_ASSERT_TRUE(conf.get_services().is_use_bme());
    TEST_ASSERT_TRUE(conf.get_services().is_send_time());
    TEST_ASSERT_TRUE(conf.get_services().is_use_tacho());
    TEST_ASSERT_FALSE(conf.get_services().is_sog_2_stw());
    TEST_ASSERT_TRUE(conf.get_services().is_use_vedirect());
}

void test_command_S_empty_string(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('S', "", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_services_calls);
}

// ============== Tests: Command 'N' (Set Device Name) ==============

void test_command_N_simple_device_name(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('N', "MyDevice", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_device_name_calls);
    TEST_ASSERT_EQUAL_STRING("MyDevice", conf.get_device_name());
}

void test_command_N_device_name_with_numbers(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('N', "Device123", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_device_name_calls);
    TEST_ASSERT_EQUAL_STRING("Device123", conf.get_device_name());
}

void test_command_N_device_name_with_special_chars(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('N', "My-Device_01", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_device_name_calls);
    TEST_ASSERT_EQUAL_STRING("My-Device_01", conf.get_device_name());
}

void test_command_N_max_length_device_name(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('N', "MyDeviceName12345", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_device_name_calls);
    TEST_ASSERT_EQUAL_STRING("MyDeviceName123", conf.get_device_name()); // expected to be trimmed at 15 chars
}

void test_command_N_empty_device_name(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('N', "", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_device_name_calls);
    TEST_ASSERT_EQUAL_STRING("", conf.get_device_name());
}

void test_command_N_single_character_name(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('N', "X", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_device_name_calls);
    TEST_ASSERT_EQUAL_STRING("X", conf.get_device_name());
}

// ============== Tests: Command 'C' (Set Services) ==============

void test_command_C_set_single_service(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('C', "1000000", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_services_calls);
    TEST_ASSERT_TRUE(conf.get_services().is_use_gps());
}

void test_command_C_set_all_services(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('C', "1111111",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_services_calls);
    TEST_ASSERT_TRUE(conf.get_services().is_use_gps());
    TEST_ASSERT_TRUE(conf.get_services().is_use_dht());
    TEST_ASSERT_TRUE(conf.get_services().is_use_bme());
}

void test_command_C_clear_all_services(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('C', "0000000",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_services_calls);
    TEST_ASSERT_FALSE(conf.get_services().is_use_gps());
    TEST_ASSERT_FALSE(conf.get_services().is_use_dht());
}

// ============== Tests: Command 'H' (Set Engine Hours) ==============

void test_command_H_zero_hours(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('H', "0",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(0, engineHours.save_engine_hours_calls);
}

void test_command_H_positive_hours(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('H', "100",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, engineHours.save_engine_hours_calls);
    TEST_ASSERT_EQUAL_UINT64(100000, engineHours.get_engine_hours());
}

void test_command_H_large_hours_value(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('H', "86400",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, engineHours.save_engine_hours_calls);
    TEST_ASSERT_EQUAL_UINT64(86400000, engineHours.get_engine_hours());
}

void test_command_H_very_large_hours_value(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('H', "31536000",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, engineHours.save_engine_hours_calls);
    TEST_ASSERT_EQUAL_UINT64(31536000000ULL, engineHours.get_engine_hours());
}

void test_command_H_single_second(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('H', "1",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, engineHours.save_engine_hours_calls);
    TEST_ASSERT_EQUAL_UINT64(1000, engineHours.get_engine_hours());
}

void test_command_H_negative_hours_rejected(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('H', "-100",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(0, engineHours.save_engine_hours_calls);
}

void test_command_H_empty_string(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('H', "",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(0, engineHours.save_engine_hours_calls);
}

void test_command_H_non_numeric_string(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('H', "abc",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(0, engineHours.save_engine_hours_calls);
}

// ============== Tests: Command 'T' (Tachometer Calibration) ==============

void test_command_T_calibration_with_valid_rpm(void)
{
    MOCK_CONTEXT_TEST

    conf.save_rpm_adjustment(1.0);
    conf.reset_call_counts();
    data.engine.rpm = 1000;

    CommandHandler::on_command('T', "2000", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_rpm_adjustment_calls);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 2.0, conf.get_rpm_adjustment());
}

void test_command_T_calibration_halves_adjustment(void)
{
    MOCK_CONTEXT_TEST

    conf.save_rpm_adjustment(2.0);
    conf.reset_call_counts();
    data.engine.rpm = 4000;

    CommandHandler::on_command('T', "2000",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_rpm_adjustment_calls);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, conf.get_rpm_adjustment());
}

void test_command_T_calibration_doubles_adjustment(void)
{
    MOCK_CONTEXT_TEST

    conf.save_rpm_adjustment(1.0);
    conf.reset_call_counts();
    data.engine.rpm = 1000;

    CommandHandler::on_command('T', "4000",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_rpm_adjustment_calls);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 4.0, conf.get_rpm_adjustment());
}

void test_command_T_zero_rpm_rejected(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('T', "0",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(0, conf.save_rpm_adjustment_calls);
}

void test_command_T_negative_rpm_rejected(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('T', "-1000",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(0, conf.save_rpm_adjustment_calls);
}

void test_command_T_empty_string(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('T', "",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(0, conf.save_rpm_adjustment_calls);
}

void test_command_T_non_numeric_string(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('T', "rpm",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(0, conf.save_rpm_adjustment_calls);
}

// ============== Tests: Command 't' (Tachometer Adjustment) ==============

void test_command_lowercase_t_adjustment(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('t', "250",conf, engineHours, data); // ratio 0.01

    TEST_ASSERT_EQUAL_INT(1, conf.save_rpm_adjustment_calls);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 2.5, conf.get_rpm_adjustment());
}

void test_command_lowercase_t_adjustment_100(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('t', "100",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_rpm_adjustment_calls);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, conf.get_rpm_adjustment());
}

void test_command_lowercase_t_adjustment_500(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('t', "500",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_rpm_adjustment_calls);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 5.0, conf.get_rpm_adjustment());
}

void test_command_lowercase_t_zero_rejected(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('t', "0",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(0, conf.save_rpm_adjustment_calls);
}

void test_command_lowercase_t_negative_rejected(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('t', "-10000",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(0, conf.save_rpm_adjustment_calls);
}

void test_command_lowercase_t_empty_string(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('t', "",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(0, conf.save_rpm_adjustment_calls);
}

// ============== Tests: Unknown Commands ==============

void test_command_unknown_command(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('X', "value",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(0, conf.save_device_name_calls);
    TEST_ASSERT_EQUAL_INT(0, conf.save_services_calls);
    TEST_ASSERT_EQUAL_INT(0, engineHours.save_engine_hours_calls);
    TEST_ASSERT_EQUAL_INT(0, conf.save_rpm_adjustment_calls);
}

void test_command_unknown_command_lowercase_x(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('z', "value",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(0, conf.save_device_name_calls);
}

void test_command_unknown_command_numeric(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('5', "value",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(0, conf.save_device_name_calls);
}

// ============== Integration Tests ==============

void test_command_sequence_name_then_services(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('N', "TestDevice",conf, engineHours, data);
    CommandHandler::on_command('S', "1111111",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_device_name_calls);
    TEST_ASSERT_EQUAL_INT(1, conf.save_services_calls);
    TEST_ASSERT_EQUAL_STRING("TestDevice", conf.get_device_name());
}

void test_command_sequence_services_then_hours(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('S', "1010101",conf, engineHours, data);
    CommandHandler::on_command('H', "3600",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_services_calls);
    TEST_ASSERT_EQUAL_INT(1, engineHours.save_engine_hours_calls);
    TEST_ASSERT_EQUAL_UINT64(3600000, engineHours.get_engine_hours());
}

void test_command_sequence_all_command_types(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('N', "Device",conf, engineHours, data);
    CommandHandler::on_command('S', "1110000",conf, engineHours, data);
    CommandHandler::on_command('H', "1000",conf, engineHours, data);
    
    conf.save_rpm_adjustment(1.0);
    data.engine.rpm = 1000;
    conf.reset_call_counts();
    
    CommandHandler::on_command('T', "3000",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_rpm_adjustment_calls);
}

void test_command_repeated_same_command(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('N', "Device1",conf, engineHours, data);
    CommandHandler::on_command('N', "Device2",conf, engineHours, data);
    CommandHandler::on_command('N', "Device3",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(3, conf.save_device_name_calls);
    TEST_ASSERT_EQUAL_STRING("Device3", conf.get_device_name());
}

// ============== Edge Cases and Boundary Tests ==============

void test_command_with_very_long_device_name(void)
{
    MOCK_CONTEXT_TEST

    const char *very_long = "VeryLongDeviceNameThatShouldBeTruncated";
    CommandHandler::on_command('N', very_long,conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_device_name_calls);
}

void test_command_H_with_string_containing_number(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('H', "123abc",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, engineHours.save_engine_hours_calls);
    TEST_ASSERT_EQUAL_UINT64(123000, engineHours.get_engine_hours());
}

void test_command_T_with_high_rpm_value(void)
{
    MOCK_CONTEXT_TEST

    conf.save_rpm_adjustment(1.0);
    conf.reset_call_counts();
    data.engine.rpm = 1000;

    CommandHandler::on_command('T', "10000", conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_rpm_adjustment_calls);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 10.0, conf.get_rpm_adjustment());
}

void test_command_lowercase_t_with_fractional_result(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('t', "150",conf, engineHours, data);

    TEST_ASSERT_EQUAL_INT(1, conf.save_rpm_adjustment_calls);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.5, conf.get_rpm_adjustment());
}

void test_command_S_service_parsing_index_0(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('S', "1000000",conf, engineHours, data);

    TEST_ASSERT_TRUE(conf.get_services().is_use_gps());
}

void test_command_S_service_parsing_index_6(void)
{
    MOCK_CONTEXT_TEST

    CommandHandler::on_command('S', "0000001",conf, engineHours, data);

    TEST_ASSERT_TRUE(conf.get_services().is_use_vedirect());
    TEST_ASSERT_FALSE(conf.get_services().is_use_gps());
}

// ============== Main Test Runner ==============

int main(int argc, char **argv)
{
    Log::enable();

    UNITY_BEGIN();

    // Command 'S' Tests
    RUN_TEST(test_command_S_single_service_enabled);
    RUN_TEST(test_command_S_multiple_services_enabled);
    RUN_TEST(test_command_S_all_services_enabled);
    RUN_TEST(test_command_S_all_services_disabled);
    RUN_TEST(test_command_S_mixed_services);
    RUN_TEST(test_command_S_empty_string);
    RUN_TEST(test_command_S_service_parsing_index_0);
    RUN_TEST(test_command_S_service_parsing_index_6);

    // Command 'N' Tests
    RUN_TEST(test_command_N_simple_device_name);
    RUN_TEST(test_command_N_device_name_with_numbers);
    RUN_TEST(test_command_N_device_name_with_special_chars);
    RUN_TEST(test_command_N_max_length_device_name);
    RUN_TEST(test_command_N_empty_device_name);
    RUN_TEST(test_command_N_single_character_name);

    // Command 'C' Tests
    RUN_TEST(test_command_C_set_single_service);
    RUN_TEST(test_command_C_set_all_services);
    RUN_TEST(test_command_C_clear_all_services);

    // Command 'H' Tests
    RUN_TEST(test_command_H_zero_hours);
    RUN_TEST(test_command_H_positive_hours);
    RUN_TEST(test_command_H_large_hours_value);
    RUN_TEST(test_command_H_very_large_hours_value);
    RUN_TEST(test_command_H_single_second);
    RUN_TEST(test_command_H_negative_hours_rejected);
    RUN_TEST(test_command_H_empty_string);
    RUN_TEST(test_command_H_non_numeric_string);

    // Command 'T' Tests
    RUN_TEST(test_command_T_calibration_with_valid_rpm);
    RUN_TEST(test_command_T_calibration_halves_adjustment);
    RUN_TEST(test_command_T_calibration_doubles_adjustment);
    RUN_TEST(test_command_T_zero_rpm_rejected);
    RUN_TEST(test_command_T_negative_rpm_rejected);
    RUN_TEST(test_command_T_empty_string);
    RUN_TEST(test_command_T_non_numeric_string);

    // Command 't' Tests
    RUN_TEST(test_command_lowercase_t_adjustment);
    RUN_TEST(test_command_lowercase_t_adjustment_100);
    RUN_TEST(test_command_lowercase_t_adjustment_500);
    RUN_TEST(test_command_lowercase_t_zero_rejected);
    RUN_TEST(test_command_lowercase_t_negative_rejected);
    RUN_TEST(test_command_lowercase_t_empty_string);

    // Unknown Commands Tests
    RUN_TEST(test_command_unknown_command);
    RUN_TEST(test_command_unknown_command_lowercase_x);
    RUN_TEST(test_command_unknown_command_numeric);

    // Integration Tests
    RUN_TEST(test_command_sequence_name_then_services);
    RUN_TEST(test_command_sequence_services_then_hours);
    RUN_TEST(test_command_sequence_all_command_types);
    RUN_TEST(test_command_repeated_same_command);

    // Edge Cases and Boundary Tests
    RUN_TEST(test_command_with_very_long_device_name);
    RUN_TEST(test_command_H_with_string_containing_number);
    RUN_TEST(test_command_T_with_high_rpm_value);
    RUN_TEST(test_command_lowercase_t_with_fractional_result);

    return UNITY_END();
}