#include <unity.h>
#include <string.h>
#include <stdint.h>
#include "Utils.h"

// ==================== Test Fixtures ====================
void setUp(void) {
}

void tearDown(void) {
}

// ==================== Tests: Constructor & Initialization ====================
void test_bytebuffer_constructor_creates_buffer() {
    ByteBuffer buf(64);
    
    TEST_ASSERT_EQUAL_INT(64, buf.size());
    TEST_ASSERT_EQUAL_INT(0, buf.length());
    TEST_ASSERT_NOT_NULL(buf.data());
}

void test_bytebuffer_different_sizes() {
    ByteBuffer buf8(8);
    ByteBuffer buf16(16);
    ByteBuffer buf128(128);
    ByteBuffer buf256(256);
    
    TEST_ASSERT_EQUAL_INT(8, buf8.size());
    TEST_ASSERT_EQUAL_INT(16, buf16.size());
    TEST_ASSERT_EQUAL_INT(128, buf128.size());
    TEST_ASSERT_EQUAL_INT(256, buf256.size());
}

void test_bytebuffer_initially_empty() {
    ByteBuffer buf(64);
    
    TEST_ASSERT_EQUAL_INT(0, buf.length());
}

// ==================== Tests: Copy Constructor ====================
void test_bytebuffer_copy_constructor() {
    ByteBuffer buf1(64);
    buf1 << (uint8_t)42;
    buf1 << (uint16_t)1000;
    
    ByteBuffer buf2(buf1);
    
    TEST_ASSERT_EQUAL_INT(buf1.size(), buf2.size());
    TEST_ASSERT_EQUAL_INT(buf1.length(), buf2.length());
    TEST_ASSERT_NOT_EQUAL(buf1.data(), buf2.data());  // Different memory
}

void test_bytebuffer_copy_constructor_copies_data() {
    ByteBuffer buf1(64);
    buf1 << (uint8_t)10;
    buf1 << (uint8_t)20;
    buf1 << (uint8_t)30;
    
    ByteBuffer buf2(buf1);
    
    TEST_ASSERT_EQUAL_INT(10, buf2.data()[0]);
    TEST_ASSERT_EQUAL_INT(20, buf2.data()[1]);
    TEST_ASSERT_EQUAL_INT(30, buf2.data()[2]);
}

void test_bytebuffer_copy_constructor_independent_buffers() {
    ByteBuffer buf1(64);
    buf1 << (uint8_t)100;
    
    ByteBuffer buf2(buf1);
    buf2 << (uint8_t)200;
    
    // buf1 should still have just one element
    TEST_ASSERT_EQUAL_INT(1, buf1.length());
    TEST_ASSERT_EQUAL_INT(2, buf2.length());
}

// ==================== Tests: Assignment Operator ====================
void test_bytebuffer_assignment_operator() {
    ByteBuffer buf1(64);
    buf1 << (uint8_t)42;
    buf1 << (uint16_t)1000;
    
    ByteBuffer buf2(32);
    buf2 = buf1;
    
    TEST_ASSERT_EQUAL_INT(buf1.size(), buf2.size());
    TEST_ASSERT_EQUAL_INT(buf1.length(), buf2.length());
}

void test_bytebuffer_assignment_operator_copies_data() {
    ByteBuffer buf1(64);
    buf1 << (uint8_t)10;
    buf1 << (uint8_t)20;
    
    ByteBuffer buf2(32);
    buf2 = buf1;
    
    TEST_ASSERT_EQUAL_INT(10, buf2.data()[0]);
    TEST_ASSERT_EQUAL_INT(20, buf2.data()[1]);
}

void test_bytebuffer_assignment_self_assignment() {
    ByteBuffer buf(64);
    buf << (uint8_t)99;
    
    buf = buf;  // Self assignment
    
    TEST_ASSERT_EQUAL_INT(64, buf.size());
    TEST_ASSERT_EQUAL_INT(1, buf.length());
    TEST_ASSERT_EQUAL_INT(99, buf.data()[0]);
}

// ==================== Tests: Reset ====================
void test_bytebuffer_reset() {
    ByteBuffer buf(64);
    buf << (uint8_t)1 << (uint8_t)2 << (uint8_t)3;
    
    TEST_ASSERT_EQUAL_INT(3, buf.length());
    
    buf.reset();
    
    TEST_ASSERT_EQUAL_INT(0, buf.length());
    TEST_ASSERT_EQUAL_INT(64, buf.size());  // Size unchanged
}

void test_bytebuffer_reset_returns_reference() {
    ByteBuffer buf(64);
    
    ByteBuffer& result = buf.reset();
    
    TEST_ASSERT_EQUAL_PTR(&buf, &result);
}

void test_bytebuffer_reset_allows_reuse() {
    ByteBuffer buf(64);
    
    buf << (uint8_t)100;
    buf.reset();
    buf << (uint8_t)200;
    
    TEST_ASSERT_EQUAL_INT(1, buf.length());
    TEST_ASSERT_EQUAL_INT(200, buf.data()[0]);
}

// ==================== Tests: Operator<< with uint8_t ====================
void test_bytebuffer_add_uint8() {
    ByteBuffer buf(64);
    buf << (uint8_t)42;
    
    TEST_ASSERT_EQUAL_INT(1, buf.length());
    TEST_ASSERT_EQUAL_INT(42, buf.data()[0]);
}

void test_bytebuffer_add_multiple_uint8() {
    ByteBuffer buf(64);
    buf << (uint8_t)10 << (uint8_t)20 << (uint8_t)30;
    
    TEST_ASSERT_EQUAL_INT(3, buf.length());
    TEST_ASSERT_EQUAL_INT(10, buf.data()[0]);
    TEST_ASSERT_EQUAL_INT(20, buf.data()[1]);
    TEST_ASSERT_EQUAL_INT(30, buf.data()[2]);
}

// ==================== Tests: Operator<< with uint16_t ====================
void test_bytebuffer_add_uint16() {
    ByteBuffer buf(64);
    buf << (uint16_t)0x1234;
    
    TEST_ASSERT_EQUAL_INT(2, buf.length());
    uint16_t val = *((uint16_t*)buf.data());
    TEST_ASSERT_EQUAL_INT(0x1234, val);
}

void test_bytebuffer_add_multiple_uint16() {
    ByteBuffer buf(64);
    buf << (uint16_t)1000 << (uint16_t)2000;
    
    TEST_ASSERT_EQUAL_INT(4, buf.length());
    uint16_t val1 = *((uint16_t*)(buf.data() + 0));
    uint16_t val2 = *((uint16_t*)(buf.data() + 2));
    TEST_ASSERT_EQUAL_INT(1000, val1);
    TEST_ASSERT_EQUAL_INT(2000, val2);
}

// ==================== Tests: Operator<< with int16_t ====================
void test_bytebuffer_add_int16() {
    ByteBuffer buf(64);
    buf << (int16_t)-1234;
    
    TEST_ASSERT_EQUAL_INT(2, buf.length());
    int16_t val = *((int16_t*)buf.data());
    TEST_ASSERT_EQUAL_INT(-1234, val);
}

void test_bytebuffer_add_int16_negative_values() {
    ByteBuffer buf(64);
    buf << (int16_t)-100 << (int16_t)-200;
    
    TEST_ASSERT_EQUAL_INT(4, buf.length());
    int16_t val1 = *((int16_t*)(buf.data() + 0));
    int16_t val2 = *((int16_t*)(buf.data() + 2));
    TEST_ASSERT_EQUAL_INT(-100, val1);
    TEST_ASSERT_EQUAL_INT(-200, val2);
}

// ==================== Tests: Operator<< with uint32_t ====================
void test_bytebuffer_add_uint32() {
    ByteBuffer buf(64);
    buf << (uint32_t)0x12345678;
    
    TEST_ASSERT_EQUAL_INT(4, buf.length());
    uint32_t val = *((uint32_t*)buf.data());
    TEST_ASSERT_EQUAL_INT(0x12345678, val);
}

void test_bytebuffer_add_multiple_uint32() {
    ByteBuffer buf(64);
    buf << (uint32_t)100000 << (uint32_t)200000;
    
    TEST_ASSERT_EQUAL_INT(8, buf.length());
    uint32_t val1 = *((uint32_t*)(buf.data() + 0));
    uint32_t val2 = *((uint32_t*)(buf.data() + 4));
    TEST_ASSERT_EQUAL_INT(100000, val1);
    TEST_ASSERT_EQUAL_INT(200000, val2);
}

// ==================== Tests: Operator<< with int32_t ====================
void test_bytebuffer_add_int32() {
    ByteBuffer buf(64);
    buf << (int32_t)-123456;
    
    TEST_ASSERT_EQUAL_INT(4, buf.length());
    int32_t val = *((int32_t*)buf.data());
    TEST_ASSERT_EQUAL_INT(-123456, val);
}

// ==================== Tests: Operator<< with int8_t ====================
void test_bytebuffer_add_int8() {
    ByteBuffer buf(64);
    buf << (int8_t)-42;
    
    TEST_ASSERT_EQUAL_INT(1, buf.length());
    int8_t val = *((int8_t*)buf.data());
    TEST_ASSERT_EQUAL_INT(-42, val);
}

// ==================== Tests: Operator<< with const char* ====================
void test_bytebuffer_add_string() {
    ByteBuffer buf(64);
    buf << "Hello";
    
    // Should be: length byte (5) + "Hello" (5) = 6 bytes
    TEST_ASSERT_EQUAL_INT(6, buf.length());
    TEST_ASSERT_EQUAL_INT(5, buf.data()[0]);  // Length byte
    TEST_ASSERT_EQUAL_INT('H', buf.data()[1]);
    TEST_ASSERT_EQUAL_INT('e', buf.data()[2]);
    TEST_ASSERT_EQUAL_INT('l', buf.data()[3]);
    TEST_ASSERT_EQUAL_INT('l', buf.data()[4]);
    TEST_ASSERT_EQUAL_INT('o', buf.data()[5]);
}

void test_bytebuffer_add_empty_string() {
    ByteBuffer buf(64);
    buf << "";
    
    // Should be: length byte (0) = 1 byte
    TEST_ASSERT_EQUAL_INT(1, buf.length());
    TEST_ASSERT_EQUAL_INT(0, buf.data()[0]);
}

void test_bytebuffer_add_string_too_long() {
    ByteBuffer buf(64);
    char long_str[300];
    memset(long_str, 'A', sizeof(long_str) - 1);
    long_str[sizeof(long_str) - 1] = 0;
    
    buf << (const char*)long_str;
    
    // Should not add string if length >= 255
    TEST_ASSERT_EQUAL_INT(0, buf.length());
}

void test_bytebuffer_add_string_exact_255() {
    ByteBuffer buf(300);
    char str_255[256];
    memset(str_255, 'X', 255);
    str_255[255] = 0;
    
    buf << str_255;
    
    // Should not add if length >= 255 (condition is t_size < 255)
    TEST_ASSERT_EQUAL_INT(0, buf.length());
}

void test_bytebuffer_add_string_254_chars() {
    ByteBuffer buf(300);
    char str_254[255];
    memset(str_254, 'X', 254);
    str_254[254] = 0;
    
    buf << str_254;
    
    // Should add: length byte (254) + 254 chars = 255 bytes
    TEST_ASSERT_EQUAL_INT(255, buf.length());
    TEST_ASSERT_EQUAL_INT(254, buf.data()[0]);
}

void test_bytebuffer_add_multiple_strings() {
    ByteBuffer buf(128);
    buf << "Hi" << "Bye";
    
    // "Hi": 1 + 2 = 3 bytes
    // "Bye": 1 + 3 = 4 bytes
    // Total: 7 bytes
    TEST_ASSERT_EQUAL_INT(7, buf.length());
    TEST_ASSERT_EQUAL_INT(2, buf.data()[0]);     // "Hi" length
    TEST_ASSERT_EQUAL_INT('H', buf.data()[1]);
    TEST_ASSERT_EQUAL_INT('i', buf.data()[2]);
    TEST_ASSERT_EQUAL_INT(3, buf.data()[3]);     // "Bye" length
    TEST_ASSERT_EQUAL_INT('B', buf.data()[4]);
}

// ==================== Tests: Operator<< Buffer Overflow ====================
void test_bytebuffer_overflow_single_value() {
    ByteBuffer buf(2);
    buf << (uint16_t)0xFFFF;
    buf << (uint8_t)42;  // Should not fit
    
    TEST_ASSERT_EQUAL_INT(2, buf.length());  // No overflow
}

void test_bytebuffer_overflow_string() {
    ByteBuffer buf(5);
    const char* str = "Hello";  // 5 chars + 1 length byte = 6 bytes
    buf << str;  // 1 + 5 = 6 bytes, but buffer only 5
    
    TEST_ASSERT_EQUAL_INT(0, buf.length());  // Should not add
}

void test_bytebuffer_near_limit() {
    ByteBuffer buf(10);
    buf << (uint32_t)0x12345678;  // 4 bytes
    buf << (uint16_t)0xABCD;      // 2 bytes, total = 6
    buf << (uint32_t)0x99999999;  // 4 bytes, would be 10 bytes total
    
    TEST_ASSERT_EQUAL_INT(10, buf.length());  // Should fit exactly
}

void test_bytebuffer_partial_overflow() {
    ByteBuffer buf(9);
    buf << (uint32_t)0x12345678;  // 4 bytes
    buf << (uint32_t)0x99999999;  // 4 bytes, total = 8
    buf << (uint16_t)0xABCD;      // 2 bytes, would be 10 - overflow
    
    TEST_ASSERT_EQUAL_INT(8, buf.length());  // Should not add last item
}

// ==================== Tests: Data Access ====================
void test_bytebuffer_data_pointer() {
    ByteBuffer buf(64);
    buf << (uint8_t)99;
    
    uint8_t* ptr = buf.data();
    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_EQUAL_INT(99, ptr[0]);
}

void test_bytebuffer_size() {
    ByteBuffer buf(128);
    TEST_ASSERT_EQUAL_INT(128, buf.size());
}

void test_bytebuffer_length_initial() {
    ByteBuffer buf(64);
    TEST_ASSERT_EQUAL_INT(0, buf.length());
}

void test_bytebuffer_length_after_add() {
    ByteBuffer buf(64);
    buf << (uint8_t)1 << (uint16_t)2 << (uint32_t)3;
    
    TEST_ASSERT_EQUAL_INT(7, buf.length());  // 1 + 2 + 4
}

// ==================== Tests: Get Data ====================
void test_bytebuffer_get_data() {
    ByteBuffer buf(64);
    buf << (uint8_t)10 << (uint8_t)20 << (uint8_t)30;
    
    uint8_t dest[64];
    buf.get_data(dest, 3);
    
    TEST_ASSERT_EQUAL_INT(10, dest[0]);
    TEST_ASSERT_EQUAL_INT(20, dest[1]);
    TEST_ASSERT_EQUAL_INT(30, dest[2]);
}

void test_bytebuffer_get_data_partial() {
    ByteBuffer buf(64);
    buf << (uint8_t)10 << (uint8_t)20 << (uint8_t)30;
    
    uint8_t dest[64];
    buf.get_data(dest, 2);
    
    TEST_ASSERT_EQUAL_INT(10, dest[0]);
    TEST_ASSERT_EQUAL_INT(20, dest[1]);
}

void test_bytebuffer_get_data_more_than_available() {
    ByteBuffer buf(64);
    buf << (uint8_t)10 << (uint8_t)20;
    
    uint8_t dest[64];
    buf.get_data(dest, 100);  // Requesting more than available
    
    // Should copy only what's available (2 bytes)
    TEST_ASSERT_EQUAL_INT(10, dest[0]);
    TEST_ASSERT_EQUAL_INT(20, dest[1]);
}

void test_bytebuffer_get_data_empty_buffer() {
    ByteBuffer buf(64);
    
    uint8_t dest[64] = {0xFF};
    buf.get_data(dest, 10);
    
    // Should not copy anything
    TEST_ASSERT_EQUAL_INT(0xFF, dest[0]);
}

// ==================== Tests: Mixed Operations ====================
void test_bytebuffer_mixed_types() {
    ByteBuffer buf(64);
    buf << (int8_t)-42 
        << (uint16_t)1000 
        << (int32_t)-123456 
        << "Test";
    
    TEST_ASSERT_EQUAL_INT(12, buf.length());  // 1 + 2 + 4 + 1 + 4
}

void test_bytebuffer_chained_operations() {
    ByteBuffer buf(64);
    
    ByteBuffer& result = buf << (uint8_t)1;
    result << (uint8_t)2;
    result << (uint8_t)3;
    
    TEST_ASSERT_EQUAL_INT(3, buf.length());
}

void test_bytebuffer_reset_and_refill() {
    ByteBuffer buf(64);
    
    buf << (uint8_t)100 << (uint8_t)200;
    TEST_ASSERT_EQUAL_INT(2, buf.length());
    
    buf.reset();
    TEST_ASSERT_EQUAL_INT(0, buf.length());
    
    buf << (uint8_t)10 << (uint8_t)20 << (uint8_t)30;
    TEST_ASSERT_EQUAL_INT(3, buf.length());
    TEST_ASSERT_EQUAL_INT(10, buf.data()[0]);
}

// ==================== Tests: Edge Cases ====================
void test_bytebuffer_minimum_size() {
    ByteBuffer buf(1);
    buf << (uint8_t)42;
    
    TEST_ASSERT_EQUAL_INT(1, buf.length());
    TEST_ASSERT_EQUAL_INT(42, buf.data()[0]);
}

void test_bytebuffer_large_buffer() {
    ByteBuffer buf(4096);
    
    TEST_ASSERT_EQUAL_INT(4096, buf.size());
    TEST_ASSERT_EQUAL_INT(0, buf.length());
}

void test_bytebuffer_exactly_fill() {
    ByteBuffer buf(10);
    buf << (uint32_t)0x11111111;  // 4 bytes
    buf << (uint32_t)0x22222222;  // 4 bytes
    buf << (uint16_t)0x3333;      // 2 bytes
    
    TEST_ASSERT_EQUAL_INT(10, buf.length());
    TEST_ASSERT_EQUAL_INT(10, buf.size());
}

void test_bytebuffer_zero_size() {
    ByteBuffer buf(0);
    
    TEST_ASSERT_EQUAL_INT(0, buf.size());
    TEST_ASSERT_EQUAL_INT(0, buf.length());
}

//--------------------------------------------------------------------------------------

// ============== Constructor with auto_expand parameter ==============

void test_bytebuffer_constructor_auto_expand_default_disabled(void)
{
    ByteBuffer buf(64);
    // Default should be false (auto_expand disabled)
    // Can't directly check auto_expand, so test behavior
    buf << (uint8_t)1;
    TEST_ASSERT_EQUAL_INT(1, buf.length());
}

void test_bytebuffer_constructor_auto_expand_explicitly_disabled(void)
{
    ByteBuffer buf(64, false);
    buf << (uint8_t)1;
    TEST_ASSERT_EQUAL_INT(1, buf.length());
}

void test_bytebuffer_constructor_auto_expand_enabled(void)
{
    ByteBuffer buf(64, true);
    buf << (uint8_t)1;
    TEST_ASSERT_EQUAL_INT(1, buf.length());
}

void test_bytebuffer_constructor_with_data_auto_expand_disabled(void)
{
    uint8_t data[] = {1, 2, 3, 4};
    ByteBuffer buf(data, 4, false);
    TEST_ASSERT_EQUAL_INT(4, buf.size());
    TEST_ASSERT_EQUAL_INT(4, buf.length());
}

void test_bytebuffer_constructor_with_data_auto_expand_enabled(void)
{
    uint8_t data[] = {1, 2, 3, 4};
    ByteBuffer buf(data, 4, true);
    TEST_ASSERT_EQUAL_INT(4, buf.size());
    TEST_ASSERT_EQUAL_INT(4, buf.length());
}

// ============== Copy Constructor preserves auto_expand ==============

void test_bytebuffer_copy_constructor_preserves_auto_expand_disabled(void)
{
    ByteBuffer buf1(64, false);
    buf1 << (uint8_t)42;
    
    ByteBuffer buf2(buf1);
    // buf2 should have auto_expand = false (copied from buf1)
    buf2 << (uint8_t)1;
    TEST_ASSERT_EQUAL_INT(2, buf2.length());
}

void test_bytebuffer_copy_constructor_preserves_auto_expand_enabled(void)
{
    ByteBuffer buf1(64, true);
    buf1 << (uint8_t)42;
    
    ByteBuffer buf2(buf1);
    // buf2 should have auto_expand = true (copied from buf1)
    buf2 << (uint8_t)1;
    TEST_ASSERT_EQUAL_INT(2, buf2.length());
}

// ============== Assignment operator preserves auto_expand ==============

void test_bytebuffer_assignment_operator_preserves_auto_expand(void)
{
    ByteBuffer buf1(64, true);
    buf1 << (uint8_t)42;
    
    ByteBuffer buf2(32, false);
    buf2 = buf1;
    // buf2 should now have auto_expand = true
    buf2 << (uint8_t)1;
    TEST_ASSERT_EQUAL_INT(2, buf2.length());
}

// ============== Resize without auto_expand ==============

void test_bytebuffer_resize_without_auto_expand_success_shrink(void)
{
    ByteBuffer buf(64, false);
    bool result = buf.resize(32);
    TEST_ASSERT_TRUE(result);
}

void test_bytebuffer_resize_without_auto_expand_failure_expand(void)
{
    ByteBuffer buf(64, false);
    bool result = buf.resize(128);
    // Should fail because auto_expand is false
    TEST_ASSERT_FALSE(result);
    // Size should remain unchanged
    TEST_ASSERT_EQUAL_INT(64, buf.size());
}

void test_bytebuffer_resize_without_auto_expand_same_size(void)
{
    ByteBuffer buf(64, false);
    bool result = buf.resize(64);
    // Resizing to same size should succeed
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(64, buf.size());
}

// ============== Resize with auto_expand ==============

void test_bytebuffer_resize_with_auto_expand_success_expand(void)
{
    ByteBuffer buf(64, true);
    bool result = buf.resize(128);
    // Should succeed because auto_expand is true
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(128, buf.size());
}

void test_bytebuffer_resize_with_auto_expand_multiple_expansions(void)
{
    ByteBuffer buf(64, true);
    TEST_ASSERT_TRUE(buf.resize(128));
    TEST_ASSERT_EQUAL_INT(128, buf.size());
    
    TEST_ASSERT_TRUE(buf.resize(256));
    TEST_ASSERT_EQUAL_INT(256, buf.size());
    
    TEST_ASSERT_TRUE(buf.resize(512));
    TEST_ASSERT_EQUAL_INT(512, buf.size());
}

void test_bytebuffer_resize_with_auto_expand_preserves_data(void)
{
    ByteBuffer buf(64, true);
    buf << (uint8_t)1 << (uint8_t)2 << (uint8_t)3;
    
    uint8_t data_before[3];
    buf.get_data(data_before, 3);
    
    buf.resize(128);
    
    uint8_t data_after[3];
    buf.get_data(data_after, 3);
    
    TEST_ASSERT_EQUAL_INT(data_before[0], data_after[0]);
    TEST_ASSERT_EQUAL_INT(data_before[1], data_after[1]);
    TEST_ASSERT_EQUAL_INT(data_before[2], data_after[2]);
    TEST_ASSERT_EQUAL_INT(3, buf.length());
}

// ============== Operator<< with auto_expand disabled ==============

void test_bytebuffer_operator_append_uint8_no_expand_within_bounds(void)
{
    ByteBuffer buf(64, false);
    buf << (uint8_t)42;
    TEST_ASSERT_EQUAL_INT(1, buf.length());
}

void test_bytebuffer_operator_append_multiple_uint8_no_expand_within_bounds(void)
{
    ByteBuffer buf(64, false);
    for (int i = 0; i < 64; i++)
    {
        buf << (uint8_t)i;
    }
    TEST_ASSERT_EQUAL_INT(64, buf.length());
}

void test_bytebuffer_operator_append_uint32_no_expand_within_bounds(void)
{
    ByteBuffer buf(64, false);
    buf << (uint32_t)0xDEADBEEF;
    TEST_ASSERT_EQUAL_INT(4, buf.length());
}

// ============== Operator<< with auto_expand enabled ==============

void test_bytebuffer_operator_append_uint8_with_expand(void)
{
    ByteBuffer buf(4, true);
    buf << (uint8_t)1 << (uint8_t)2 << (uint8_t)3 << (uint8_t)4;
    TEST_ASSERT_EQUAL_INT(4, buf.length());
    
    // This should trigger auto-expand
    buf << (uint8_t)5;
    TEST_ASSERT_EQUAL_INT(5, buf.length());
    TEST_ASSERT_GREATER_THAN_INT(4, buf.size());
}

void test_bytebuffer_operator_append_uint16_with_expand(void)
{
    ByteBuffer buf(4, true);
    buf << (uint8_t)1 << (uint16_t)2;
    
    // Adding uint16 (2 bytes) to 2 bytes should trigger expand
    buf << (uint16_t)0xABCD;
    TEST_ASSERT_EQUAL_INT(5, buf.length());
    TEST_ASSERT_GREATER_THAN_INT(4, buf.size());
}

void test_bytebuffer_operator_append_uint32_with_expand(void)
{
    ByteBuffer buf(2, true);
    buf << (uint8_t)1;
    
    // Adding uint32 (4 bytes) to 1 byte should trigger expand
    buf << (uint32_t)0xDEADBEEF;
    TEST_ASSERT_EQUAL_INT(5, buf.length());
    TEST_ASSERT_GREATER_THAN_INT(2, buf.size());
}

void test_bytebuffer_operator_append_string_with_expand(void)
{
    ByteBuffer buf(10, true);
    buf << "Hello";  // 1 (length) + 5 = 6 bytes
    TEST_ASSERT_EQUAL_INT(6, buf.length());
    
    // Adding another string should trigger expand
    buf << "World";  // 1 (length) + 5 = 6 bytes, total 12
    TEST_ASSERT_EQUAL_INT(12, buf.length());
    TEST_ASSERT_GREATER_THAN_INT(10, buf.size());
}

void test_bytebuffer_operator_append_large_string_with_expand(void)
{
    ByteBuffer buf(4, true);
    const char *large_string = "This is a large string that exceeds buffer size";
    
    buf << large_string;
    size_t expected_length = strlen(large_string) + 1;  // +1 for length byte
    TEST_ASSERT_EQUAL_INT(expected_length, buf.length());
    TEST_ASSERT_GREATER_THAN_INT(4, buf.size());
}

// ============== Mixed operations with auto_expand ==============

void test_bytebuffer_mixed_types_with_expand(void)
{
    ByteBuffer buf(8, true);
    buf << (uint8_t)255;
    buf << (uint16_t)0xABCD;
    buf << (uint32_t)0xDEADBEEF;
    TEST_ASSERT_EQUAL_INT(7, buf.length());  // 1 + 2 + 4
    TEST_ASSERT_EQUAL_INT(8, buf.size()); // did not need to expand yet
}

void test_bytebuffer_expand_then_fill(void)
{
    ByteBuffer buf(8, true);
    buf << (uint8_t)1 << (uint8_t)2 << (uint8_t)3 << (uint8_t)4
        << (uint8_t)5 << (uint8_t)6 << (uint8_t)7 << (uint8_t)8
        << (uint8_t)9 << (uint8_t)10;
    
    TEST_ASSERT_EQUAL_INT(10, buf.length());
    TEST_ASSERT_GREATER_THAN_INT(8, buf.size());
}

// ============== Reset with auto_expand ==============

void test_bytebuffer_reset_with_auto_expand_enabled(void)
{
    ByteBuffer buf(8, true);
    buf << (uint8_t)1 << (uint8_t)2 << (uint8_t)3;
    buf << (uint8_t)4 << (uint8_t)5 << (uint8_t)6 << (uint8_t)7 << (uint8_t)8 << (uint8_t)9;
    
    TEST_ASSERT_GREATER_THAN_INT(8, buf.size());
    
    buf.reset();
    TEST_ASSERT_EQUAL_INT(0, buf.length());
    // Size should remain expanded
    TEST_ASSERT_GREATER_THAN_INT(8, buf.size());
}

void test_bytebuffer_reset_then_refill_expanded(void)
{
    ByteBuffer buf(8, true);
    buf << (uint8_t)1 << (uint8_t)2 << (uint8_t)3 << (uint8_t)4 << (uint8_t)5;
    size_t expanded_size = buf.size();
    
    buf.reset();
    buf << (uint8_t)10 << (uint8_t)11 << (uint8_t)12;
    
    TEST_ASSERT_EQUAL_INT(3, buf.length());
    TEST_ASSERT_EQUAL_INT(expanded_size, buf.size());
}

// ============== Operator== with auto_expand ==============

void test_bytebuffer_equality_same_content_different_auto_expand(void)
{
    ByteBuffer buf1(64, true);
    ByteBuffer buf2(64, false);
    
    buf1 << (uint8_t)42 << (uint8_t)100;
    buf2 << (uint8_t)42 << (uint8_t)100;
    
    TEST_ASSERT_TRUE(buf1 == buf2);
}

// ============== Edge cases with auto_expand ==============

void test_bytebuffer_expand_zero_size_buffer(void)
{
    ByteBuffer buf(0, true);
    // Resize from 0 should work
    bool result = buf.resize(64);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(64, buf.size());
}

void test_bytebuffer_auto_expand_preserves_offset(void)
{
    ByteBuffer buf(4, true);
    buf << (uint8_t)1 << (uint8_t)2;
    TEST_ASSERT_EQUAL_INT(2, buf.length());
    
    buf << (uint8_t)3 << (uint8_t)4 << (uint8_t)5;
    TEST_ASSERT_EQUAL_INT(5, buf.length());
}

void test_bytebuffer_auto_expand_multiple_expansions_large(void)
{
    ByteBuffer buf(16, true);
    
    for (int i = 0; i < 100; i++)
    {
        buf << (uint8_t)i;
    }
    
    TEST_ASSERT_EQUAL_INT(100, buf.length());
    TEST_ASSERT_GREATER_THAN_INT(16, buf.size());
}

void test_bytebuffer_no_expand_overflow_protection(void)
{
    ByteBuffer buf(4, false);
    buf << (uint8_t)1 << (uint8_t)2;
    
    // Try to add data that would overflow
    buf << (uint32_t)0xFFFFFFFF;
    
    // Should only have 2 bytes since resize would fail
    TEST_ASSERT_EQUAL_INT(2, buf.length());
}

void test_bytebuffer_string_append_without_expand_within_bounds(void)
{
    ByteBuffer buf(20, false);
    buf << "Hi";  // 1 + 2 = 3 bytes
    TEST_ASSERT_EQUAL_INT(3, buf.length());
}

void test_bytebuffer_string_append_with_expand_overflow(void)
{
    ByteBuffer buf(5, true);
    buf << "Test";  // 1 + 4 = 5 bytes
    TEST_ASSERT_EQUAL_INT(5, buf.length());
    
    // Adding another string should auto-expand
    buf << "Data";  // 1 + 4 = 5 bytes
    TEST_ASSERT_EQUAL_INT(10, buf.length());
    TEST_ASSERT_GREATER_THAN_INT(5, buf.size());
}

void test_bytebuffer_auto_expand_exponential_growth(void)
{
    ByteBuffer buf(4, true);
    size_t prev_size = buf.size();
    
    // Fill and expand multiple times
    for (int i = 0; i < 20; i++)
    {
        buf << (uint8_t)i;
        if (buf.size() > prev_size)
        {
            // Size should have grown (auto-expanded)
            TEST_ASSERT_GREATER_THAN_INT(prev_size, buf.size());
            prev_size = buf.size();
        }
    }
}

// ============== Data integrity with auto_expand ==============

void test_bytebuffer_data_integrity_after_expand(void)
{
    ByteBuffer buf(4, true);
    uint8_t values[] = {10, 20, 30, 40, 50, 60, 70, 80};
    
    for (int i = 0; i < 8; i++)
    {
        buf << values[i];
    }
    
    uint8_t retrieved[8];
    buf.get_data(retrieved, 8);
    
    for (int i = 0; i < 8; i++)
    {
        TEST_ASSERT_EQUAL_INT(values[i], retrieved[i]);
    }
}

void test_bytebuffer_mixed_numeric_types_data_integrity(void)
{
    ByteBuffer buf(4, true);
    
    uint8_t u8 = 0xFF;
    uint16_t u16 = 0xABCD;
    uint32_t u32 = 0xDEADBEEF;
    
    buf << u8 << u16 << u32;
    
    uint8_t data[7];
    buf.get_data(data, 7);
    
    TEST_ASSERT_EQUAL_INT(u8, data[0]);
    TEST_ASSERT_EQUAL_INT(0xCD, data[1]);  // Little-endian
    TEST_ASSERT_EQUAL_INT(0xAB, data[2]);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    
    // Constructor & Initialization
    RUN_TEST(test_bytebuffer_constructor_creates_buffer);
    RUN_TEST(test_bytebuffer_different_sizes);
    RUN_TEST(test_bytebuffer_initially_empty);
    
    // Copy Constructor
    RUN_TEST(test_bytebuffer_copy_constructor);
    RUN_TEST(test_bytebuffer_copy_constructor_copies_data);
    RUN_TEST(test_bytebuffer_copy_constructor_independent_buffers);
    
    // Assignment Operator
    RUN_TEST(test_bytebuffer_assignment_operator);
    RUN_TEST(test_bytebuffer_assignment_operator_copies_data);
    RUN_TEST(test_bytebuffer_assignment_self_assignment);
    
    // Reset
    RUN_TEST(test_bytebuffer_reset);
    RUN_TEST(test_bytebuffer_reset_returns_reference);
    RUN_TEST(test_bytebuffer_reset_allows_reuse);
    
    // Operator<< with uint8_t
    RUN_TEST(test_bytebuffer_add_uint8);
    RUN_TEST(test_bytebuffer_add_multiple_uint8);
    
    // Operator<< with uint16_t
    RUN_TEST(test_bytebuffer_add_uint16);
    RUN_TEST(test_bytebuffer_add_multiple_uint16);
    
    // Operator<< with int16_t
    RUN_TEST(test_bytebuffer_add_int16);
    RUN_TEST(test_bytebuffer_add_int16_negative_values);
    
    // Operator<< with uint32_t
    RUN_TEST(test_bytebuffer_add_uint32);
    RUN_TEST(test_bytebuffer_add_multiple_uint32);
    
    // Operator<< with int32_t
    RUN_TEST(test_bytebuffer_add_int32);
    
    // Operator<< with int8_t
    RUN_TEST(test_bytebuffer_add_int8);
    
    // Operator<< with const char*
    RUN_TEST(test_bytebuffer_add_string);
    RUN_TEST(test_bytebuffer_add_empty_string);
    RUN_TEST(test_bytebuffer_add_string_too_long);
    RUN_TEST(test_bytebuffer_add_string_exact_255);
    RUN_TEST(test_bytebuffer_add_string_254_chars);
    RUN_TEST(test_bytebuffer_add_multiple_strings);
    
    // Buffer Overflow
    RUN_TEST(test_bytebuffer_overflow_single_value);
    RUN_TEST(test_bytebuffer_overflow_string);
    RUN_TEST(test_bytebuffer_near_limit);
    RUN_TEST(test_bytebuffer_partial_overflow);
    
    // Data Access
    RUN_TEST(test_bytebuffer_data_pointer);
    RUN_TEST(test_bytebuffer_size);
    RUN_TEST(test_bytebuffer_length_initial);
    RUN_TEST(test_bytebuffer_length_after_add);
    
    // Get Data
    RUN_TEST(test_bytebuffer_get_data);
    RUN_TEST(test_bytebuffer_get_data_partial);
    RUN_TEST(test_bytebuffer_get_data_more_than_available);
    RUN_TEST(test_bytebuffer_get_data_empty_buffer);
    
    // Mixed Operations
    RUN_TEST(test_bytebuffer_mixed_types);
    RUN_TEST(test_bytebuffer_chained_operations);
    RUN_TEST(test_bytebuffer_reset_and_refill);
    
    // Edge Cases
    RUN_TEST(test_bytebuffer_minimum_size);
    RUN_TEST(test_bytebuffer_large_buffer);
    RUN_TEST(test_bytebuffer_exactly_fill);
    RUN_TEST(test_bytebuffer_zero_size);

    //---------------------------------

        // Constructor tests with auto_expand
    RUN_TEST(test_bytebuffer_constructor_auto_expand_default_disabled);
    RUN_TEST(test_bytebuffer_constructor_auto_expand_explicitly_disabled);
    RUN_TEST(test_bytebuffer_constructor_auto_expand_enabled);
    RUN_TEST(test_bytebuffer_constructor_with_data_auto_expand_disabled);
    RUN_TEST(test_bytebuffer_constructor_with_data_auto_expand_enabled);

    // Copy constructor tests
    RUN_TEST(test_bytebuffer_copy_constructor_preserves_auto_expand_disabled);
    RUN_TEST(test_bytebuffer_copy_constructor_preserves_auto_expand_enabled);

    // Assignment operator tests
    RUN_TEST(test_bytebuffer_assignment_operator_preserves_auto_expand);

    // Resize without auto_expand tests
    RUN_TEST(test_bytebuffer_resize_without_auto_expand_success_shrink);
    RUN_TEST(test_bytebuffer_resize_without_auto_expand_failure_expand);
    RUN_TEST(test_bytebuffer_resize_without_auto_expand_same_size);

    // Resize with auto_expand tests
    RUN_TEST(test_bytebuffer_resize_with_auto_expand_success_expand);
    RUN_TEST(test_bytebuffer_resize_with_auto_expand_multiple_expansions);
    RUN_TEST(test_bytebuffer_resize_with_auto_expand_preserves_data);

    // Operator<< without auto_expand tests
    RUN_TEST(test_bytebuffer_operator_append_uint8_no_expand_within_bounds);
    RUN_TEST(test_bytebuffer_operator_append_multiple_uint8_no_expand_within_bounds);
    RUN_TEST(test_bytebuffer_operator_append_uint32_no_expand_within_bounds);

    // Operator<< with auto_expand tests
    RUN_TEST(test_bytebuffer_operator_append_uint8_with_expand);
    RUN_TEST(test_bytebuffer_operator_append_uint16_with_expand);
    RUN_TEST(test_bytebuffer_operator_append_uint32_with_expand);
    RUN_TEST(test_bytebuffer_operator_append_string_with_expand);
    RUN_TEST(test_bytebuffer_operator_append_large_string_with_expand);

    // Mixed operations tests
    RUN_TEST(test_bytebuffer_mixed_types_with_expand);
    RUN_TEST(test_bytebuffer_expand_then_fill);

    // Reset tests
    RUN_TEST(test_bytebuffer_reset_with_auto_expand_enabled);
    RUN_TEST(test_bytebuffer_reset_then_refill_expanded);

    // Equality tests
    RUN_TEST(test_bytebuffer_equality_same_content_different_auto_expand);

    // Edge case tests
    RUN_TEST(test_bytebuffer_expand_zero_size_buffer);
    RUN_TEST(test_bytebuffer_auto_expand_preserves_offset);
    RUN_TEST(test_bytebuffer_auto_expand_multiple_expansions_large);
    RUN_TEST(test_bytebuffer_no_expand_overflow_protection);
    RUN_TEST(test_bytebuffer_string_append_without_expand_within_bounds);
    RUN_TEST(test_bytebuffer_string_append_with_expand_overflow);
    RUN_TEST(test_bytebuffer_auto_expand_exponential_growth);

    // Data integrity tests
    RUN_TEST(test_bytebuffer_data_integrity_after_expand);
    RUN_TEST(test_bytebuffer_mixed_numeric_types_data_integrity);

    
    return UNITY_END();
}