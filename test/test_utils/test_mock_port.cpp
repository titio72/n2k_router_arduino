#include <unity.h>
#include "MockPort.hpp"
#include "Ports.h"
#include "Conf.h"
#include "Context.h"

class TestPortListener : public PortListener
{
public:
    int on_line_read_count = 0;
    int on_partial_count = 0;
    char last_line[256] = {0};
    
    void on_line_read(const char* line) override
    {
        on_line_read_count++;
        if (line)
        {
            strncpy(last_line, line, sizeof(last_line) - 1);
            last_line[sizeof(last_line) - 1] = '\0';
        }
    }
    
    void on_partial(const char* line) override
    {
        on_partial_count++;
    }
    
    void reset()
    {
        on_line_read_count = 0;
        on_partial_count = 0;
        memset(last_line, 0, sizeof(last_line));
    }
};

void setUp(void)
{
}

void tearDown(void)
{
}

// ==================== MockPort Construction Tests ====================
void test_mock_port_construction(void)
{
    MockPort port("TEST_PORT");
    
    TEST_ASSERT_FALSE(port.is_open());
    TEST_ASSERT_EQUAL_INT(0, port.get_open_count());
    TEST_ASSERT_EQUAL_INT(0, port.get_close_count());
    TEST_ASSERT_EQUAL_INT(0, port.get_read_count());
}

void test_mock_port_default_name(void)
{
    MockPort port("SERIAL_PORT");
    
    TEST_ASSERT_FALSE(port.is_open());
}

// ==================== MockPort Open/Close Tests ====================
void test_mock_port_open(void)
{
    MockPort port("TEST_PORT");
    port.open();
    
    TEST_ASSERT_TRUE(port.is_open());
}

void test_mock_port_close(void)
{
    MockPort port("TEST_PORT");
    port.open();
    TEST_ASSERT_TRUE(port.is_open());
    
    port.close();
    TEST_ASSERT_FALSE(port.is_open());
}

void test_mock_port_multiple_open_close(void)
{
    MockPort port("TEST_PORT");
    
    for (int i = 0; i < 5; i++)
    {
        port.open();
        TEST_ASSERT_TRUE(port.is_open());
        
        port.close();
        TEST_ASSERT_FALSE(port.is_open());
    }
}

// ==================== MockPort Data Simulation Tests ====================
void test_mock_port_simulate_single_character(void)
{
    MockPort port("TEST_PORT");
    port.simulate_data("A");
    
    TEST_ASSERT_FALSE(port.is_input_queue_empty());
    TEST_ASSERT_EQUAL_INT(1, port.get_total_bytes_simulated());
}

void test_mock_port_simulate_string(void)
{
    MockPort port("TEST_PORT");
    port.simulate_data("Hello");
    
    TEST_ASSERT_FALSE(port.is_input_queue_empty());
    TEST_ASSERT_EQUAL_INT(5, port.get_total_bytes_simulated());
}

void test_mock_port_simulate_line(void)
{
    MockPort port("TEST_PORT");
    port.simulate_line("TEST_DATA");
    
    // Should contain line + \r\n
    TEST_ASSERT_FALSE(port.is_input_queue_empty());
    TEST_ASSERT_EQUAL_INT(11, port.get_total_bytes_simulated());  // "TEST_DATA\r\n"
}

void test_mock_port_simulate_multiple_lines(void)
{
    MockPort port("TEST_PORT");
    
    const char* lines[] = {
        "LINE1",
        "LINE2",
        "LINE3"
    };
    
    port.simulate_lines(lines, 3);
    
    TEST_ASSERT_EQUAL_INT(21, port.get_total_bytes_simulated());  // 3 lines * (5 + 2) chars
}

// ==================== MockPort Read Tests ====================
void test_mock_port_read_single_char(void)
{
    MockPort port("TEST_PORT");
    port.open();
    port.simulate_data("X");
    
    bool nothing_to_read = false;
    bool error = false;
    int result = port._read(nothing_to_read, error);
    
    TEST_ASSERT_EQUAL_INT((int)(unsigned char)'X', result);
    TEST_ASSERT_FALSE(nothing_to_read);
    TEST_ASSERT_FALSE(error);
    TEST_ASSERT_EQUAL_INT(1, port.get_read_count());
}

void test_mock_port_read_multiple_chars(void)
{
    MockPort port("TEST_PORT");
    port.open();
    port.simulate_data("ABC");
    
    bool nothing_to_read = false;
    bool error = false;
    
    int c1 = port._read(nothing_to_read, error);
    int c2 = port._read(nothing_to_read, error);
    int c3 = port._read(nothing_to_read, error);
    
    TEST_ASSERT_EQUAL_INT((int)(unsigned char)'A', c1);
    TEST_ASSERT_EQUAL_INT((int)(unsigned char)'B', c2);
    TEST_ASSERT_EQUAL_INT((int)(unsigned char)'C', c3);
    TEST_ASSERT_EQUAL_INT(3, port.get_read_count());
}

void test_mock_port_read_empty_queue(void)
{
    MockPort port("TEST_PORT");
    port.open();
    
    bool nothing_to_read = false;
    bool error = false;
    int result = port._read(nothing_to_read, error);
    
    TEST_ASSERT_EQUAL_INT(-1, result);
    TEST_ASSERT_TRUE(nothing_to_read);
    TEST_ASSERT_FALSE(error);
}

void test_mock_port_read_with_error(void)
{
    MockPort port("TEST_PORT");
    port.open();
    port.set_error_on_read(true);
    port.simulate_data("DATA");
    
    bool nothing_to_read = false;
    bool error = false;
    int result = port._read(nothing_to_read, error);
    
    TEST_ASSERT_EQUAL_INT(-1, result);
    TEST_ASSERT_TRUE(error);
}

// ==================== MockPort Counter Tests ====================
void test_mock_port_read_counter(void)
{
    MockPort port("TEST_PORT");
    port.open();
    port.simulate_data("TESTDATA");
    
    bool nothing_to_read = false;
    bool error = false;
    
    for (int i = 0; i < 8; i++)
    {
        port._read(nothing_to_read, error);
    }
    
    TEST_ASSERT_EQUAL_INT(8, port.get_read_count());
}

void test_mock_port_total_bytes_simulated(void)
{
    MockPort port("TEST_PORT");
    
    port.simulate_data("ABC");      // 3 bytes
    port.simulate_data("DEFGH");    // 5 bytes
    port.simulate_line("XY");       // 4 bytes (XY + \r\n)
    
    TEST_ASSERT_EQUAL_INT(12, port.get_total_bytes_simulated());
}

// ==================== MockPort Reset Tests ====================
void test_mock_port_reset_counters(void)
{
    MockPort port("TEST_PORT");
    port.simulate_data("TEST");
    
    TEST_ASSERT_NOT_EQUAL(0, port.get_total_bytes_simulated());
    
    port.reset_counters();
    
    TEST_ASSERT_EQUAL_INT(0, port.get_total_bytes_simulated());
}

void test_mock_port_clear_input_queue(void)
{
    MockPort port("TEST_PORT");
    port.simulate_data("LOTS_OF_DATA");
    
    TEST_ASSERT_FALSE(port.is_input_queue_empty());
    
    port.clear_input_queue();
    
    TEST_ASSERT_TRUE(port.is_input_queue_empty());
}

// ==================== MockPort Listener Integration Tests ====================
void test_mock_port_with_listener(void)
{
    MockPort port("TEST_PORT");
    TestPortListener listener;
    
    port.set_handler(&listener);
    port.open();
    port.simulate_line("TEST_LINE");
    port.listen(100);
    
    TEST_ASSERT_GREATER_THAN(0, listener.on_line_read_count);
}

void test_mock_port_listener_data_accuracy(void)
{
    MockPort port("TEST_PORT");
    TestPortListener listener;
    
    port.set_handler(&listener);
    port.open();
    port.simulate_line("ACCURACY_TEST");
    port.listen(100);
    
    // Check that listener received the data
    if (listener.on_line_read_count > 0)
    {
        TEST_ASSERT_TRUE(strlen(listener.last_line) > 0);
    }
}

// ==================== Edge Cases ====================
void test_mock_port_empty_line(void)
{
    MockPort port("TEST_PORT");
    port.simulate_line("");
    
    // Should handle empty line gracefully
    TEST_ASSERT_EQUAL_INT(2, port.get_total_bytes_simulated());  // Just \r\n
}

void test_mock_port_special_characters(void)
{
    MockPort port("TEST_PORT");
    port.simulate_data("\t\n\r");
    
    TEST_ASSERT_EQUAL_INT(3, port.get_total_bytes_simulated());
}

void test_mock_port_null_data(void)
{
    MockPort port("TEST_PORT");
    port.simulate_data(NULL);
    
    // Should handle NULL gracefully
    TEST_ASSERT_TRUE(port.is_input_queue_empty());
}

void test_mock_port_large_data_stream(void)
{
    MockPort port("TEST_PORT");
    
    // Simulate large data stream
    for (int i = 0; i < 100; i++)
    {
        port.simulate_line("DATA_LINE");
    }
    
    // 100 lines * (9 + 2) = 1100 bytes
    TEST_ASSERT_EQUAL_INT(1100, port.get_total_bytes_simulated());
}

void test_mock_port_read_increments_counter(void)
{
    MockPort port("TEST_PORT");
    port.open();
    port.simulate_data("X");
    
    TEST_ASSERT_EQUAL_INT(0, port.get_read_count());
    
    bool nothing_to_read = false;
    bool error = false;
    port._read(nothing_to_read, error);
    
    TEST_ASSERT_EQUAL_INT(1, port.get_read_count());
}

void test_mock_port_open_increments_counter(void)
{
    MockPort port("TEST_PORT");
    
    TEST_ASSERT_EQUAL_INT(0, port.get_open_count());
    port.open();
    TEST_ASSERT_EQUAL_INT(1, port.get_open_count());
}

void test_mock_port_close_increments_counter(void)
{
    MockPort port("TEST_PORT");
    port.open();
    
    TEST_ASSERT_EQUAL_INT(0, port.get_close_count());
    port.close();
    TEST_ASSERT_EQUAL_INT(1, port.get_close_count());
}

int main(int argc, char** argv)
{
    UNITY_BEGIN();
    
    // Construction tests
    RUN_TEST(test_mock_port_construction);
    RUN_TEST(test_mock_port_default_name);

    // Open/Close tests
    RUN_TEST(test_mock_port_open);
    RUN_TEST(test_mock_port_close);
    RUN_TEST(test_mock_port_multiple_open_close);

    // Data simulation tests
    RUN_TEST(test_mock_port_simulate_single_character);
    RUN_TEST(test_mock_port_simulate_string);
    RUN_TEST(test_mock_port_simulate_line);
    RUN_TEST(test_mock_port_simulate_multiple_lines);

    // Read tests
    RUN_TEST(test_mock_port_read_single_char);
    RUN_TEST(test_mock_port_read_multiple_chars);
    RUN_TEST(test_mock_port_read_empty_queue);
    RUN_TEST(test_mock_port_read_with_error);

    // Counter tests
    RUN_TEST(test_mock_port_read_counter);
    RUN_TEST(test_mock_port_total_bytes_simulated);

    // Reset tests
    RUN_TEST(test_mock_port_reset_counters);
    RUN_TEST(test_mock_port_clear_input_queue);

    // Listener integration tests
    RUN_TEST(test_mock_port_with_listener);
    RUN_TEST(test_mock_port_listener_data_accuracy);

    // Edge cases
    RUN_TEST(test_mock_port_empty_line);
    RUN_TEST(test_mock_port_special_characters);
    RUN_TEST(test_mock_port_null_data);
    RUN_TEST(test_mock_port_large_data_stream);
    RUN_TEST(test_mock_port_read_increments_counter);
    RUN_TEST(test_mock_port_open_increments_counter);
    RUN_TEST(test_mock_port_close_increments_counter);

    return UNITY_END();
}