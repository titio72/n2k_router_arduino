#include <unity.h>
#include <math.h>
#include "Agents.hpp"
#include "Data.h"
#include "Conf.h"
#include "N2K_router.h"
#include "Conf.h"
// Include mocks
#include "Context.h"

// Mock Agent for testing
class MockAgent
{
public:
    bool enabled;
    int enable_call_count;
    int disable_call_count;
    int loop_call_count;
    int setup_call_count;
    unsigned long last_loop_time;
    bool should_fail_enable;

    MockAgent()
        : enabled(false),
          enable_call_count(0),
          disable_call_count(0),
          loop_call_count(0),
          setup_call_count(0),
          last_loop_time(0),
          should_fail_enable(false)
    {
    }

    void enable()
    {
        enable_call_count++;
        if (!should_fail_enable)
        {
            enabled = true;
        }
    }

    void disable()
    {
        disable_call_count++;
        enabled = false;
    }

    bool is_enabled() const
    {
        return enabled;
    }

    void loop(unsigned long time, Context &context)
    {
        loop_call_count++;
        last_loop_time = time;
    }

    void setup(Context &context)
    {
        setup_call_count++;
    }

    void reset_counts()
    {
        enable_call_count = 0;
        disable_call_count = 0;
        loop_call_count = 0;
        setup_call_count = 0;
        last_loop_time = 0;
    }
};

void setUp(void)
{
    // Reset before each test
}

void tearDown(void)
{
    // Cleanup after each test
}

#pragma region handle_agent_enable Tests

void test_handle_agent_enable_enables_disabled_agent(void)
{
    MockAgent agent;
    unsigned short retry = 0;
    
    MOCK_CONTEXT

    bool result = handle_agent_enable(agent, true, &retry, "TestAgent");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(1, agent.enable_call_count);
    TEST_ASSERT_EQUAL_INT(0, retry);
}

void test_handle_agent_enable_idempotent_when_already_enabled(void)
{
    MockAgent agent;
    unsigned short retry = 0;
    MOCK_CONTEXT

    agent.enable();
    agent.reset_counts();

    bool result = handle_agent_enable(agent, true, &retry, "TestAgent");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, agent.enable_call_count);
    TEST_ASSERT_EQUAL_INT(0, retry);
}

void test_handle_agent_enable_handles_enable_failure(void)
{
    MockAgent agent;
    agent.should_fail_enable = true;
    unsigned short retry = 0;
    MOCK_CONTEXT

    bool result = handle_agent_enable(agent, true, &retry, "TestAgent");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(1, agent.enable_call_count);
    TEST_ASSERT_EQUAL_INT(1, retry);
}

void test_handle_agent_enable_retries_on_failure(void)
{
    MockAgent agent;
    agent.should_fail_enable = true;
    unsigned short retry = 0;
    MOCK_CONTEXT

    // First attempt
    bool result1 = handle_agent_enable(agent, true, &retry, "TestAgent");
    TEST_ASSERT_FALSE(result1);
    TEST_ASSERT_EQUAL_INT(1, retry);

    // Second attempt
    bool result2 = handle_agent_enable(agent, true, &retry, "TestAgent");
    TEST_ASSERT_FALSE(result2);
    TEST_ASSERT_EQUAL_INT(2, retry);

    // Third attempt
    bool result3 = handle_agent_enable(agent, true, &retry, "TestAgent");
    TEST_ASSERT_FALSE(result3);
    TEST_ASSERT_EQUAL_INT(3, retry);
}

void test_handle_agent_enable_respects_max_retry(void)
{
    MockAgent agent;
    agent.should_fail_enable = true;
    unsigned short retry = MAX_RETRY - 1;
    MOCK_CONTEXT

    bool result = handle_agent_enable(agent, true, &retry, "TestAgent");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(MAX_RETRY, retry);
}

void test_handle_agent_enable_stops_after_max_retry(void)
{
    MockAgent agent;
    agent.should_fail_enable = true;
    unsigned short retry = MAX_RETRY;
    MOCK_CONTEXT

    agent.reset_counts();
    bool result = handle_agent_enable(agent, true, &retry, "TestAgent");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(0, agent.enable_call_count);  // Should not attempt
    TEST_ASSERT_EQUAL_INT(MAX_RETRY, retry);
}

void test_handle_agent_enable_resets_retry_on_success(void)
{
    MockAgent agent;
    unsigned short retry = 2;
    MOCK_CONTEXT

    bool result = handle_agent_enable(agent, true, &retry, "TestAgent");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, retry);
}

void test_handle_agent_enable_without_retry_pointer(void)
{
    MockAgent agent;
    MOCK_CONTEXT

    bool result = handle_agent_enable(agent, true, NULL, "TestAgent");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(1, agent.enable_call_count);
}

void test_handle_agent_enable_without_description(void)
{
    MockAgent agent;
    unsigned short retry = 0;
    MOCK_CONTEXT

    bool result = handle_agent_enable(agent, true, &retry);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(agent.is_enabled());
}

void test_handle_agent_enable_with_null_retry_and_fail(void)
{
    MockAgent agent;
    agent.should_fail_enable = true;
    MOCK_CONTEXT

    bool result = handle_agent_enable(agent, true, NULL, "TestAgent");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(agent.is_enabled());
    // Should still attempt to enable without retry tracking
    TEST_ASSERT_EQUAL_INT(1, agent.enable_call_count);
}

void test_handle_agent_enable_multiple_calls_eventually_succeeds(void)
{
    MockAgent agent;
    agent.should_fail_enable = true;
    unsigned short retry = 0;
    MOCK_CONTEXT

    // Fail 3 times
    for (int i = 0; i < 2; i++)
    {
        bool result = handle_agent_enable(agent, true, &retry, "TestAgent");
        TEST_ASSERT_FALSE(result);
        TEST_ASSERT_EQUAL_INT(i + 1, retry);
    }

    // Now succeed
    agent.should_fail_enable = false;
    bool result = handle_agent_enable(agent, true, &retry, "TestAgent");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, retry);
    TEST_ASSERT_EQUAL_INT(3, agent.enable_call_count);
}

#pragma endregion

#pragma region handle_agent_loop Tests

void test_handle_agent_loop_enables_and_calls_loop_when_enable_true(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;
    unsigned long micros = 1000000;

    agent.setup(context);
    handle_agent_loop(agent, context, true, &retry, micros, "TestAgent");

    TEST_ASSERT_TRUE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(1, agent.loop_call_count);
    TEST_ASSERT_EQUAL_INT(micros, agent.last_loop_time);
}

void test_handle_agent_loop_disables_when_enable_false(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;
    unsigned long micros = 1000000;

    agent.setup(context);
    agent.enable();
    TEST_ASSERT_TRUE(agent.is_enabled());

    handle_agent_loop(agent, context, false, &retry, micros, "TestAgent");

    TEST_ASSERT_FALSE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(1, agent.disable_call_count);
}

void test_handle_agent_loop_does_not_call_loop_when_enable_false(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;
    unsigned long micros = 1000000;

    agent.setup(context);
    agent.enable();
    agent.reset_counts();

    handle_agent_loop(agent, context, false, &retry, micros, "TestAgent");

    TEST_ASSERT_EQUAL_INT(0, agent.loop_call_count);
}

void test_handle_agent_loop_resets_retry_when_disable(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 5;
    unsigned long micros = 1000000;

    agent.setup(context);
    agent.enable();

    handle_agent_loop(agent, context, false, &retry, micros, "TestAgent");

    TEST_ASSERT_EQUAL_INT(0, retry);
}

void test_handle_agent_loop_does_not_loop_if_enable_fails(void)
{
    MockAgent agent;
    agent.should_fail_enable = true;
    MOCK_CONTEXT
    unsigned short retry = 0;
    unsigned long micros = 1000000;

    agent.setup(context);

    handle_agent_loop(agent, context, true, &retry, micros, "TestAgent");

    TEST_ASSERT_FALSE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(0, agent.loop_call_count);
}

void test_handle_agent_loop_retries_enable_on_failure(void)
{
    MockAgent agent;
    agent.should_fail_enable = true;
    MOCK_CONTEXT
    unsigned short retry = 0;
    unsigned long micros = 1000000;

    agent.setup(context);

    // First call - fails
    handle_agent_loop(agent, context, true, &retry, micros, "TestAgent");
    TEST_ASSERT_EQUAL_INT(1, retry);
    TEST_ASSERT_FALSE(agent.is_enabled());

    // Second call - still fails
    handle_agent_loop(agent, context, true, &retry, micros + 1000, "TestAgent");
    TEST_ASSERT_EQUAL_INT(2, retry);
    TEST_ASSERT_FALSE(agent.is_enabled());
}

void test_handle_agent_loop_passes_correct_timestamp(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;
    unsigned long micros = 5000000;

    agent.setup(context);
    handle_agent_loop(agent, context, true, &retry, micros, "TestAgent");

    TEST_ASSERT_EQUAL_INT(micros, agent.last_loop_time);
}

void test_handle_agent_loop_without_retry_pointer(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned long micros = 1000000;

    agent.setup(context);
    handle_agent_loop(agent, context, true, NULL, micros, "TestAgent");

    TEST_ASSERT_TRUE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(1, agent.loop_call_count);
}

void test_handle_agent_loop_without_description(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;
    unsigned long micros = 1000000;

    agent.setup(context);
    handle_agent_loop(agent, context, true, &retry, micros);

    TEST_ASSERT_TRUE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(1, agent.loop_call_count);
}

void test_handle_agent_loop_multiple_calls_enabled(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;

    agent.setup(context);

    for (int i = 0; i < 5; i++)
    {
        unsigned long micros = 1000000 * (i + 1);
        handle_agent_loop(agent, context, true, &retry, micros, "TestAgent");
    }

    TEST_ASSERT_TRUE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(5, agent.loop_call_count);
    TEST_ASSERT_EQUAL_INT(5000000, agent.last_loop_time);
}

void test_handle_agent_loop_toggle_enable_disable(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;

    agent.setup(context);

    // Enable
    handle_agent_loop(agent, context, true, &retry, 1000000, "TestAgent");
    TEST_ASSERT_TRUE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(1, agent.loop_call_count);

    // Disable
    handle_agent_loop(agent, context, false, &retry, 2000000, "TestAgent");
    TEST_ASSERT_FALSE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(1, agent.loop_call_count);  // No additional loop call

    // Enable again
    handle_agent_loop(agent, context, true, &retry, 3000000, "TestAgent");
    TEST_ASSERT_TRUE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(2, agent.loop_call_count);
}

void test_handle_agent_loop_disable_when_already_disabled(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;

    agent.setup(context);
    // Don't enable

    handle_agent_loop(agent, context, false, &retry, 1000000, "TestAgent");

    TEST_ASSERT_FALSE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(0, agent.loop_call_count);
}

void test_handle_agent_loop_zero_timestamp(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;

    agent.setup(context);
    handle_agent_loop(agent, context, true, &retry, 0, "TestAgent");

    TEST_ASSERT_EQUAL_INT(0, agent.last_loop_time);
    TEST_ASSERT_EQUAL_INT(1, agent.loop_call_count);
}

void test_handle_agent_loop_large_timestamp(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;
    unsigned long large_micros = 0xFFFFFFFFUL;

    agent.setup(context);
    handle_agent_loop(agent, context, true, &retry, large_micros, "TestAgent");

    TEST_ASSERT_EQUAL_INT(large_micros, agent.last_loop_time);
}

#pragma endregion

#pragma region Integration Tests

void test_agent_lifecycle_setup_enable_loop_disable(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;

    // Setup
    agent.setup(context);
    TEST_ASSERT_EQUAL_INT(1, agent.setup_call_count);
    TEST_ASSERT_FALSE(agent.is_enabled());

    // Enable via loop
    handle_agent_loop(agent, context, true, &retry, 1000000, "TestAgent");
    TEST_ASSERT_TRUE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(1, agent.enable_call_count);
    TEST_ASSERT_EQUAL_INT(1, agent.loop_call_count);

    // Loop again
    handle_agent_loop(agent, context, true, &retry, 2000000, "TestAgent");
    TEST_ASSERT_TRUE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(2, agent.loop_call_count);

    // Disable
    handle_agent_loop(agent, context, false, &retry, 3000000, "TestAgent");
    TEST_ASSERT_FALSE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(1, agent.disable_call_count);
}

void test_agent_lifecycle_with_enable_failures(void)
{
    MockAgent agent;
    agent.should_fail_enable = true;
    MOCK_CONTEXT
    unsigned short retry = 0;

    agent.setup(context);

    // Try to enable - fails
    for (int i = 0; i < 3; i++)
    {
        handle_agent_loop(agent, context, true, &retry, 1000000 * (i + 1), "TestAgent");
        TEST_ASSERT_FALSE(agent.is_enabled());
        TEST_ASSERT_EQUAL_INT(i + 1, retry);
        TEST_ASSERT_EQUAL_INT(0, agent.loop_call_count);
    }

    // Disable while failed
    handle_agent_loop(agent, context, false, &retry, 4000000, "TestAgent");
    TEST_ASSERT_FALSE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(0, retry);
}

void test_agent_lifecycle_success_after_failures(void)
{
    MockAgent agent;
    agent.should_fail_enable = true;
    MOCK_CONTEXT
    unsigned short retry = 0;

    agent.setup(context);

    // Fail twice
    handle_agent_loop(agent, context, true, &retry, 1000000, "TestAgent");
    TEST_ASSERT_EQUAL_INT(1, retry);

    handle_agent_loop(agent, context, true, &retry, 2000000, "TestAgent");
    TEST_ASSERT_EQUAL_INT(2, retry);

    // Now succeed
    agent.should_fail_enable = false;
    handle_agent_loop(agent, context, true, &retry, 3000000, "TestAgent");
    TEST_ASSERT_TRUE(agent.is_enabled());
    TEST_ASSERT_EQUAL_INT(0, retry);
    TEST_ASSERT_EQUAL_INT(1, agent.loop_call_count);
}

void test_multiple_agents_independent_state(void)
{
    MockAgent agent1;
    MockAgent agent2;
    MockAgent agent3;
    MOCK_CONTEXT
    unsigned short retry1 = 0;
    unsigned short retry2 = 0;
    unsigned short retry3 = 0;

    agent1.setup(context);
    agent2.setup(context);
    agent3.setup(context);

    // Enable agent1
    handle_agent_loop(agent1, context, true, &retry1, 1000000, "Agent1");
    TEST_ASSERT_TRUE(agent1.is_enabled());
    TEST_ASSERT_FALSE(agent2.is_enabled());
    TEST_ASSERT_FALSE(agent3.is_enabled());

    // Enable agent2
    handle_agent_loop(agent2, context, true, &retry2, 1000000, "Agent2");
    TEST_ASSERT_TRUE(agent1.is_enabled());
    TEST_ASSERT_TRUE(agent2.is_enabled());
    TEST_ASSERT_FALSE(agent3.is_enabled());

    // Disable agent1
    handle_agent_loop(agent1, context, false, &retry1, 2000000, "Agent1");
    TEST_ASSERT_FALSE(agent1.is_enabled());
    TEST_ASSERT_TRUE(agent2.is_enabled());
    TEST_ASSERT_FALSE(agent3.is_enabled());
}

void test_agent_with_dynamic_enable_config(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;

    agent.setup(context);

    // Enable
    bool enable_from_config = true;
    handle_agent_loop(agent, context, enable_from_config, &retry, 1000000, "TestAgent");
    TEST_ASSERT_TRUE(agent.is_enabled());

    // Change config and disable
    enable_from_config = false;
    handle_agent_loop(agent, context, enable_from_config, &retry, 2000000, "TestAgent");
    TEST_ASSERT_FALSE(agent.is_enabled());

    // Change config and enable again
    enable_from_config = true;
    handle_agent_loop(agent, context, enable_from_config, &retry, 3000000, "TestAgent");
    TEST_ASSERT_TRUE(agent.is_enabled());
}

void test_agent_retry_counter_persistence(void)
{
    MockAgent agent;
    agent.should_fail_enable = true;
    MOCK_CONTEXT
    unsigned short retry = 0;

    agent.setup(context);

    // Build up retries
    for (int i = 0; i < 3; i++)
    {
        handle_agent_loop(agent, context, true, &retry, 1000000, "TestAgent");
    }
    TEST_ASSERT_EQUAL_INT(3, retry);

    // Disable should reset retry
    handle_agent_loop(agent, context, false, &retry, 2000000, "TestAgent");
    TEST_ASSERT_EQUAL_INT(0, retry);

    // Enable again with clean retry counter
    agent.should_fail_enable = false;
    handle_agent_loop(agent, context, true, &retry, 3000000, "TestAgent");
    TEST_ASSERT_EQUAL_INT(0, retry);
    TEST_ASSERT_TRUE(agent.is_enabled());
}

#pragma endregion

#pragma region Edge Case Tests

void test_agent_enable_with_max_retry_boundary(void)
{
    MockAgent agent;
    agent.should_fail_enable = true;
    MOCK_CONTEXT

    agent.setup(context);

    // Test right at MAX_RETRY boundary
    unsigned short retry = MAX_RETRY - 1;
    handle_agent_enable(agent, true, &retry, "TestAgent");
    TEST_ASSERT_EQUAL_INT(MAX_RETRY, retry);
    TEST_ASSERT_FALSE(agent.is_enabled());

    // Next call should not attempt enable
    int enable_count_before = agent.enable_call_count;
    handle_agent_enable(agent, true, &retry, "TestAgent");
    TEST_ASSERT_EQUAL_INT(enable_count_before, agent.enable_call_count);
}

void test_agent_alternating_enable_disable_rapid(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;

    agent.setup(context);

    for (int i = 0; i < 10; i++)
    {
        bool enable = (i % 2 == 0);
        handle_agent_loop(agent, context, enable, &retry, 1000000 * (i + 1), "TestAgent");
        
        if (enable)
        {
            TEST_ASSERT_TRUE(agent.is_enabled());
        }
        else
        {
            TEST_ASSERT_FALSE(agent.is_enabled());
        }
    }
}

void test_agent_enable_already_enabled_not_called_again(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;

    agent.setup(context);

    // First enable
    handle_agent_enable(agent, true, &retry, "TestAgent");
    int first_enable_count = agent.enable_call_count;

    // Second call should not call enable again
    handle_agent_enable(agent, true, &retry, "TestAgent");
    TEST_ASSERT_EQUAL_INT(first_enable_count, agent.enable_call_count);
}

void test_agent_disable_when_already_disabled(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;

    agent.setup(context);
    // Agent starts disabled

    handle_agent_loop(agent, context, false, &retry, 1000000, "TestAgent");

    TEST_ASSERT_EQUAL_INT(1, agent.disable_call_count);  // Still calls disable
}

void test_agent_loop_microsecond_precision(void)
{
    MockAgent agent;
    MOCK_CONTEXT
    unsigned short retry = 0;

    agent.setup(context);

    // Test with specific microsecond values
    unsigned long test_times[] = {0, 1, 999, 1000, 1000000, 1000001, 1000000000, 0xFFFFFFFE};

    for (size_t i = 0; i < sizeof(test_times) / sizeof(test_times[0]); i++)
    {
        agent.reset_counts();
        handle_agent_loop(agent, context, true, &retry, test_times[i], "TestAgent");
        TEST_ASSERT_EQUAL_INT(test_times[i], agent.last_loop_time);
    }
}

#pragma endregion

// Test runner
void run_agent_tests(void)
{
    // handle_agent_enable tests
    RUN_TEST(test_handle_agent_enable_enables_disabled_agent);
    RUN_TEST(test_handle_agent_enable_idempotent_when_already_enabled);
    RUN_TEST(test_handle_agent_enable_handles_enable_failure);
    RUN_TEST(test_handle_agent_enable_retries_on_failure);
    RUN_TEST(test_handle_agent_enable_respects_max_retry);
    RUN_TEST(test_handle_agent_enable_stops_after_max_retry);
    RUN_TEST(test_handle_agent_enable_resets_retry_on_success);
    RUN_TEST(test_handle_agent_enable_without_retry_pointer);
    RUN_TEST(test_handle_agent_enable_without_description);
    RUN_TEST(test_handle_agent_enable_with_null_retry_and_fail);
    RUN_TEST(test_handle_agent_enable_multiple_calls_eventually_succeeds);

    // handle_agent_loop tests
    RUN_TEST(test_handle_agent_loop_enables_and_calls_loop_when_enable_true);
    RUN_TEST(test_handle_agent_loop_disables_when_enable_false);
    RUN_TEST(test_handle_agent_loop_does_not_call_loop_when_enable_false);
    RUN_TEST(test_handle_agent_loop_resets_retry_when_disable);
    RUN_TEST(test_handle_agent_loop_does_not_loop_if_enable_fails);
    RUN_TEST(test_handle_agent_loop_retries_enable_on_failure);
    RUN_TEST(test_handle_agent_loop_passes_correct_timestamp);
    RUN_TEST(test_handle_agent_loop_without_retry_pointer);
    RUN_TEST(test_handle_agent_loop_without_description);
    RUN_TEST(test_handle_agent_loop_multiple_calls_enabled);
    RUN_TEST(test_handle_agent_loop_toggle_enable_disable);
    RUN_TEST(test_handle_agent_loop_disable_when_already_disabled);
    RUN_TEST(test_handle_agent_loop_zero_timestamp);
    RUN_TEST(test_handle_agent_loop_large_timestamp);

    // Integration tests
    RUN_TEST(test_agent_lifecycle_setup_enable_loop_disable);
    RUN_TEST(test_agent_lifecycle_with_enable_failures);
    RUN_TEST(test_agent_lifecycle_success_after_failures);
    RUN_TEST(test_multiple_agents_independent_state);
    RUN_TEST(test_agent_with_dynamic_enable_config);
    RUN_TEST(test_agent_retry_counter_persistence);

    // Edge case tests
    RUN_TEST(test_agent_enable_with_max_retry_boundary);
    RUN_TEST(test_agent_alternating_enable_disable_rapid);
    RUN_TEST(test_agent_enable_already_enabled_not_called_again);
    RUN_TEST(test_agent_disable_when_already_disabled);
    RUN_TEST(test_agent_loop_microsecond_precision);
}

void setup()
{
    UNITY_BEGIN();
    run_agent_tests();
    UNITY_END();

}

void loop() {}

int main()
{
    setup();
    return 0;
}