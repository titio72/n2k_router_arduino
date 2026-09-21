#include <unity.h>
#include "ResetButton.h"
#include "Conf.h"
#include "Context.h"
#include "Data.h"
#include "N2K_router.h"

#define HOLD 2000000UL

static bool button_down = false;
static int restart_calls = 0;

static bool read_button(int pin) { return button_down; }
static void mock_restart() { restart_calls++; }

void setUp(void)
{
    button_down = false;
    restart_calls = 0;
}

void tearDown(void) {}

#define RESET_TEST_SETUP \
    MOCK_CONTEXT \
    ResetButton btn(5, HOLD, read_button, mock_restart);

void test_disabled_by_default(void)
{
    RESET_TEST_SETUP
    TEST_ASSERT_FALSE(btn.is_enabled());
    button_down = true;
    btn.loop(0, context);
    btn.loop(HOLD * 2, context);
    TEST_ASSERT_EQUAL_INT(0, restart_calls);
}

void test_no_pin_cannot_be_enabled(void)
{
    MOCK_CONTEXT
    ResetButton btn(-1, HOLD, read_button, mock_restart);
    btn.enable(context);
    TEST_ASSERT_FALSE(btn.is_enabled());
}

void test_enable_disable(void)
{
    RESET_TEST_SETUP
    btn.enable(context);
    TEST_ASSERT_TRUE(btn.is_enabled());
    btn.disable(context);
    TEST_ASSERT_FALSE(btn.is_enabled());
}

void test_no_restart_when_not_pressed(void)
{
    RESET_TEST_SETUP
    btn.enable(context);
    for (unsigned long t = 0; t < HOLD * 3; t += 100000)
        btn.loop(t, context);
    TEST_ASSERT_EQUAL_INT(0, restart_calls);
}

void test_restart_after_hold(void)
{
    RESET_TEST_SETUP
    btn.enable(context);
    btn.loop(0, context); // released
    button_down = true;
    btn.loop(1000000, context); // press starts
    btn.loop(1000000 + HOLD - 1, context);
    TEST_ASSERT_EQUAL_INT(0, restart_calls);
    btn.loop(1000000 + HOLD, context);
    TEST_ASSERT_EQUAL_INT(1, restart_calls);
}

void test_short_press_does_not_restart(void)
{
    RESET_TEST_SETUP
    btn.enable(context);
    btn.loop(0, context);
    button_down = true;
    btn.loop(1000000, context);
    btn.loop(1000000 + HOLD / 2, context);
    button_down = false;
    btn.loop(1000000 + HOLD / 2 + 1000, context);
    TEST_ASSERT_EQUAL_INT(0, restart_calls);
}

void test_hold_timer_restarts_after_release(void)
{
    RESET_TEST_SETUP
    btn.enable(context);
    btn.loop(0, context);
    // two short presses must not add up
    for (int i = 0; i < 2; i++)
    {
        unsigned long t0 = 1000000UL * (i + 1) * 10;
        button_down = true;
        btn.loop(t0, context);
        btn.loop(t0 + HOLD - 100000, context);
        button_down = false;
        btn.loop(t0 + HOLD - 50000, context);
    }
    TEST_ASSERT_EQUAL_INT(0, restart_calls);
}

void test_button_held_at_enable_is_ignored_until_released(void)
{
    RESET_TEST_SETUP
    button_down = true;
    btn.enable(context);
    btn.loop(0, context);
    btn.loop(HOLD * 5, context);
    TEST_ASSERT_EQUAL_INT(0, restart_calls);

    button_down = false;
    btn.loop(HOLD * 6, context);
    button_down = true;
    btn.loop(HOLD * 7, context);
    btn.loop(HOLD * 8, context);
    TEST_ASSERT_EQUAL_INT(1, restart_calls);
}

void test_restart_fires_once_per_press(void)
{
    RESET_TEST_SETUP
    btn.enable(context);
    btn.loop(0, context);
    button_down = true;
    btn.loop(1000, context);
    btn.loop(1000 + HOLD, context);
    btn.loop(1000 + HOLD + 1000, context);
    btn.loop(1000 + HOLD * 4, context);
    TEST_ASSERT_EQUAL_INT(1, restart_calls);
}

void test_timer_rollover(void)
{
    RESET_TEST_SETUP
    btn.enable(context);
    btn.loop(0, context);
    button_down = true;
    unsigned long start = (unsigned long)(-1) - 1000; // micros() about to wrap
    btn.loop(start, context);
    btn.loop(start + HOLD, context); // wrapped
    TEST_ASSERT_EQUAL_INT(1, restart_calls);
}

static int before_restart_calls = 0;
static int restarts_seen_by_callback = -1;
static void before_restart_cb()
{
    before_restart_calls++;
    restarts_seen_by_callback = restart_calls; // must still be 0: the callback runs first
}

void test_before_restart_callback_runs_before_the_restart(void)
{
    before_restart_calls = 0;
    restarts_seen_by_callback = -1;
    RESET_TEST_SETUP
    btn.set_before_restart(before_restart_cb);
    btn.enable(context);
    btn.loop(0, context);
    button_down = true;
    btn.loop(1000, context);
    btn.loop(1000 + HOLD, context);
    TEST_ASSERT_EQUAL_INT(1, before_restart_calls);
    TEST_ASSERT_EQUAL_INT(0, restarts_seen_by_callback);
    TEST_ASSERT_EQUAL_INT(1, restart_calls);
}

void test_before_restart_callback_not_called_without_a_full_press(void)
{
    before_restart_calls = 0;
    RESET_TEST_SETUP
    btn.set_before_restart(before_restart_cb);
    btn.enable(context);
    btn.loop(0, context);
    button_down = true;
    btn.loop(1000, context);
    btn.loop(1000 + HOLD / 2, context);
    TEST_ASSERT_EQUAL_INT(0, before_restart_calls);
    TEST_ASSERT_EQUAL_INT(0, restart_calls);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_disabled_by_default);
    RUN_TEST(test_no_pin_cannot_be_enabled);
    RUN_TEST(test_enable_disable);
    RUN_TEST(test_no_restart_when_not_pressed);
    RUN_TEST(test_restart_after_hold);
    RUN_TEST(test_short_press_does_not_restart);
    RUN_TEST(test_hold_timer_restarts_after_release);
    RUN_TEST(test_button_held_at_enable_is_ignored_until_released);
    RUN_TEST(test_restart_fires_once_per_press);
    RUN_TEST(test_timer_rollover);
    RUN_TEST(test_before_restart_callback_runs_before_the_restart);
    RUN_TEST(test_before_restart_callback_not_called_without_a_full_press);
    return UNITY_END();
}
