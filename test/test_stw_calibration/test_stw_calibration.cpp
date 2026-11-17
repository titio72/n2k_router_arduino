#include <unity.h>
#include <math.h>
#include <string.h>
#include "StwCalibration.h"

void setUp(void) {}
void tearDown(void) {}

#define EPS 1e-9

void test_empty_table_is_identity(void)
{
    StwCalibration t;
    TEST_ASSERT_DOUBLE_WITHIN(EPS, 2.0, t.speed(2.0));
    TEST_ASSERT_DOUBLE_WITHIN(EPS, 13.7, t.speed(13.7));
    TEST_ASSERT_DOUBLE_WITHIN(EPS, 0.0, t.speed(0));
}

void test_single_point_is_line_through_origin(void)
{
    StwCalibration t;
    TEST_ASSERT_TRUE(t.upsert(4.8, 1.0));
    TEST_ASSERT_DOUBLE_WITHIN(EPS, 0.5, t.speed(2.4));
    TEST_ASSERT_DOUBLE_WITHIN(EPS, 2.0, t.speed(9.6));
}

void test_interpolates_between_points(void)
{
    StwCalibration t;
    TEST_ASSERT_TRUE(t.upsert(4.8, 1.0));
    TEST_ASSERT_TRUE(t.upsert(10.0, 2.5));
    TEST_ASSERT_DOUBLE_WITHIN(EPS, 1.75, t.speed(7.4));
    TEST_ASSERT_EQUAL_DOUBLE(1.0, t.speed(4.8));
    TEST_ASSERT_EQUAL_DOUBLE(2.5, t.speed(10.0));
}

void test_extends_last_slope_above_range(void)
{
    StwCalibration t;
    t.upsert(4.8, 1.0);
    t.upsert(10.0, 2.5);
    TEST_ASSERT_DOUBLE_WITHIN(EPS, 4.0, t.speed(15.2));
}

void test_below_first_point_interpolates_from_origin(void)
{
    StwCalibration t;
    t.upsert(4.8, 1.0);
    TEST_ASSERT_DOUBLE_WITHIN(EPS, 0.5, t.speed(2.4));
}

void test_nan_and_negative_speed(void)
{
    StwCalibration t;
    TEST_ASSERT_TRUE(isnan(t.speed(NAN)));
    t.upsert(4.8, 1.0);
    TEST_ASSERT_TRUE(isnan(t.speed(NAN)));
    TEST_ASSERT_EQUAL_DOUBLE(0.0, t.speed(-1.0));
}

void test_upsert_keeps_sorted(void)
{
    StwCalibration t;
    TEST_ASSERT_TRUE(t.upsert(10, 3));
    TEST_ASSERT_TRUE(t.upsert(4.8, 1));
    TEST_ASSERT_TRUE(t.upsert(7, 2));
    TEST_ASSERT_EQUAL_UINT8(3, t.size());
    TEST_ASSERT_EQUAL_UINT16(480, t.point(0).raw100);
    TEST_ASSERT_EQUAL_UINT16(700, t.point(1).raw100);
    TEST_ASSERT_EQUAL_UINT16(1000, t.point(2).raw100);
}

void test_upsert_same_raw_replaces(void)
{
    StwCalibration t;
    TEST_ASSERT_TRUE(t.upsert(4.8, 1.0));
    TEST_ASSERT_TRUE(t.upsert(4.8, 1.1));
    TEST_ASSERT_EQUAL_UINT8(1, t.size());
    TEST_ASSERT_EQUAL_UINT16(110, t.point(0).kn100);
}

void test_upsert_rejects_non_monotonic(void)
{
    StwCalibration t;
    t.upsert(4, 1);
    t.upsert(8, 2);
    t.upsert(12, 3);
    TEST_ASSERT_FALSE(t.upsert(8, 3.5));
    TEST_ASSERT_FALSE(t.upsert(6, 0.9));
    TEST_ASSERT_FALSE(t.upsert(10, 1.5));
    TEST_ASSERT_EQUAL_UINT8(3, t.size());
    TEST_ASSERT_EQUAL_UINT16(200, t.point(1).kn100);
}

void test_upsert_rejects_out_of_range(void)
{
    StwCalibration t;
    TEST_ASSERT_FALSE(t.upsert(0, 1));
    TEST_ASSERT_FALSE(t.upsert(-1, 1));
    TEST_ASSERT_FALSE(t.upsert(50.01, 1));
    TEST_ASSERT_FALSE(t.upsert(NAN, 1));
    TEST_ASSERT_FALSE(t.upsert(5, 0));
    TEST_ASSERT_FALSE(t.upsert(5, -1));
    TEST_ASSERT_FALSE(t.upsert(5, 50.01));
    TEST_ASSERT_FALSE(t.upsert(5, NAN));
    TEST_ASSERT_EQUAL_UINT8(0, t.size());
}

static void assert_full_table_unchanged(const StwCalibration &t)
{
    TEST_ASSERT_EQUAL_UINT8(5, t.size());
    for (uint8_t i = 0; i < 5; i++)
    {
        TEST_ASSERT_EQUAL_UINT16((i + 1) * 200, t.point(i).raw100);
        TEST_ASSERT_EQUAL_UINT16((i + 1) * 100, t.point(i).kn100);
    }
}

void test_upsert_full_table(void)
{
    StwCalibration t;
    for (int i = 1; i <= 5; i++)
        TEST_ASSERT_TRUE(t.upsert(i * 2.0, i * 1.0));
    assert_full_table_unchanged(t);
    // distinct raw speed rejected: after the last, between points, before the first
    TEST_ASSERT_FALSE(t.upsert(11, 5.5));
    assert_full_table_unchanged(t);
    TEST_ASSERT_FALSE(t.upsert(3, 1.2));
    assert_full_table_unchanged(t);
    TEST_ASSERT_FALSE(t.upsert(1, 0.5));
    assert_full_table_unchanged(t);
    TEST_ASSERT_FALSE(t.upsert(9, 4.5));
    assert_full_table_unchanged(t);
}

void test_upsert_replace_on_full_table(void)
{
    StwCalibration t;
    for (int i = 1; i <= 5; i++)
        t.upsert(i * 2.0, i * 1.0);
    TEST_ASSERT_TRUE(t.upsert(6, 3.2));
    TEST_ASSERT_EQUAL_UINT8(5, t.size());
    TEST_ASSERT_EQUAL_UINT16(600, t.point(2).raw100);
    TEST_ASSERT_EQUAL_UINT16(320, t.point(2).kn100);
    TEST_ASSERT_EQUAL_UINT16(400, t.point(1).raw100);
    TEST_ASSERT_EQUAL_UINT16(800, t.point(3).raw100);
    // replace first and last too
    TEST_ASSERT_TRUE(t.upsert(2, 0.5));
    TEST_ASSERT_TRUE(t.upsert(10, 6.0));
    TEST_ASSERT_EQUAL_UINT8(5, t.size());
    TEST_ASSERT_EQUAL_UINT16(50, t.point(0).kn100);
    TEST_ASSERT_EQUAL_UINT16(600, t.point(4).kn100);
    // a non monotonic replacement is still rejected
    TEST_ASSERT_FALSE(t.upsert(6, 7.0));
    TEST_ASSERT_EQUAL_UINT16(320, t.point(2).kn100);
}

void test_remove(void)
{
    StwCalibration t;
    TEST_ASSERT_FALSE(t.remove(4));
    t.upsert(4, 1);
    t.upsert(8, 2);
    t.upsert(12, 3);
    TEST_ASSERT_TRUE(t.remove(8));
    TEST_ASSERT_EQUAL_UINT8(2, t.size());
    TEST_ASSERT_EQUAL_UINT16(400, t.point(0).raw100);
    TEST_ASSERT_EQUAL_UINT16(1200, t.point(1).raw100);
    TEST_ASSERT_FALSE(t.remove(8));
    TEST_ASSERT_FALSE(t.remove(9));
    TEST_ASSERT_EQUAL_UINT8(2, t.size());
}

void test_clear(void)
{
    StwCalibration t;
    t.upsert(4, 1);
    t.upsert(8, 2);
    t.clear();
    TEST_ASSERT_EQUAL_UINT8(0, t.size());
    TEST_ASSERT_DOUBLE_WITHIN(EPS, 2.0, t.speed(2.0));
}

void test_store_load_roundtrip(void)
{
    StwCalibration a;
    a.upsert(4.8, 1.0);
    a.upsert(10.0, 2.5);
    a.upsert(20.25, 5.5);
    uint8_t blob[StwCalibration::MAX_BLOB];
    TEST_ASSERT_EQUAL_size_t(14, a.store(blob, sizeof(blob)));
    TEST_ASSERT_EQUAL_size_t(0, a.store(blob, 13));

    StwCalibration b;
    TEST_ASSERT_TRUE(b.load(blob, 14));
    TEST_ASSERT_EQUAL_UINT8(3, b.size());
    for (uint8_t i = 0; i < 3; i++)
    {
        TEST_ASSERT_EQUAL_UINT16(a.point(i).raw100, b.point(i).raw100);
        TEST_ASSERT_EQUAL_UINT16(a.point(i).kn100, b.point(i).kn100);
    }
}

void test_load_rejects_corrupt(void)
{
    StwCalibration t;
    // valid: 2 points (4.00,1.00) (8.00,2.00)
    const uint8_t good[] = {2, 2, 0x90, 0x01, 100, 0, 0x20, 0x03, 200, 0};
    TEST_ASSERT_TRUE(t.load(good, sizeof(good)));
    TEST_ASSERT_EQUAL_UINT8(2, t.size());

    uint8_t bad[sizeof(good)];
    memcpy(bad, good, sizeof(good));
    bad[0] = 3; // wrong version
    TEST_ASSERT_FALSE(t.load(bad, sizeof(bad)));
    TEST_ASSERT_EQUAL_UINT8(0, t.size());

    t.load(good, sizeof(good));
    memcpy(bad, good, sizeof(good));
    bad[1] = 6; // count > max
    TEST_ASSERT_FALSE(t.load(bad, sizeof(bad)));
    TEST_ASSERT_EQUAL_UINT8(0, t.size());

    t.load(good, sizeof(good));
    TEST_ASSERT_FALSE(t.load(good, sizeof(good) - 1)); // truncated
    TEST_ASSERT_EQUAL_UINT8(0, t.size());

    t.load(good, sizeof(good));
    TEST_ASSERT_FALSE(t.load(good, 1));
    TEST_ASSERT_FALSE(t.load(nullptr, 0));

    const uint8_t non_monotonic[] = {2, 2, 0x90, 0x01, 200, 0, 0x20, 0x03, 100, 0};
    t.load(good, sizeof(good));
    TEST_ASSERT_FALSE(t.load(non_monotonic, sizeof(non_monotonic)));
    TEST_ASSERT_EQUAL_UINT8(0, t.size());

    const uint8_t zero_raw[] = {2, 1, 0, 0, 100, 0};
    // a v1 blob (Hz -> kn points) must not be read as knots -> knots
    const uint8_t v1[] = {1, 2, 0x90, 0x01, 100, 0, 0x20, 0x03, 200, 0};
    t.load(good, sizeof(good));
    TEST_ASSERT_FALSE(t.load(v1, sizeof(v1)));
    TEST_ASSERT_EQUAL_UINT8(0, t.size());

    t.load(good, sizeof(good));
    TEST_ASSERT_FALSE(t.load(zero_raw, sizeof(zero_raw)));
    TEST_ASSERT_EQUAL_UINT8(0, t.size());
}

void test_parse_point_strict(void)
{
    double raw = 0, kn = 0;
    TEST_ASSERT_TRUE(parse_stw_point("9.6,2.0", raw, kn));
    TEST_ASSERT_EQUAL_DOUBLE(9.6, raw);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, kn);
    TEST_ASSERT_TRUE(parse_stw_point("9.6,2", raw, kn));
    TEST_ASSERT_TRUE(parse_stw_point("0.5,0.1", raw, kn));
    TEST_ASSERT_EQUAL_DOUBLE(0.1, kn);

    const char *bad[] = {"", "abc", "9.6", "9.6,", ",2", "1,2,3", "9.6,2.0x", "nan,1", "inf,1", "9.6,-2"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++)
    {
        TEST_ASSERT_FALSE_MESSAGE(parse_stw_point(bad[i], raw, kn), bad[i]);
    }
}

void test_parse_raw_strict(void)
{
    double raw = 0;
    TEST_ASSERT_TRUE(parse_stw_raw("9.6", raw));
    TEST_ASSERT_EQUAL_DOUBLE(9.6, raw);
    TEST_ASSERT_TRUE(parse_stw_raw("10", raw));
    TEST_ASSERT_EQUAL_DOUBLE(10.0, raw);
    const char *bad[] = {"", "x", "9.6x", "nan"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++)
    {
        TEST_ASSERT_FALSE_MESSAGE(parse_stw_raw(bad[i], raw), bad[i]);
    }
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_empty_table_is_identity);
    RUN_TEST(test_single_point_is_line_through_origin);
    RUN_TEST(test_interpolates_between_points);
    RUN_TEST(test_extends_last_slope_above_range);
    RUN_TEST(test_below_first_point_interpolates_from_origin);
    RUN_TEST(test_nan_and_negative_speed);
    RUN_TEST(test_upsert_keeps_sorted);
    RUN_TEST(test_upsert_same_raw_replaces);
    RUN_TEST(test_upsert_rejects_non_monotonic);
    RUN_TEST(test_upsert_rejects_out_of_range);
    RUN_TEST(test_upsert_full_table);
    RUN_TEST(test_upsert_replace_on_full_table);
    RUN_TEST(test_remove);
    RUN_TEST(test_clear);
    RUN_TEST(test_store_load_roundtrip);
    RUN_TEST(test_load_rejects_corrupt);
    RUN_TEST(test_parse_point_strict);
    RUN_TEST(test_parse_raw_strict);
    return UNITY_END();
}
