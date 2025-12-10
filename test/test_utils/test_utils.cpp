#include <unity.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "Utils.h"

// ============== Tests: N2KSid ==============

void test_n2ksid_constructor_initializes_to_zero(void)
{
    N2KSid sid;
    TEST_ASSERT_EQUAL_INT(0, sid.getCurrent());
}

void test_n2ksid_get_new_increments(void)
{
    N2KSid sid;
    unsigned char first = sid.getNew();
    TEST_ASSERT_EQUAL_INT(1, first);
    TEST_ASSERT_EQUAL_INT(1, sid.getCurrent());
}

void test_n2ksid_get_new_multiple_increments(void)
{
    N2KSid sid;
    sid.getNew();
    sid.getNew();
    unsigned char third = sid.getNew();
    TEST_ASSERT_EQUAL_INT(3, third);
}

void test_n2ksid_get_current_does_not_increment(void)
{
    N2KSid sid;
    sid.getNew();
    unsigned char current1 = sid.getCurrent();
    unsigned char current2 = sid.getCurrent();
    TEST_ASSERT_EQUAL_INT(current1, current2);
}

void test_n2ksid_wraps_at_253(void)
{
    N2KSid sid;
    // Get to 252
    for (int i = 0; i < 252; i++)
    {
        sid.getNew();
    }
    TEST_ASSERT_EQUAL_INT(252, sid.getCurrent());
    
    // Next should wrap to 0
    unsigned char wrapped = sid.getNew();
    TEST_ASSERT_EQUAL_INT(0, wrapped);
    TEST_ASSERT_EQUAL_INT(0, sid.getCurrent());
}

void test_n2ksid_wraps_then_increments_again(void)
{
    N2KSid sid;
    for (int i = 0; i < 252; i++)
    {
        sid.getNew();
    }
    sid.getNew(); // wraps to 0
    unsigned char next = sid.getNew(); // should be 1
    TEST_ASSERT_EQUAL_INT(1, next);
}

void test_n2ksid_full_cycle(void)
{
    N2KSid sid;
    for (int i = 0; i < 253 * 2; i++)
    {
        unsigned char val = sid.getNew();
        TEST_ASSERT(val < 253);
    }
}

// ============== Tests: startswith ==============

void test_startswith_exact_match(void)
{
    TEST_ASSERT_TRUE(startswith("hello", "hello"));
}

void test_startswith_prefix_match(void)
{
    TEST_ASSERT_TRUE(startswith("hello", "helloworld"));
}

void test_startswith_single_char_match(void)
{
    TEST_ASSERT_TRUE(startswith("h", "hello"));
}

void test_startswith_no_match(void)
{
    TEST_ASSERT_FALSE(startswith("world", "hello"));
}

void test_startswith_no_match_prefix(void)
{
    TEST_ASSERT_FALSE(startswith("hel", "world"));
}

void test_startswith_empty_search_string(void)
{
    TEST_ASSERT_TRUE(startswith("", "hello"));
}

void test_startswith_empty_target_string(void)
{
    TEST_ASSERT_FALSE(startswith("hello", ""));
}

void test_startswith_both_empty(void)
{
    TEST_ASSERT_TRUE(startswith("", ""));
}

void test_startswith_case_sensitive(void)
{
    TEST_ASSERT_FALSE(startswith("Hello", "hello"));
}

void test_startswith_longer_search_than_target(void)
{
    TEST_ASSERT_FALSE(startswith("helloworld", "hello"));
}

void test_startswith_special_characters(void)
{
    TEST_ASSERT_TRUE(startswith("$hello", "$helloworld"));
}

void test_startswith_numbers(void)
{
    TEST_ASSERT_TRUE(startswith("123", "123456"));
}

// ============== Tests: indexOf ==============

void test_indexof_found_at_start(void)
{
    TEST_ASSERT_EQUAL_INT(0, indexOf("helloworld", "hello"));
}

void test_indexof_found_in_middle(void)
{
    TEST_ASSERT_EQUAL_INT(5, indexOf("helloworld", "world"));
}

void test_indexof_single_char_found(void)
{
    TEST_ASSERT_EQUAL_INT(1, indexOf("hello", "e"));
}

void test_indexof_not_found(void)
{
    TEST_ASSERT_EQUAL_INT(-1, indexOf("hello", "xyz"));
}

void test_indexof_empty_needle(void)
{
    // strstr("hello", "") returns pointer to "hello"
    TEST_ASSERT_EQUAL_INT(0, indexOf("hello", ""));
}

void test_indexof_empty_haystack(void)
{
    TEST_ASSERT_EQUAL_INT(-1, indexOf("", "hello"));
}

void test_indexof_both_empty(void)
{
    TEST_ASSERT_EQUAL_INT(0, indexOf("", ""));
}

void test_indexof_exact_match(void)
{
    TEST_ASSERT_EQUAL_INT(0, indexOf("hello", "hello"));
}

void test_indexof_multiple_occurrences_returns_first(void)
{
    TEST_ASSERT_EQUAL_INT(0, indexOf("ababab", "ab"));
}

void test_indexof_last_occurrence(void)
{
    TEST_ASSERT_EQUAL_INT(3, indexOf("hello", "lo"));
}

// ============== Tests: array_contains ==============

void test_array_contains_found(void)
{
    short arr[] = {1, 2, 3, 4, 5};
    TEST_ASSERT_TRUE(array_contains(3, arr, 5));
}

void test_array_contains_not_found(void)
{
    short arr[] = {1, 2, 3, 4, 5};
    TEST_ASSERT_FALSE(array_contains(10, arr, 5));
}

void test_array_contains_first_element(void)
{
    short arr[] = {1, 2, 3, 4, 5};
    TEST_ASSERT_TRUE(array_contains(1, arr, 5));
}

void test_array_contains_last_element(void)
{
    short arr[] = {1, 2, 3, 4, 5};
    TEST_ASSERT_TRUE(array_contains(5, arr, 5));
}

void test_array_contains_empty_array(void)
{
    short arr[] = {1, 2, 3};
    TEST_ASSERT_FALSE(array_contains(1, arr, 0));
}

void test_array_contains_negative_numbers(void)
{
    short arr[] = {-1, -2, -3, 1, 2, 3};
    TEST_ASSERT_TRUE(array_contains(-2, arr, 6));
}

void test_array_contains_single_element_found(void)
{
    short arr[] = {42};
    TEST_ASSERT_TRUE(array_contains(42, arr, 1));
}

void test_array_contains_single_element_not_found(void)
{
    short arr[] = {42};
    TEST_ASSERT_FALSE(array_contains(0, arr, 1));
}

void test_array_contains_zero(void)
{
    short arr[] = {0, 1, 2, 3};
    TEST_ASSERT_TRUE(array_contains(0, arr, 4));
}

// ============== Tests: getDaysSince1970 ==============

void test_get_days_since_1970_epoch(void)
{
    int days = getDaysSince1970(1970, 1, 1);
    TEST_ASSERT_EQUAL_INT(0, days);
}

void test_get_days_since_1970_one_day_later(void)
{
    int days = getDaysSince1970(1970, 1, 2);
    TEST_ASSERT_EQUAL_INT(1, days);
}

void test_get_days_since_1970_one_year_later(void)
{
    int days = getDaysSince1970(1971, 1, 1);
    // 1970 was not a leap year, so 365 days
    TEST_ASSERT_EQUAL_INT(365, days);
}

void test_get_days_since_1970_leap_year(void)
{
    int days = getDaysSince1970(1972, 1, 1);
    // 1970 + 1971 = 365 + 365 = 730 days
    TEST_ASSERT_EQUAL_INT(730, days);
}

void test_get_days_since_1970_feb_28_non_leap(void)
{
    int days1 = getDaysSince1970(1970, 2, 28);
    int days2 = getDaysSince1970(1970, 3, 1);
    // Feb 28 and Mar 1 should differ by 1 day in non-leap year
    TEST_ASSERT_EQUAL_INT(1, days2 - days1);
}

void test_get_days_since_1970_feb_28_leap_year(void)
{
    int days1 = getDaysSince1970(1972, 2, 28);
    int days2 = getDaysSince1970(1972, 2, 29);
    int days3 = getDaysSince1970(1972, 3, 1);
    TEST_ASSERT_EQUAL_INT(1, days2 - days1);
    TEST_ASSERT_EQUAL_INT(1, days3 - days2);
}

void test_get_days_since_1970_year_2000(void)
{
    int days = getDaysSince1970(2000, 1, 1);
    // 2000 is a leap year, result should be positive
    TEST_ASSERT_GREATER_THAN_INT(0, days);
}

void test_get_days_since_1970_december(void)
{
    int days1 = getDaysSince1970(1970, 1, 1);
    int days2 = getDaysSince1970(1970, 12, 31);
    TEST_ASSERT_GREATER_THAN_INT(days1, days2);
}

// ============== Tests: format_thousands_sep ==============

void test_format_thousands_sep_less_than_thousand(void)
{
    char buf[50];
    format_thousands_sep(buf, 123);
    TEST_ASSERT_EQUAL_STRING("123", buf);
}

void test_format_thousands_sep_exactly_thousand(void)
{
    char buf[50];
    format_thousands_sep(buf, 1000);
    TEST_ASSERT_EQUAL_STRING("1,000", buf);
}

void test_format_thousands_sep_multiple_thousands(void)
{
    char buf[50];
    format_thousands_sep(buf, 1234567);
    TEST_ASSERT_EQUAL_STRING("1,234,567", buf);
}

void test_format_thousands_sep_zero(void)
{
    char buf[50];
    format_thousands_sep(buf, 0);
    TEST_ASSERT_EQUAL_STRING("0", buf);
}

void test_format_thousands_sep_ten_thousand(void)
{
    char buf[50];
    format_thousands_sep(buf, 10000);
    TEST_ASSERT_EQUAL_STRING("10,000", buf);
}

void test_format_thousands_sep_one_million(void)
{
    char buf[50];
    format_thousands_sep(buf, 1000000);
    TEST_ASSERT_EQUAL_STRING("1,000,000", buf);
}

void test_format_thousands_sep_9999(void)
{
    char buf[50];
    format_thousands_sep(buf, 9999);
    TEST_ASSERT_EQUAL_STRING("9,999", buf);
}

void test_format_thousands_sep_999999(void)
{
    char buf[50];
    format_thousands_sep(buf, 999999);
    TEST_ASSERT_EQUAL_STRING("999,999", buf);
}

// ============== Tests: lpf (Low Pass Filter) ==============

void test_lpf_alpha_zero(void)
{
    double result = lpf(100.0, 50.0, 0.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 50.0, result);
}

void test_lpf_alpha_one(void)
{
    double result = lpf(100.0, 50.0, 1.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 100.0, result);
}

void test_lpf_alpha_half(void)
{
    double result = lpf(100.0, 50.0, 0.5);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 75.0, result);
}

void test_lpf_same_values(void)
{
    double result = lpf(50.0, 50.0, 0.5);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 50.0, result);
}

void test_lpf_increasing_sequence(void)
{
    double val = 0.0;
    val = lpf(10.0, val, 0.5);  // 5.0
    val = lpf(10.0, val, 0.5);  // 7.5
    val = lpf(10.0, val, 0.5);  // 8.75
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 8.75, val);
}

void test_lpf_decreasing_sequence(void)
{
    double val = 100.0;
    val = lpf(0.0, val, 0.5);   // 50.0
    val = lpf(0.0, val, 0.5);   // 25.0
    val = lpf(0.0, val, 0.5);   // 12.5
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 12.5, val);
}

void test_lpf_negative_values(void)
{
    double result = lpf(-100.0, 50.0, 0.5);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, -25.0, result);
}

void test_lpf_small_alpha(void)
{
    double result = lpf(100.0, 50.0, 0.1);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 55.0, result);
}

void test_lpf_large_alpha(void)
{
    double result = lpf(100.0, 50.0, 0.9);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 95.0, result);
}

// ============== Tests: check_elapsed ==============

void test_check_elapsed_first_call(void)
{
    unsigned long last_time = 0;
    unsigned long result = check_elapsed(1000, last_time, 500);
    TEST_ASSERT_EQUAL_INT(1000, result);
    TEST_ASSERT_EQUAL_INT(1000, last_time);
}

void test_check_elapsed_within_period(void)
{
    unsigned long last_time = 1000;
    unsigned long result = check_elapsed(1200, last_time, 500);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1000, last_time);
}

void test_check_elapsed_exact_period(void)
{
    unsigned long last_time = 1000;
    unsigned long result = check_elapsed(1500, last_time, 500);
    TEST_ASSERT_NOT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_INT(1500, last_time);
}

void test_check_elapsed_beyond_period(void)
{
    unsigned long last_time = 1000;
    unsigned long result = check_elapsed(2000, last_time, 500);
    TEST_ASSERT_NOT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_INT(2000, last_time);
}

void test_check_elapsed_time_overflow(void)
{
    unsigned long last_time = 0xFFFFFFFEUL;
    unsigned long result = check_elapsed(0, last_time, 500);
    // Overflow: 0 - 0xFFFFFFFE = negative, which is < 0
    TEST_ASSERT_NOT_EQUAL(0, result);
}

void test_check_elapsed_zero_period(void)
{
    unsigned long last_time = 1000;
    unsigned long result = check_elapsed(1001, last_time, 0);
    // dT = 1, period = 0, so dT >= period (true)
    TEST_ASSERT_NOT_EQUAL(0, result);
}

void test_check_elapsed_consecutive_calls(void)
{
    unsigned long last_time = 0;
    check_elapsed(1000, last_time, 500);
    unsigned long result = check_elapsed(1200, last_time, 500);
    TEST_ASSERT_EQUAL_INT(0, result);
}

// ============== Tests: replace ==============

void test_replace_simple_replacement(void)
{
    char *result = replace("hello world", "world", "friend", false);
    TEST_ASSERT_EQUAL_STRING("hello friend", result);
    free(result);
}

void test_replace_multiple_occurrences_all(void)
{
    char *result = replace("cat cat cat", "cat", "dog", false);
    TEST_ASSERT_EQUAL_STRING("dog dog dog", result);
    free(result);
}

void test_replace_multiple_occurrences_first_only(void)
{
    char *result = replace("cat cat cat", "cat", "dog", true);
    TEST_ASSERT_EQUAL_STRING("dog cat cat", result);
    free(result);
}

void test_replace_no_match(void)
{
    char *result = replace("hello world", "xyz", "abc", false);
    TEST_ASSERT_EQUAL_STRING("hello world", result);
    free(result);
}

void test_replace_longer_replacement(void)
{
    char *result = replace("hi", "hi", "hello", false);
    TEST_ASSERT_EQUAL_STRING("hello", result);
    free(result);
}

void test_replace_shorter_replacement(void)
{
    char *result = replace("hello", "hello", "hi", false);
    TEST_ASSERT_EQUAL_STRING("hi", result);
    free(result);
}

void test_replace_empty_replacement(void)
{
    char *result = replace("hello world", "world", "", false);
    TEST_ASSERT_EQUAL_STRING("hello ", result);
    free(result);
}

void test_replace_empty_original(void)
{
    char *result = replace("", "a", "b", false);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

void test_replace_pattern_at_end(void)
{
    char *result = replace("hello world", "world", "universe", false);
    TEST_ASSERT_EQUAL_STRING("hello universe", result);
    free(result);
}

void test_replace_overlapping_patterns_first_only(void)
{
    char *result = replace("aaa", "aa", "bb", true);
    TEST_ASSERT_EQUAL_STRING("bba", result);
    free(result);
}

void test_replace_special_characters(void)
{
    char *result = replace("$100 is $100", "$100", "$50", false);
    TEST_ASSERT_EQUAL_STRING("$50 is $50", result);
    free(result);
}

void test_replace_numbers(void)
{
    char *result = replace("123 456 123", "123", "999", false);
    TEST_ASSERT_EQUAL_STRING("999 456 999", result);
    free(result);
}

// ============== Tests: replace_and_free ==============

void test_replace_and_free_simple(void)
{
    char *orig = (char *)malloc(20);
    strcpy(orig, "hello world");
    
    char *result = replace_and_free(orig, "world", "friend", false);
    TEST_ASSERT_EQUAL_STRING("hello friend", result);
    free(result);
}

void test_replace_and_free_multiple(void)
{
    char *orig = (char *)malloc(20);
    strcpy(orig, "a a a");
    
    char *result = replace_and_free(orig, "a", "b", false);
    TEST_ASSERT_EQUAL_STRING("b b b", result);
    free(result);
}

void test_replace_and_free_first_only(void)
{
    char *orig = (char *)malloc(20);
    strcpy(orig, "a a a");
    
    char *result = replace_and_free(orig, "a", "b", true);
    TEST_ASSERT_EQUAL_STRING("b a a", result);
    free(result);
}

// ============== Main Test Runner ==============

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    // N2KSid Tests
    RUN_TEST(test_n2ksid_constructor_initializes_to_zero);
    RUN_TEST(test_n2ksid_get_new_increments);
    RUN_TEST(test_n2ksid_get_new_multiple_increments);
    RUN_TEST(test_n2ksid_get_current_does_not_increment);
    RUN_TEST(test_n2ksid_wraps_at_253);
    RUN_TEST(test_n2ksid_wraps_then_increments_again);
    RUN_TEST(test_n2ksid_full_cycle);

    // startswith Tests
    RUN_TEST(test_startswith_exact_match);
    RUN_TEST(test_startswith_prefix_match);
    RUN_TEST(test_startswith_single_char_match);
    RUN_TEST(test_startswith_no_match);
    RUN_TEST(test_startswith_no_match_prefix);
    RUN_TEST(test_startswith_empty_search_string);
    RUN_TEST(test_startswith_empty_target_string);
    RUN_TEST(test_startswith_both_empty);
    RUN_TEST(test_startswith_case_sensitive);
    RUN_TEST(test_startswith_longer_search_than_target);
    RUN_TEST(test_startswith_special_characters);
    RUN_TEST(test_startswith_numbers);

    // indexOf Tests
    RUN_TEST(test_indexof_found_at_start);
    RUN_TEST(test_indexof_found_in_middle);
    RUN_TEST(test_indexof_single_char_found);
    RUN_TEST(test_indexof_not_found);
    RUN_TEST(test_indexof_empty_needle);
    RUN_TEST(test_indexof_empty_haystack);
    RUN_TEST(test_indexof_both_empty);
    RUN_TEST(test_indexof_exact_match);
    RUN_TEST(test_indexof_multiple_occurrences_returns_first);
    RUN_TEST(test_indexof_last_occurrence);

    // array_contains Tests
    RUN_TEST(test_array_contains_found);
    RUN_TEST(test_array_contains_not_found);
    RUN_TEST(test_array_contains_first_element);
    RUN_TEST(test_array_contains_last_element);
    RUN_TEST(test_array_contains_empty_array);
    RUN_TEST(test_array_contains_negative_numbers);
    RUN_TEST(test_array_contains_single_element_found);
    RUN_TEST(test_array_contains_single_element_not_found);
    RUN_TEST(test_array_contains_zero);

    // getDaysSince1970 Tests
    RUN_TEST(test_get_days_since_1970_epoch);
    RUN_TEST(test_get_days_since_1970_one_day_later);
    RUN_TEST(test_get_days_since_1970_one_year_later);
    RUN_TEST(test_get_days_since_1970_leap_year);
    RUN_TEST(test_get_days_since_1970_feb_28_non_leap);
    RUN_TEST(test_get_days_since_1970_feb_28_leap_year);
    RUN_TEST(test_get_days_since_1970_year_2000);
    RUN_TEST(test_get_days_since_1970_december);

    // format_thousands_sep Tests
    RUN_TEST(test_format_thousands_sep_less_than_thousand);
    RUN_TEST(test_format_thousands_sep_exactly_thousand);
    RUN_TEST(test_format_thousands_sep_multiple_thousands);
    RUN_TEST(test_format_thousands_sep_zero);
    RUN_TEST(test_format_thousands_sep_ten_thousand);
    RUN_TEST(test_format_thousands_sep_one_million);
    RUN_TEST(test_format_thousands_sep_9999);
    RUN_TEST(test_format_thousands_sep_999999);

    // lpf Tests
    RUN_TEST(test_lpf_alpha_zero);
    RUN_TEST(test_lpf_alpha_one);
    RUN_TEST(test_lpf_alpha_half);
    RUN_TEST(test_lpf_same_values);
    RUN_TEST(test_lpf_increasing_sequence);
    RUN_TEST(test_lpf_decreasing_sequence);
    RUN_TEST(test_lpf_negative_values);
    RUN_TEST(test_lpf_small_alpha);
    RUN_TEST(test_lpf_large_alpha);

    // check_elapsed Tests
    RUN_TEST(test_check_elapsed_first_call);
    RUN_TEST(test_check_elapsed_within_period);
    RUN_TEST(test_check_elapsed_exact_period);
    RUN_TEST(test_check_elapsed_beyond_period);
    RUN_TEST(test_check_elapsed_time_overflow);
    RUN_TEST(test_check_elapsed_zero_period);
    RUN_TEST(test_check_elapsed_consecutive_calls);

    // replace Tests
    RUN_TEST(test_replace_simple_replacement);
    RUN_TEST(test_replace_multiple_occurrences_all);
    RUN_TEST(test_replace_multiple_occurrences_first_only);
    RUN_TEST(test_replace_no_match);
    RUN_TEST(test_replace_longer_replacement);
    RUN_TEST(test_replace_shorter_replacement);
    RUN_TEST(test_replace_empty_replacement);
    RUN_TEST(test_replace_empty_original);
    RUN_TEST(test_replace_pattern_at_end);
    RUN_TEST(test_replace_overlapping_patterns_first_only);
    RUN_TEST(test_replace_special_characters);
    RUN_TEST(test_replace_numbers);

    // replace_and_free Tests
    RUN_TEST(test_replace_and_free_simple);
    RUN_TEST(test_replace_and_free_multiple);
    RUN_TEST(test_replace_and_free_first_only);

    return UNITY_END();
}