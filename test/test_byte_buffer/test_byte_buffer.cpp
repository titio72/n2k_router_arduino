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
    
    return UNITY_END();
}