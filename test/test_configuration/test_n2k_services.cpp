#include <unity.h>
#include "Conf.h"

#ifndef TEST_N2KSERVICES
#define TEST_N2KSERVICES

// Test fixture setup
void setUpN2k_services(void)
{
    // Reset before each test
}

void tearDownN2k_services(void)
{
    // Cleanup after each test
}

#pragma region Constructor Tests

void test_n2k_services_constructor_default_values(void)
{
    N2KServices svc;

    // Verify defaults match the macro definitions
    TEST_ASSERT_EQUAL_INT(DEFAULT_USE_GPS, svc.is_use_gps());
    TEST_ASSERT_EQUAL_INT(DEFAULT_USE_BME, svc.is_use_bme());
    TEST_ASSERT_EQUAL_INT(DEFAULT_USE_DHT, svc.is_use_dht());
    TEST_ASSERT_EQUAL_INT(DEFAULT_USE_TIME, svc.is_send_time());
    TEST_ASSERT_EQUAL_INT(DEFAULT_SOG_2_STW, svc.is_sog_2_stw());
    TEST_ASSERT_EQUAL_INT(DEFAULT_USE_TACHO, svc.is_use_tacho());
    TEST_ASSERT_EQUAL_INT(DEFAULT_USE_VE_DIRECT, svc.is_use_vedirect());
}

void test_n2k_services_size_returns_correct_value(void)
{
    N2KServices svc;
    TEST_ASSERT_EQUAL_INT(7, svc.size()); // MAX_CONF = 7
}

#pragma endregion

#pragma region Setter/Getter Tests

void test_n2k_services_set_use_gps_true(void)
{
    N2KServices svc;
    svc.set_use_gps(true);
    TEST_ASSERT_TRUE(svc.is_use_gps());
}

void test_n2k_services_set_use_gps_false(void)
{
    N2KServices svc;
    svc.set_use_gps(true);
    svc.set_use_gps(false);
    TEST_ASSERT_FALSE(svc.is_use_gps());
}

void test_n2k_services_set_use_bme_true(void)
{
    N2KServices svc;
    svc.set_use_bme(true);
    TEST_ASSERT_TRUE(svc.is_use_bme());
}

void test_n2k_services_set_use_bme_false(void)
{
    N2KServices svc;
    svc.set_use_bme(true);
    svc.set_use_bme(false);
    TEST_ASSERT_FALSE(svc.is_use_bme());
}

void test_n2k_services_set_use_dht_true(void)
{
    N2KServices svc;
    svc.set_use_dht(true);
    TEST_ASSERT_TRUE(svc.is_use_dht());
}

void test_n2k_services_set_use_dht_false(void)
{
    N2KServices svc;
    svc.set_use_dht(true);
    svc.set_use_dht(false);
    TEST_ASSERT_FALSE(svc.is_use_dht());
}

void test_n2k_services_set_send_time_true(void)
{
    N2KServices svc;
    svc.set_send_time(true);
    TEST_ASSERT_TRUE(svc.is_send_time());
}

void test_n2k_services_set_send_time_false(void)
{
    N2KServices svc;
    svc.set_send_time(true);
    svc.set_send_time(false);
    TEST_ASSERT_FALSE(svc.is_send_time());
}

void test_n2k_services_set_use_tacho_true(void)
{
    N2KServices svc;
    svc.set_use_tacho(true);
    TEST_ASSERT_TRUE(svc.is_use_tacho());
}

void test_n2k_services_set_use_tacho_false(void)
{
    N2KServices svc;
    svc.set_use_tacho(true);
    svc.set_use_tacho(false);
    TEST_ASSERT_FALSE(svc.is_use_tacho());
}

void test_n2k_services_set_sog_2_stw_true(void)
{
    N2KServices svc;
    svc.set_sog_2_stw(true);
    TEST_ASSERT_TRUE(svc.is_sog_2_stw());
}

void test_n2k_services_set_sog_2_stw_false(void)
{
    N2KServices svc;
    svc.set_sog_2_stw(true);
    svc.set_sog_2_stw(false);
    TEST_ASSERT_FALSE(svc.is_sog_2_stw());
}

void test_n2k_services_set_use_vedirect_true(void)
{
    N2KServices svc;
    svc.set_use_vedirect(true);
    TEST_ASSERT_TRUE(svc.is_use_vedirect());
}

void test_n2k_services_set_use_vedirect_false(void)
{
    N2KServices svc;
    svc.set_use_vedirect(true);
    svc.set_use_vedirect(false);
    TEST_ASSERT_FALSE(svc.is_use_vedirect());
}

#pragma endregion

#pragma region Multiple Services Tests

void test_n2k_services_multiple_services_independent(void)
{
    N2KServices svc;
    svc.set_use_gps(true);
    svc.set_use_bme(false);
    svc.set_use_dht(true);

    TEST_ASSERT_TRUE(svc.is_use_gps());
    TEST_ASSERT_FALSE(svc.is_use_bme());
    TEST_ASSERT_TRUE(svc.is_use_dht());
}

void test_n2k_services_toggle_multiple_times(void)
{
    N2KServices svc;

    // Toggle GPS multiple times
    svc.set_use_gps(true);
    TEST_ASSERT_TRUE(svc.is_use_gps());
    svc.set_use_gps(false);
    TEST_ASSERT_FALSE(svc.is_use_gps());
    svc.set_use_gps(true);
    TEST_ASSERT_TRUE(svc.is_use_gps());
}

void test_n2k_services_all_services_enabled(void)
{
    N2KServices svc;
    svc.set_use_gps(true);
    svc.set_use_bme(true);
    svc.set_use_dht(true);
    svc.set_send_time(true);
    svc.set_use_tacho(true);
    svc.set_sog_2_stw(true);
    svc.set_use_vedirect(true);

    TEST_ASSERT_TRUE(svc.is_use_gps());
    TEST_ASSERT_TRUE(svc.is_use_bme());
    TEST_ASSERT_TRUE(svc.is_use_dht());
    TEST_ASSERT_TRUE(svc.is_send_time());
    TEST_ASSERT_TRUE(svc.is_use_tacho());
    TEST_ASSERT_TRUE(svc.is_sog_2_stw());
    TEST_ASSERT_TRUE(svc.is_use_vedirect());
}

void test_n2k_services_all_services_disabled(void)
{
    N2KServices svc;
    svc.set_use_gps(false);
    svc.set_use_bme(false);
    svc.set_use_dht(false);
    svc.set_send_time(false);
    svc.set_use_tacho(false);
    svc.set_sog_2_stw(false);
    svc.set_use_vedirect(false);

    TEST_ASSERT_FALSE(svc.is_use_gps());
    TEST_ASSERT_FALSE(svc.is_use_bme());
    TEST_ASSERT_FALSE(svc.is_use_dht());
    TEST_ASSERT_FALSE(svc.is_send_time());
    TEST_ASSERT_FALSE(svc.is_use_tacho());
    TEST_ASSERT_FALSE(svc.is_sog_2_stw());
    TEST_ASSERT_FALSE(svc.is_use_vedirect());
}

#pragma endregion

#pragma region Serialization Tests

void test_n2k_services_serialize_empty(void)
{
    N2KServices svc;
    svc.set_use_gps(false);
    svc.set_use_bme(false);
    svc.set_use_dht(false);
    svc.set_send_time(false);
    svc.set_use_tacho(false);
    svc.set_sog_2_stw(false);
    svc.set_use_vedirect(false);

    uint8_t serialized = svc.serialize();
    TEST_ASSERT_EQUAL_HEX8(0x00, serialized);
}

void test_n2k_services_serialize_all_set(void)
{
    N2KServices svc;
    svc.set_use_gps(true);
    svc.set_use_bme(true);
    svc.set_use_dht(true);
    svc.set_send_time(true);
    svc.set_use_tacho(true);
    svc.set_sog_2_stw(true);
    svc.set_use_vedirect(true);

    uint8_t serialized = svc.serialize();
    TEST_ASSERT_EQUAL_HEX8(0x7F, serialized); // 0111 1111 = 7 bits set
}

void test_n2k_services_serialize_mixed(void)
{
    N2KServices svc;
    svc.set_use_gps(true); // bit 0
    svc.set_use_dht(true); // bit 1
    svc.set_use_bme(false);
    svc.set_send_time(false);
    svc.set_use_tacho(true); // bit 4
    svc.set_sog_2_stw(false);
    svc.set_use_vedirect(true); // bit 6

    uint8_t serialized = svc.serialize();
    // bits: 6543210 = 1010011 = 0x53
    TEST_ASSERT_EQUAL_HEX8(0x53, serialized);
}

void test_n2k_services_deserialize_empty(void)
{
    N2KServices svc;
    svc.deserialize(0x00);

    TEST_ASSERT_FALSE(svc.is_use_gps());
    TEST_ASSERT_FALSE(svc.is_use_bme());
    TEST_ASSERT_FALSE(svc.is_use_dht());
    TEST_ASSERT_FALSE(svc.is_send_time());
    TEST_ASSERT_FALSE(svc.is_use_tacho());
    TEST_ASSERT_FALSE(svc.is_sog_2_stw());
    TEST_ASSERT_FALSE(svc.is_use_vedirect());
}

void test_n2k_services_deserialize_all_set(void)
{
    N2KServices svc;
    svc.deserialize(0x7F);

    TEST_ASSERT_TRUE(svc.is_use_gps());
    TEST_ASSERT_TRUE(svc.is_use_bme());
    TEST_ASSERT_TRUE(svc.is_use_dht());
    TEST_ASSERT_TRUE(svc.is_send_time());
    TEST_ASSERT_TRUE(svc.is_use_tacho());
    TEST_ASSERT_TRUE(svc.is_sog_2_stw());
    TEST_ASSERT_TRUE(svc.is_use_vedirect());
}

void test_n2k_services_deserialize_mixed(void)
{
    N2KServices svc;
    svc.deserialize(0x53); // 0101 0011

    TEST_ASSERT_TRUE(svc.is_use_gps());      // bit 0
    TEST_ASSERT_TRUE(svc.is_use_dht());      // bit 1
    TEST_ASSERT_FALSE(svc.is_use_bme());     // bit 2
    TEST_ASSERT_FALSE(svc.is_send_time());   // bit 3
    TEST_ASSERT_TRUE(svc.is_use_tacho());    // bit 4
    TEST_ASSERT_FALSE(svc.is_sog_2_stw());   // bit 5
    TEST_ASSERT_TRUE(svc.is_use_vedirect()); // bit 6
}

void test_n2k_services_serialize_deserialize_roundtrip(void)
{
    N2KServices svc1;
    svc1.set_use_gps(true);
    svc1.set_use_bme(false);
    svc1.set_use_dht(true);
    svc1.set_send_time(true);
    svc1.set_use_tacho(false);
    svc1.set_sog_2_stw(true);
    svc1.set_use_vedirect(false);

    uint8_t serialized = svc1.serialize();

    N2KServices svc2;
    svc2.deserialize(serialized);

    TEST_ASSERT_EQUAL_INT(svc1.is_use_gps(), svc2.is_use_gps());
    TEST_ASSERT_EQUAL_INT(svc1.is_use_bme(), svc2.is_use_bme());
    TEST_ASSERT_EQUAL_INT(svc1.is_use_dht(), svc2.is_use_dht());
    TEST_ASSERT_EQUAL_INT(svc1.is_send_time(), svc2.is_send_time());
    TEST_ASSERT_EQUAL_INT(svc1.is_use_tacho(), svc2.is_use_tacho());
    TEST_ASSERT_EQUAL_INT(svc1.is_sog_2_stw(), svc2.is_sog_2_stw());
    TEST_ASSERT_EQUAL_INT(svc1.is_use_vedirect(), svc2.is_use_vedirect());
}

#pragma endregion

#pragma region String Conversion Tests

void test_n2k_services_to_string_all_enabled(void)
{
    N2KServices svc;
    svc.set_use_gps(true);
    svc.set_use_bme(true);
    svc.set_use_dht(true);
    svc.set_send_time(true);
    svc.set_use_tacho(true);
    svc.set_sog_2_stw(true);
    svc.set_use_vedirect(true);

    char buffer[16] = {0};
    bool result = svc.to_string(buffer, sizeof(buffer));

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("1111111", buffer);
}

void test_n2k_services_to_string_all_disabled(void)
{
    N2KServices svc;
    svc.set_use_gps(false);
    svc.set_use_bme(false);
    svc.set_use_dht(false);
    svc.set_send_time(false);
    svc.set_use_tacho(false);
    svc.set_sog_2_stw(false);
    svc.set_use_vedirect(false);

    char buffer[16] = {0};
    bool result = svc.to_string(buffer, sizeof(buffer));

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("0000000", buffer);
}

void test_n2k_services_to_string_mixed(void)
{
    N2KServices svc;
    svc.set_use_gps(true);
    svc.set_use_dht(true);
    svc.set_use_bme(false);
    svc.set_send_time(false);
    svc.set_use_tacho(true);
    svc.set_sog_2_stw(false);
    svc.set_use_vedirect(true);

    char buffer[16] = {0};
    bool result = svc.to_string(buffer, sizeof(buffer));

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("1100101", buffer);
}

void test_n2k_services_to_string_buffer_too_small(void)
{
    N2KServices svc;
    svc.set_use_gps(true);

    char buffer[5] = {0}; // Too small for 7 chars + null terminator
    bool result = svc.to_string(buffer, sizeof(buffer));

    TEST_ASSERT_FALSE(result);
}

void test_n2k_services_to_string_null_terminated(void)
{
    N2KServices svc;
    svc.set_use_gps(true);
    svc.set_use_bme(true);

    char buffer[16] = { '\xFF', '\xFF', '\xFF', '\xFF'}; // Pre-fill with garbage
    bool result = svc.to_string(buffer, sizeof(buffer));

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_CHAR('\0', buffer[7]); // Should be null terminated at position 7
}

void test_n2k_services_from_string_all_enabled(void)
{
    N2KServices svc;
    const char *input = "1111111";
    bool result = svc.from_string(input);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(svc.is_use_gps());
    TEST_ASSERT_TRUE(svc.is_use_bme());
    TEST_ASSERT_TRUE(svc.is_use_dht());
    TEST_ASSERT_TRUE(svc.is_send_time());
    TEST_ASSERT_TRUE(svc.is_use_tacho());
    TEST_ASSERT_TRUE(svc.is_sog_2_stw());
    TEST_ASSERT_TRUE(svc.is_use_vedirect());
}

void test_n2k_services_from_string_all_disabled(void)
{
    N2KServices svc;
    const char *input = "0000000";
    bool result = svc.from_string(input);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(svc.is_use_gps());
    TEST_ASSERT_FALSE(svc.is_use_bme());
    TEST_ASSERT_FALSE(svc.is_use_dht());
    TEST_ASSERT_FALSE(svc.is_send_time());
    TEST_ASSERT_FALSE(svc.is_use_tacho());
    TEST_ASSERT_FALSE(svc.is_sog_2_stw());
    TEST_ASSERT_FALSE(svc.is_use_vedirect());
}

void test_n2k_services_from_string_mixed(void)
{
    N2KServices svc;
    const char *input = "1010101";
    bool result = svc.from_string(input);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(svc.is_use_gps());
    TEST_ASSERT_FALSE(svc.is_use_dht());
    TEST_ASSERT_TRUE(svc.is_use_bme());
    TEST_ASSERT_FALSE(svc.is_send_time());
    TEST_ASSERT_TRUE(svc.is_use_tacho());
    TEST_ASSERT_FALSE(svc.is_sog_2_stw());
    TEST_ASSERT_TRUE(svc.is_use_vedirect());
}

void test_n2k_services_from_string_non_binary_chars_treated_as_true(void)
{
    N2KServices svc;
    const char *input = "X010X01"; // Non-'0' chars treated as true
    bool result = svc.from_string(input);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(svc.is_use_gps()); // 'X' is non-zero, treated as true
    TEST_ASSERT_FALSE(svc.is_use_dht());
    TEST_ASSERT_TRUE(svc.is_use_bme()); // '1' is true
    TEST_ASSERT_FALSE(svc.is_send_time());
    TEST_ASSERT_TRUE(svc.is_use_tacho()); // 'X' is non-zero, treated as true
    TEST_ASSERT_FALSE(svc.is_sog_2_stw());
    TEST_ASSERT_TRUE(svc.is_use_vedirect()); // '1' is true
}

void test_n2k_services_to_from_string_roundtrip(void)
{
    N2KServices svc1;
    svc1.set_use_gps(true);
    svc1.set_use_bme(false);
    svc1.set_use_dht(true);
    svc1.set_send_time(true);
    svc1.set_use_tacho(false);
    svc1.set_sog_2_stw(true);
    svc1.set_use_vedirect(false);

    char buffer[16] = {0};
    svc1.to_string(buffer, sizeof(buffer));

    N2KServices svc2;
    svc2.from_string(buffer);

    TEST_ASSERT_EQUAL_INT(svc1.serialize(), svc2.serialize());
}

#pragma endregion

#pragma region Assignment Operator Tests

void test_n2k_services_assignment_operator(void)
{
    N2KServices svc1;
    svc1.set_use_gps(true);
    svc1.set_use_bme(false);
    svc1.set_use_dht(true);

    N2KServices svc2;
    svc2 = svc1;

    TEST_ASSERT_EQUAL_INT(svc1.is_use_gps(), svc2.is_use_gps());
    TEST_ASSERT_EQUAL_INT(svc1.is_use_bme(), svc2.is_use_bme());
    TEST_ASSERT_EQUAL_INT(svc1.is_use_dht(), svc2.is_use_dht());
}

void test_n2k_services_assignment_independence(void)
{
    N2KServices svc1;
    svc1.set_use_gps(true);

    N2KServices svc2;
    svc2 = svc1;

    svc2.set_use_gps(false);

    TEST_ASSERT_TRUE(svc1.is_use_gps());  // svc1 should be unchanged
    TEST_ASSERT_FALSE(svc2.is_use_gps()); // svc2 should reflect the change
}

#pragma endregion

// Test runner
void run_N2k_services_tests(void)
{
    RUN_TEST(test_n2k_services_constructor_default_values);
    RUN_TEST(test_n2k_services_size_returns_correct_value);

    RUN_TEST(test_n2k_services_set_use_gps_true);
    RUN_TEST(test_n2k_services_set_use_gps_false);
    RUN_TEST(test_n2k_services_set_use_bme_true);
    RUN_TEST(test_n2k_services_set_use_bme_false);
    RUN_TEST(test_n2k_services_set_use_dht_true);
    RUN_TEST(test_n2k_services_set_use_dht_false);
    RUN_TEST(test_n2k_services_set_send_time_true);
    RUN_TEST(test_n2k_services_set_send_time_false);
    RUN_TEST(test_n2k_services_set_use_tacho_true);
    RUN_TEST(test_n2k_services_set_use_tacho_false);
    RUN_TEST(test_n2k_services_set_sog_2_stw_true);
    RUN_TEST(test_n2k_services_set_sog_2_stw_false);
    RUN_TEST(test_n2k_services_set_use_vedirect_true);
    RUN_TEST(test_n2k_services_set_use_vedirect_false);

    RUN_TEST(test_n2k_services_multiple_services_independent);
    RUN_TEST(test_n2k_services_toggle_multiple_times);
    RUN_TEST(test_n2k_services_all_services_enabled);
    RUN_TEST(test_n2k_services_all_services_disabled);

    RUN_TEST(test_n2k_services_serialize_empty);
    RUN_TEST(test_n2k_services_serialize_all_set);
    RUN_TEST(test_n2k_services_serialize_mixed);
    RUN_TEST(test_n2k_services_deserialize_empty);
    RUN_TEST(test_n2k_services_deserialize_all_set);
    RUN_TEST(test_n2k_services_deserialize_mixed);
    RUN_TEST(test_n2k_services_serialize_deserialize_roundtrip);

    RUN_TEST(test_n2k_services_to_string_all_enabled);
    RUN_TEST(test_n2k_services_to_string_all_disabled);
    RUN_TEST(test_n2k_services_to_string_mixed);
    RUN_TEST(test_n2k_services_to_string_buffer_too_small);
    RUN_TEST(test_n2k_services_to_string_null_terminated);
    RUN_TEST(test_n2k_services_from_string_all_enabled);
    RUN_TEST(test_n2k_services_from_string_all_disabled);
    RUN_TEST(test_n2k_services_from_string_mixed);
    RUN_TEST(test_n2k_services_from_string_non_binary_chars_treated_as_true);
    RUN_TEST(test_n2k_services_to_from_string_roundtrip);

    RUN_TEST(test_n2k_services_assignment_operator);
    RUN_TEST(test_n2k_services_assignment_independence);
}

#endif
