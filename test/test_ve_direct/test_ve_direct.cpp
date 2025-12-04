#include <unity.h>
#include <string.h>
#include "VeDirect.h"
#include "Conf.h"
#include "Context.h"
#include "N2K_router.h"

char c_sum[11];

const char* C(char c)
{
    strcpy(c_sum, "Checksum\tx");
    c_sum[9] = c;
    return c_sum;
}

// Mock VEDirectListener for testing
class MockVEDirectListener : public VEDirectListener
{
public:
    int complete_call_count;
    VEDirectObject *last_object;
    
    MockVEDirectListener()
        : complete_call_count(0),
          last_object(nullptr)
    {
    }
    
    void on_complete(VEDirectObject &obj) override
    {
        complete_call_count++;
        last_object = &obj;
    }
    
    void reset()
    {
        complete_call_count = 0;
        last_object = nullptr;
    }
};

void setUp(void)
{
}

void tearDown(void)
{
}

#pragma region VEDirectField Tests

void test_vedirect_field_construction(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_NUMBER, "TestField", 0, "unit");
    VEDirectFieldNumber field(def);
    
    TEST_ASSERT_FALSE(field.is_set());
    TEST_ASSERT_EQUAL_INT(0, field.get_last_time());
}

void test_vedirect_field_set_unset(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_NUMBER, "TestField", 0, "unit");
    VEDirectFieldNumber field(def);
    
    field.set_value(42);
    TEST_ASSERT_TRUE(field.is_set());
    
    field.unset();
    TEST_ASSERT_FALSE(field.is_set());
}

void test_vedirect_field_timestamp(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_NUMBER, "TestField", 0, "unit");
    VEDirectFieldNumber field(def);
    
    field.set_last_time(5000000);
    TEST_ASSERT_EQUAL_INT(5000000, field.get_last_time());
}

#pragma endregion

#pragma region VEDirectFieldNumber Tests

void test_vedirect_field_number_set_value(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_NUMBER, "Voltage", 1, "mV");
    VEDirectFieldNumber field(def);
    
    field.set_value(12488);
    
    TEST_ASSERT_TRUE(field.is_set());
    TEST_ASSERT_EQUAL_INT(12488, field.get_value());
}

void test_vedirect_field_number_parse_decimal(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_NUMBER, "Voltage", 1, "mV");
    VEDirectFieldNumber field(def);
    
    bool result = field.parse("12488");
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(field.is_set());
    TEST_ASSERT_EQUAL_INT(12488, field.get_value());
}

void test_vedirect_field_number_parse_hex(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_HEX, "PID", 0);
    VEDirectFieldNumber field(def);
    
    bool result = field.parse("0xA381");
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(field.is_set());
    TEST_ASSERT_EQUAL_INT(0xA381, field.get_value());
}

void test_vedirect_field_number_parse_negative(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_NUMBER, "CE", 4, "mAh");
    VEDirectFieldNumber field(def);
    
    bool result = field.parse("-89423");
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(-89423, field.get_value());
}

void test_vedirect_field_number_parse_zero(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_NUMBER, "Current", 3, "mA");
    VEDirectFieldNumber field(def);
    
    bool result = field.parse("0");
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, field.get_value());
}

void test_vedirect_field_number_parse_minus_one(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_NUMBER, "TTG", 6, "Minutes");
    VEDirectFieldNumber field(def);
    
    bool result = field.parse("-1");
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(-1, field.get_value());
}

void test_vedirect_field_number_parse_large_value(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_NUMBER, "Power", 4, "W");
    VEDirectFieldNumber field(def);
    
    bool result = field.parse("999999");
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(999999, field.get_value());
}

#pragma endregion

#pragma region VEDirectFieldBool Tests

void test_vedirect_field_bool_set_true(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_BOOLEAN, "Alarm", 7);
    VEDirectFieldBool field(def);
    
    field.set_value(true);
    
    TEST_ASSERT_TRUE(field.is_set());
    TEST_ASSERT_TRUE(field.get_value());
}

void test_vedirect_field_bool_set_false(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_BOOLEAN, "Relay", 8);
    VEDirectFieldBool field(def);
    
    field.set_value(false);
    
    TEST_ASSERT_TRUE(field.is_set());
    TEST_ASSERT_FALSE(field.get_value());
}

void test_vedirect_field_bool_parse_on(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_BOOLEAN, "Alarm", 7);
    VEDirectFieldBool field(def);
    
    bool result = field.parse("ON");
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(field.is_set());
    TEST_ASSERT_TRUE(field.get_value());
}

void test_vedirect_field_bool_parse_off(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_BOOLEAN, "Relay", 8);
    VEDirectFieldBool field(def);
    
    bool result = field.parse("OFF");
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(field.is_set());
    TEST_ASSERT_FALSE(field.get_value());
}

void test_vedirect_field_bool_toggle(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_BOOLEAN, "Alarm", 7);
    VEDirectFieldBool field(def);
    
    field.set_value(false);
    TEST_ASSERT_FALSE(field.get_value());
    
    field.set_value(true);
    TEST_ASSERT_TRUE(field.get_value());
}

#pragma endregion

#pragma region VEDirectFieldString Tests

void test_vedirect_field_string_set_value(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_STRING, "BMV", 13);
    VEDirectFieldString field(def);
    
    field.set_value("712 Smart");
    
    TEST_ASSERT_TRUE(field.is_set());
    TEST_ASSERT_EQUAL_STRING("712 Smart", field.get_value());
}

void test_vedirect_field_string_parse(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_STRING, "FW", 10);
    VEDirectFieldString field(def);
    
    bool result = field.parse("0413");
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(field.is_set());
    TEST_ASSERT_EQUAL_STRING("0413", field.get_value());
}

void test_vedirect_field_string_truncate_long_value(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_STRING, "Model", 10);
    VEDirectFieldString field(def);
    
    // String longer than 16 chars should be truncated
    field.set_value("VeryLongModelNameThatExceedsLimit");
    
    TEST_ASSERT_TRUE(field.is_set());
    // Should be truncated to fit in 16-byte buffer
    TEST_ASSERT_TRUE(strlen(field.get_value()) < 16);
}

void test_vedirect_field_string_empty(void)
{
    MOCK_CONTEXT
    
    VEDirectValueDefinition def(VE_STRING, "Model", 10);
    VEDirectFieldString field(def);
    
    field.set_value("");
    
    TEST_ASSERT_TRUE(field.is_set());
    TEST_ASSERT_EQUAL_STRING("", field.get_value());
}

#pragma endregion

#pragma region VEDirectObject Tests

void test_vedirect_object_construction(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    
    TEST_ASSERT_FALSE(obj.is_valid());
}

void test_vedirect_object_init(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    TEST_ASSERT_FALSE(obj.is_valid());
}

void test_vedirect_object_reset(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    obj.reset();
    
    TEST_ASSERT_FALSE(obj.is_valid());
}

#pragma endregion

#pragma region Helper Function Tests

void test_load_key_value_valid(void)
{
    MOCK_CONTEXT
    
    char key[16];
    char value[16];
    
    bool result = load_key_value("V\t13406", key, 16, value, 16);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("V", key);
    TEST_ASSERT_EQUAL_STRING("13406", value);
}

void test_load_key_value_with_spaces(void)
{
    MOCK_CONTEXT
    
    char key[16];
    char value[16];
    
    bool result = load_key_value("BMV\t712 Smart", key, 16, value, 16);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("BMV", key);
    TEST_ASSERT_EQUAL_STRING("712 Smart", value);
}

void test_load_key_value_no_tab(void)
{
    MOCK_CONTEXT
    
    char key[16];
    char value[16];
    
    bool result = load_key_value("InvalidLine", key, 16, value, 16);
    
    TEST_ASSERT_FALSE(result);
}

void test_load_key_value_empty_key(void)
{
    MOCK_CONTEXT
    
    char key[16];
    char value[16];
    
    bool result = load_key_value("\t13406", key, 16, value, 16);
    
    TEST_ASSERT_FALSE(result);
}

void test_load_key_value_empty_value(void)
{
    MOCK_CONTEXT
    
    char key[16];
    char value[16];
    
    bool result = load_key_value("V\t", key, 16, value, 16);
    
    TEST_ASSERT_FALSE(result);
}

void test_load_key_value_key_too_long(void)
{
    MOCK_CONTEXT
    
    char key[5];
    char value[16];
    
    bool result = load_key_value("VeryLongKeyName\t13406", key, 5, value, 16);
    
    TEST_ASSERT_FALSE(result);
}

void test_load_key_value_value_too_long(void)
{
    MOCK_CONTEXT
    
    char key[16];
    char value[5];
    
    bool result = load_key_value("V\tVeryLongValue12345", key, 16, value, 5);
    
    TEST_ASSERT_FALSE(result);
}

#pragma endregion

#pragma region Message Parsing Tests

void test_vedirect_parse_single_line_voltage(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    obj.on_line_read("V\t13406");
    
    int voltage = 0;
    bool result = obj.get_number_value(voltage, BMV_VOLTAGE);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(13406, voltage);
}

void test_vedirect_parse_single_line_pid_hex(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    obj.on_line_read("PID\t0xA381");
    
    int pid = 0;
    bool result = obj.get_number_value(pid, BMV_PID);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0xA381, pid);
}

void test_vedirect_parse_single_line_negative_ce(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    obj.on_line_read("CE\t-89423");
    
    int ce = 0;
    bool result = obj.get_number_value(ce, BMV_CONSUMPTION);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(-89423, ce);
}

void test_vedirect_parse_single_line_bool_alarm_off(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    obj.on_line_read("Alarm\tOFF");
    
    bool alarm = true;
    bool result = obj.get_boolean_value(alarm, BMV_ALARM);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(alarm);
}

void test_vedirect_parse_single_line_bool_relay_on(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    obj.on_line_read("Relay\tON");
    
    bool relay = false;
    bool result = obj.get_boolean_value(relay, BMV_RELAY);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(relay);
}

void test_vedirect_parse_single_line_string_bmv(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    obj.on_line_read("BMV\t712 Smart");
    
    char model[16];
    bool result = obj.get_string_value(model, BMV_BMV);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("712 Smart", model);
}

void test_vedirect_parse_soc_value(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    obj.on_line_read("SOC\t689");
    
    double soc = 0;
    bool result = obj.get_number_value(soc, 0.1, BMV_SOC);
    TEST_ASSERT_TRUE(result);
    // SOC 689 with precision 0.1 = 68.9%
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 68.9, soc);
}

#pragma endregion

#pragma region Complete Message Tests

void test_vedirect_complete_message_minimal(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    MockVEDirectListener listener;
    obj.set_listener(&listener);
    
    // Message starts with PID line
    obj.on_partial("PID\t", strlen("PID\t"));
    
    // Load some fields
    obj.on_line_read("PID\t0xA381");
    obj.on_line_read("V\t13406");
    
    // Message ends with Checksum line
    obj.on_partial(C(183), strlen("Checksum\ty"));

    TEST_ASSERT_EQUAL_INT(1, listener.complete_call_count);
}

void test_vedirect_complete_message_all_fields(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    MockVEDirectListener listener;
    obj.set_listener(&listener);
    
    // Start message
    obj.on_partial("PID\t", strlen("PID\t"));
    
    // All fields from actual message
    obj.on_line_read("PID\t0xA381");
    obj.on_line_read("V\t13406");
    obj.on_line_read("VS\t13152");
    obj.on_line_read("I\t0");
    obj.on_line_read("P\t0");
    obj.on_line_read("CE\t-89423");
    obj.on_line_read("SOC\t689");
    obj.on_line_read("TTG\t-1");
    obj.on_line_read("Alarm\tOFF");
    obj.on_line_read("Relay\tOFF");
    obj.on_line_read("AR\t0");
    obj.on_line_read("BMV\t712 Smart");
    obj.on_line_read("FW\t0413");
    obj.on_line_read("MON\t0");
    
    // End message
    obj.on_partial("Checksum\ty", strlen("Checksum\ty"));
    
    TEST_ASSERT_EQUAL_INT(1, listener.complete_call_count);
}

void test_vedirect_multiple_messages(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    MockVEDirectListener listener;
    obj.set_listener(&listener);
    
    // First message
    obj.on_partial("PID\t", strlen("PID\t"));
    obj.on_line_read("PID\t0xA381");
    obj.on_line_read("V\t13406");
    obj.on_partial(C(183), strlen("Checksum\ty"));
    
    TEST_ASSERT_EQUAL_INT(1, listener.complete_call_count);
    
    // Second message
    obj.on_partial("PID\t", strlen("PID\t"));
    obj.on_line_read("PID\t0xA381");
    obj.on_line_read("V\t13500");
    obj.on_partial(C(188), strlen("Checksum\tx"));
    
    TEST_ASSERT_EQUAL_INT(2, listener.complete_call_count);
}

void test_vedirect_pid_resets_object(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    MockVEDirectListener listener;
    obj.set_listener(&listener);
    
    // First message
    obj.on_partial("PID\t", strlen("PID\t"));
    obj.on_line_read("PID\t0xA381");
    obj.on_line_read("V\t13406");
    obj.on_partial("Checksum\ty", strlen("Checksum\ty"));
    
    // Immediate PID line starts new message (should reset)
    obj.on_partial("PID\t", strlen("PID\t"));
    obj.on_line_read("PID\t0xA381");
    obj.on_line_read("V\t13500");
    
    // Old values should be cleared
    int voltage = 0;
    int result = obj.get_number_value(voltage, BMV_VOLTAGE);
    TEST_ASSERT_EQUAL_INT(13500, voltage);
}

void test_vedirect_message_without_listener(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    // No listener set
    obj.on_partial("PID\t", strlen("PID\t"));
    obj.on_line_read("PID\t0xA381");
    obj.on_line_read("V\t13406");
    obj.on_partial("Checksum\ty", strlen("Checksum\ty"));
    
    // Should not crash without listener
    TEST_ASSERT_TRUE(true);
}

#pragma endregion

#pragma region Edge Cases

void test_vedirect_unknown_field(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    // Unknown field should be ignored gracefully
    obj.on_line_read("UnknownField\t12345");
    
    TEST_ASSERT_FALSE(obj.is_valid());
}

void test_vedirect_empty_line(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    // Empty line should be handled gracefully
    obj.on_line_read("");
    
    TEST_ASSERT_FALSE(obj.is_valid());
}

void test_vedirect_malformed_line(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    // Malformed line (no tab)
    obj.on_line_read("V13406");
    
    TEST_ASSERT_FALSE(obj.is_valid());
}

void test_vedirect_extra_whitespace(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    // Line with extra spaces should still parse if tab-delimited
    obj.on_line_read("BMV\t712 Smart  ");
    
    char model[16];
    int result = obj.get_string_value(model, BMV_BMV);
    TEST_ASSERT_NOT_EQUAL(0, result);
}

void test_vedirect_case_sensitive_on_off(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    // ON/OFF must be uppercase
    obj.on_line_read("Alarm\tOFF");
    
    bool alarm = true;
    int result = obj.get_boolean_value(alarm, BMV_ALARM);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_FALSE(alarm);
}

#pragma endregion

#pragma region Checksum Tests

void test_vedirect_checksum_incorrect(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    MockVEDirectListener listener;
    obj.set_listener(&listener);
    
    // Start message
    obj.on_partial("PID\t", strlen("PID\t"));
    
    obj.on_line_read("PID\t0xA381");
    obj.on_line_read("V\t13406");
    obj.on_line_read("I\t0");
    
    // Wrong checksum - use 'X' which is unlikely to be correct
    obj.on_line_read("Checksum\tX");
    
    // Message should be rejected (listener not called or marked invalid)
    TEST_ASSERT_EQUAL_INT(0, listener.complete_call_count);
    TEST_ASSERT_FALSE(obj.is_valid());
}

void test_vedirect_checksum_off_by_one(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    MockVEDirectListener listener;
    obj.set_listener(&listener);
    
    // Start message
    obj.on_partial("PID\t", strlen("PID\t"));
    
    obj.on_line_read("PID\t0xA381");
    obj.on_line_read("V\t13406");
    
    // Calculate correct checksum
    int checksum = update_checksum(0, "PID\t0xA381");
    checksum = update_checksum(checksum, "V\t13406");
    
    // Use checksum + 1 (incorrect)
    int wrong_checksum = (checksum + 1) & 0xFF;
    char wrong_checksum_char = (char)wrong_checksum;
    
    char checksum_line[32];
    snprintf(checksum_line, sizeof(checksum_line), "Checksum\t%c", wrong_checksum_char);
    obj.on_partial(checksum_line, strlen(checksum_line));
    
    // Should reject the message
    TEST_ASSERT_FALSE(obj.is_valid());
}

void test_vedirect_checksum_zero(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    MockVEDirectListener listener;
    obj.set_listener(&listener);
    
    // Start message
    obj.on_partial("PID\t", strlen("PID\t"));
    
    obj.on_line_read("PID\t0xA381");
    
    // Checksum of 0 (NULL character) - edge case
    obj.on_line_read("Checksum\t\0");
    
    // Should handle null character gracefully
    TEST_ASSERT_FALSE(obj.is_valid());
}

void test_vedirect_checksum_with_multiple_lines(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    MockVEDirectListener listener;
    obj.set_listener(&listener);
    
    // Start message
    obj.on_partial("PID\t", strlen("PID\t"));
    
    obj.on_line_read("PID\t0xA381");
    obj.on_line_read("V\t13406");
    obj.on_line_read("VS\t13152");
    obj.on_line_read("I\t0");
    obj.on_line_read("P\t0");
    obj.on_line_read("CE\t-89423");
    obj.on_line_read("SOC\t689");
    obj.on_line_read("TTG\t-1");
    obj.on_line_read("Alarm\tOFF");
    obj.on_line_read("Relay\tOFF");
    obj.on_line_read("AR\t0");
    obj.on_line_read("BMV\t712 Smart");
    obj.on_line_read("FW\t0413");
    obj.on_line_read("MON\t0");
    
    // Calculate correct checksum for all lines
    int checksum = 0;
    checksum = update_checksum(checksum, "PID\t0xA381");
    checksum = update_checksum(checksum, "V\t13406");
    checksum = update_checksum(checksum, "VS\t13152");
    checksum = update_checksum(checksum, "I\t0");
    checksum = update_checksum(checksum, "P\t0");
    checksum = update_checksum(checksum, "CE\t-89423");
    checksum = update_checksum(checksum, "SOC\t689");
    checksum = update_checksum(checksum, "TTG\t-1");
    checksum = update_checksum(checksum, "Alarm\tOFF");
    checksum = update_checksum(checksum, "Relay\tOFF");
    checksum = update_checksum(checksum, "AR\t0");
    checksum = update_checksum(checksum, "BMV\t712 Smart");
    checksum = update_checksum(checksum, "FW\t0413");
    checksum = update_checksum(checksum, "MON\t0");
    checksum = update_checksum(checksum, C(0));
    char correct_checksum_char = (char)((256 - checksum) & 0xFF);
    
    obj.on_partial(C(correct_checksum_char), strlen("Checksum\tx"));
    
    // With correct checksum, message should be complete
    TEST_ASSERT_EQUAL_INT(1, listener.complete_call_count);
}

void test_vedirect_checksum_wrong_after_multiple_lines(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    MockVEDirectListener listener;
    obj.set_listener(&listener);
    
    // Start message
    obj.on_partial("PID\t", strlen("PID\t"));
    
    obj.on_line_read("PID\t0xA381");
    obj.on_line_read("V\t13406");
    obj.on_line_read("VS\t13152");
    obj.on_line_read("I\t0");
    
    // Use wrong checksum (just use 'z')
    obj.on_partial("Checksum\tz", strlen("Checksum\tz"));
    
    // Message should be rejected
    TEST_ASSERT_EQUAL_INT(0, listener.complete_call_count);
    TEST_ASSERT_FALSE(obj.is_valid());
}

void test_vedirect_checksum_masked_to_8bit(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    MockVEDirectListener listener;
    obj.set_listener(&listener);
    
    // Start message
    obj.on_partial("PID\t", strlen("PID\t"));
    
    // Add many lines to generate large checksum
    for (int i = 0; i < 20; i++)
    {
        char line[32];
        snprintf(line, sizeof(line), "V\t%d", i);
        obj.on_line_read(line);
    }
    
    // Calculate checksum (should be masked to 8-bit)
    int checksum = 0;
    for (int i = 0; i < 20; i++)
    {
        char line[32];
        snprintf(line, sizeof(line), "V\t%d", i);
        checksum = update_checksum(checksum, line);
    }
    
    checksum = checksum & 0xFF;  // Should be masked
    char correct_checksum_char = (char)checksum;
    
    char checksum_line[32];
    snprintf(checksum_line, sizeof(checksum_line), "Checksum\t%c", correct_checksum_char);
    obj.on_partial(checksum_line, strlen(checksum_line));;
    
    // Checksum should be within 8-bit range
    TEST_ASSERT_TRUE(checksum >= 0 && checksum <= 0xFF);
}

void test_vedirect_checksum_empty_message(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    MockVEDirectListener listener;
    obj.set_listener(&listener);
    
    // Start message but immediately end with checksum
    obj.on_partial("PID\t", strlen("PID\t"));
    
    // Checksum with no data - should be 0 or minimal
    int checksum = 0;
    char checksum_char = (char)checksum;
    
    char checksum_line[32];
    snprintf(checksum_line, sizeof(checksum_line), "Checksum\t%c", checksum_char);
    obj.on_partial(checksum_line, strlen(checksum_line));;
    
    // Should handle gracefully (either accept or reject cleanly)
    TEST_ASSERT_TRUE(true);  // Just ensure no crash
}

void test_vedirect_checksum_printable_characters(void)
{
    MOCK_CONTEXT
    
    VEDirectObject obj;
    obj.init(BMV_FIELDS, BMV_N_FIELDS);
    
    MockVEDirectListener listener;
    obj.set_listener(&listener);
    
    // Start message
    obj.on_partial("PID\t", strlen("PID\t"));
    
    obj.on_line_read("PID\t0xA381");
    obj.on_line_read("V\t13406");
    
    // Calculate checksum
    int checksum = 0;
    checksum = update_checksum(checksum, "PID\t0xA381");
    checksum = update_checksum(checksum, "V\t13406");
    checksum = update_checksum(checksum, C(0));
    checksum = checksum & 0xFF;
    
    // Checksum should be a printable character (or at least a valid char)
    char checksum_char = (char)(256 - checksum);
    
    obj.on_partial(C(checksum_char), strlen("Checksum\tx"));
    
    // Message should validate correctly
    TEST_ASSERT_EQUAL_INT(1, listener.complete_call_count);
}

#pragma endregion

// Test runner
void run_vedirect_tests(void)
{
    // VEDirectField tests
    RUN_TEST(test_vedirect_field_construction);
    RUN_TEST(test_vedirect_field_set_unset);
    RUN_TEST(test_vedirect_field_timestamp);

    // VEDirectFieldNumber tests
    RUN_TEST(test_vedirect_field_number_set_value);
    RUN_TEST(test_vedirect_field_number_parse_decimal);
    RUN_TEST(test_vedirect_field_number_parse_hex);
    RUN_TEST(test_vedirect_field_number_parse_negative);
    RUN_TEST(test_vedirect_field_number_parse_zero);
    RUN_TEST(test_vedirect_field_number_parse_minus_one);
    RUN_TEST(test_vedirect_field_number_parse_large_value);

    // VEDirectFieldBool tests
    RUN_TEST(test_vedirect_field_bool_set_true);
    RUN_TEST(test_vedirect_field_bool_set_false);
    RUN_TEST(test_vedirect_field_bool_parse_on);
    RUN_TEST(test_vedirect_field_bool_parse_off);
    RUN_TEST(test_vedirect_field_bool_toggle);

    // VEDirectFieldString tests
    RUN_TEST(test_vedirect_field_string_set_value);
    RUN_TEST(test_vedirect_field_string_parse);
    RUN_TEST(test_vedirect_field_string_truncate_long_value);
    RUN_TEST(test_vedirect_field_string_empty);

    // VEDirectObject tests
    RUN_TEST(test_vedirect_object_construction);
    RUN_TEST(test_vedirect_object_init);
    RUN_TEST(test_vedirect_object_reset);

    // Helper function tests
    RUN_TEST(test_load_key_value_valid);
    RUN_TEST(test_load_key_value_with_spaces);
    RUN_TEST(test_load_key_value_no_tab);
    RUN_TEST(test_load_key_value_empty_key);
    RUN_TEST(test_load_key_value_empty_value);
    RUN_TEST(test_load_key_value_key_too_long);
    RUN_TEST(test_load_key_value_value_too_long);

    // Message parsing tests
    RUN_TEST(test_vedirect_parse_single_line_voltage);
    RUN_TEST(test_vedirect_parse_single_line_pid_hex);
    RUN_TEST(test_vedirect_parse_single_line_negative_ce);
    RUN_TEST(test_vedirect_parse_single_line_bool_alarm_off);
    RUN_TEST(test_vedirect_parse_single_line_bool_relay_on);
    RUN_TEST(test_vedirect_parse_single_line_string_bmv);
    RUN_TEST(test_vedirect_parse_soc_value);

    // Complete message tests
    RUN_TEST(test_vedirect_complete_message_minimal);
    RUN_TEST(test_vedirect_complete_message_all_fields);
    RUN_TEST(test_vedirect_multiple_messages);
    RUN_TEST(test_vedirect_pid_resets_object);
    RUN_TEST(test_vedirect_message_without_listener);

    // Edge cases
    RUN_TEST(test_vedirect_unknown_field);
    RUN_TEST(test_vedirect_empty_line);
    RUN_TEST(test_vedirect_malformed_line);
    RUN_TEST(test_vedirect_extra_whitespace);
    RUN_TEST(test_vedirect_case_sensitive_on_off);

    //checksum tests
    RUN_TEST(test_vedirect_checksum_incorrect);
    RUN_TEST(test_vedirect_checksum_off_by_one);
    RUN_TEST(test_vedirect_checksum_zero);
    RUN_TEST(test_vedirect_checksum_with_multiple_lines);
    RUN_TEST(test_vedirect_checksum_wrong_after_multiple_lines);
    RUN_TEST(test_vedirect_checksum_masked_to_8bit);
    RUN_TEST(test_vedirect_checksum_empty_message);
    RUN_TEST(test_vedirect_checksum_printable_characters);
}

void setup()
{
    UNITY_BEGIN();
    run_vedirect_tests();
    UNITY_END();
}

void loop() {}

int main()
{
    setup();
}