#include <unity.h>
#include <string.h>
#include <stdio.h>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include "DeferredEvents.h"
#include "Conf.h"
#include "Data.h"

void setUp(void) {}
void tearDown(void) {}

#define DEFERRED_CONTEXT \
    DeferredEvents events; \
    MockConfiguration conf; \
    MockEngineHours engineHours; \
    Data data;

// ---------------------------------------------------------------- CommandQueue

void test_queue_is_fifo(void)
{
    CommandQueue q;
    q.push('N', "one");
    q.push('B', "2");
    q.push('S', "1010");
    DeferredCommand c;
    TEST_ASSERT_TRUE(q.pop(c));
    TEST_ASSERT_EQUAL_CHAR('N', c.command);
    TEST_ASSERT_EQUAL_STRING("one", c.value);
    TEST_ASSERT_TRUE(q.pop(c));
    TEST_ASSERT_EQUAL_CHAR('B', c.command);
    TEST_ASSERT_TRUE(q.pop(c));
    TEST_ASSERT_EQUAL_STRING("1010", c.value);
    TEST_ASSERT_FALSE(q.pop(c));
}

void test_queue_rejects_when_full_and_counts_drops(void)
{
    CommandQueue q;
    for (int i = 0; i < DEFERRED_COMMAND_QUEUE_SIZE; i++)
        TEST_ASSERT_TRUE(q.push('B', "1"));
    TEST_ASSERT_FALSE(q.push('B', "1"));
    TEST_ASSERT_FALSE(q.push('B', "1"));
    TEST_ASSERT_EQUAL_UINT32(2, q.take_dropped());
    TEST_ASSERT_EQUAL_UINT32(0, q.take_dropped());

    DeferredCommand c;
    TEST_ASSERT_TRUE(q.pop(c));
    TEST_ASSERT_TRUE(q.push('B', "1")); // room again
}

void test_queue_wraps_around(void)
{
    CommandQueue q;
    DeferredCommand c;
    for (int round = 0; round < 5 * DEFERRED_COMMAND_QUEUE_SIZE; round++)
    {
        char v[8];
        snprintf(v, sizeof v, "%d", round);
        TEST_ASSERT_TRUE(q.push('B', v));
        TEST_ASSERT_TRUE(q.pop(c));
        TEST_ASSERT_EQUAL_STRING(v, c.value);
    }
}

void test_queue_truncates_long_value_and_handles_null(void)
{
    CommandQueue q;
    char big[300];
    memset(big, 'x', sizeof big);
    big[sizeof big - 1] = '\0';
    q.push('N', big);
    q.push('N', nullptr);
    DeferredCommand c;
    q.pop(c);
    TEST_ASSERT_EQUAL_INT(DEFERRED_COMMAND_VALUE_SIZE - 1, (int)strlen(c.value));
    q.pop(c);
    TEST_ASSERT_EQUAL_STRING("", c.value);
}

// Several threads posting at once (like the NimBLE task and others would) while the "loop task" drains.
// Every item must arrive intact and each producer's items must stay in order.
void test_queue_concurrent_producers(void)
{
    const int PRODUCERS = 3;
    const int PER_PRODUCER = 2000;
    CommandQueue q;
    std::atomic<bool> go{false};
    std::vector<std::thread> threads;
    for (int p = 0; p < PRODUCERS; p++)
    {
        threads.emplace_back([&, p]() {
            while (!go) {}
            for (int i = 0; i < PER_PRODUCER; i++)
            {
                char v[32];
                snprintf(v, sizeof v, "%d:%d", p, i);
                while (!q.push('B', v)) std::this_thread::yield(); // full: retry
            }
        });
    }

    int next[PRODUCERS] = {0};
    int received = 0, corrupt = 0, out_of_order = 0;
    // a broken queue loses items and would wait forever: give up after a deadline so the test fails instead
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    bool timed_out = false;
    go = true;
    while (received < PRODUCERS * PER_PRODUCER)
    {
        DeferredCommand c;
        if (!q.pop(c))
        {
            if (std::chrono::steady_clock::now() > deadline)
            {
                timed_out = true;
                break;
            }
            std::this_thread::yield();
            continue;
        }
        int p = -1, i = -1;
        if (c.command != 'B' || sscanf(c.value, "%d:%d", &p, &i) != 2 || p < 0 || p >= PRODUCERS)
        {
            corrupt++;
        }
        else
        {
            if (i != next[p]) out_of_order++;
            next[p] = i + 1;
        }
        received++;
    }
    // on timeout the producers may be stuck retrying a full queue: keep draining so they can finish
    std::atomic<bool> joined{false};
    std::thread drainer([&]() {
        DeferredCommand c;
        while (timed_out && !joined) q.pop(c);
    });
    for (auto &t : threads) t.join();
    joined = true;
    drainer.join();

    TEST_ASSERT_FALSE_MESSAGE(timed_out, "items were lost");
    TEST_ASSERT_EQUAL_INT(0, corrupt);
    TEST_ASSERT_EQUAL_INT(0, out_of_order);
    for (int p = 0; p < PRODUCERS; p++)
        TEST_ASSERT_EQUAL_INT(PER_PRODUCER, next[p]);
}

// ---------------------------------------------------------------- commands are applied on the consumer side only

void test_commands_are_not_applied_until_processed(void)
{
    DEFERRED_CONTEXT
    conf.save_device_name("before");
    conf.reset_call_counts();

    events.post_command('N', "after");
    events.post_command('B', "400");
    TEST_ASSERT_EQUAL_STRING("before", conf.get_device_name()); // posting touches nothing
    TEST_ASSERT_EQUAL_INT(0, conf.save_device_name_calls);

    TEST_ASSERT_EQUAL_INT(2, process_deferred_commands(events, conf, engineHours, data));
    TEST_ASSERT_EQUAL_STRING("after", conf.get_device_name());
    TEST_ASSERT_EQUAL_UINT16(400, conf.get_batter_capacity());
    TEST_ASSERT_EQUAL_INT(0, process_deferred_commands(events, conf, engineHours, data));
}

void test_commands_are_applied_in_order(void)
{
    DEFERRED_CONTEXT
    events.post_command('B', "100");
    events.post_command('B', "200");
    events.post_command('B', "300");
    process_deferred_commands(events, conf, engineHours, data);
    TEST_ASSERT_EQUAL_UINT16(300, conf.get_batter_capacity()); // last one wins
}

void test_engine_hours_command_goes_through_the_queue(void)
{
    DEFERRED_CONTEXT
    events.post_command('H', "3600"); // 1 hour
    TEST_ASSERT_EQUAL_UINT64(0, engineHours.get_engine_hours());
    process_deferred_commands(events, conf, engineHours, data);
    TEST_ASSERT_EQUAL_UINT64(3600000, engineHours.get_engine_hours());
    TEST_ASSERT_EQUAL_UINT64(3600000, data.engine.engine_time);
}

void test_full_queue_drops_extra_commands_without_blocking(void)
{
    DEFERRED_CONTEXT
    bool ok = true;
    for (int i = 0; i < DEFERRED_COMMAND_QUEUE_SIZE + 3; i++)
        ok = events.post_command('B', "10") && ok;
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL_INT(DEFERRED_COMMAND_QUEUE_SIZE, process_deferred_commands(events, conf, engineHours, data));
}

// ---------------------------------------------------------------- N2K source claim

void test_source_claim_is_saved_when_not_pinned(void)
{
    DEFERRED_CONTEXT
    conf.reset_call_counts();
    events.post_source_claim(22, 31);
    TEST_ASSERT_EQUAL_INT(0, conf.save_n2k_source_calls); // nothing happens on the posting task
    TEST_ASSERT_TRUE(process_deferred_source_claim(events, conf));
    TEST_ASSERT_EQUAL_INT(1, conf.save_n2k_source_calls);
    TEST_ASSERT_EQUAL_UINT8(31, conf.get_n2k_source());
    TEST_ASSERT_FALSE(process_deferred_source_claim(events, conf)); // consumed
}

void test_source_claim_ignored_when_pinned(void)
{
    DEFERRED_CONTEXT
    N2KServices svc = conf.get_services();
    svc.set_keep_n2k_src(true);
    conf.save_services(svc);
    conf.reset_call_counts();
    unsigned char before = conf.get_n2k_source();

    events.post_source_claim(22, 31);
    TEST_ASSERT_TRUE(process_deferred_source_claim(events, conf));
    TEST_ASSERT_EQUAL_INT(0, conf.save_n2k_source_calls);
    TEST_ASSERT_EQUAL_UINT8(before, conf.get_n2k_source());
}

void test_only_latest_source_claim_is_kept(void)
{
    DEFERRED_CONTEXT
    events.post_source_claim(22, 30);
    events.post_source_claim(30, 44);
    unsigned char o = 0, n = 0;
    TEST_ASSERT_TRUE(events.take_source_claim(o, n));
    TEST_ASSERT_EQUAL_UINT8(30, o);
    TEST_ASSERT_EQUAL_UINT8(44, n);
    TEST_ASSERT_FALSE(events.take_source_claim(o, n));
}

void test_source_claim_of_address_zero_is_not_lost(void)
{
    // the "nothing pending" marker must not collide with a real (0, 0) claim
    DeferredEvents events;
    events.post_source_claim(0, 0);
    unsigned char o = 9, n = 9;
    TEST_ASSERT_TRUE(events.take_source_claim(o, n));
    TEST_ASSERT_EQUAL_UINT8(0, n);
}

// ---------------------------------------------------------------- N2K activity (LED)

void test_n2k_activity_bits(void)
{
    DeferredEvents events;
    TEST_ASSERT_EQUAL_UINT32(0, events.take_n2k_activity());
    events.post_n2k_activity(true);
    TEST_ASSERT_EQUAL_UINT32(DeferredEvents::ACTIVITY_SENT, events.take_n2k_activity());
    events.post_n2k_activity(true);
    events.post_n2k_activity(false);
    events.post_n2k_activity(true);
    TEST_ASSERT_EQUAL_UINT32(DeferredEvents::ACTIVITY_SENT | DeferredEvents::ACTIVITY_FAILED, events.take_n2k_activity());
    TEST_ASSERT_EQUAL_UINT32(0, events.take_n2k_activity());
}

void test_concurrent_posts_from_other_tasks_while_loop_drains(void)
{
    // N2K-task style events (claims + activity) hammered from a thread while the "loop task" consumes
    DeferredEvents events;
    std::atomic<bool> stop{false};
    std::thread n2k([&]() {
        unsigned char s = 0;
        while (!stop)
        {
            events.post_source_claim(s, (unsigned char)(s + 1));
            events.post_n2k_activity((s & 1) == 0);
            s++;
        }
    });
    int claims = 0;
    for (int i = 0; i < 20000; i++)
    {
        unsigned char o, nn;
        if (events.take_source_claim(o, nn))
        {
            claims++;
            TEST_ASSERT_EQUAL_UINT8((unsigned char)(o + 1), nn); // never a torn old/new pair
        }
        events.take_n2k_activity();
    }
    stop = true;
    n2k.join();
    TEST_ASSERT_GREATER_THAN(0, claims);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_queue_is_fifo);
    RUN_TEST(test_queue_rejects_when_full_and_counts_drops);
    RUN_TEST(test_queue_wraps_around);
    RUN_TEST(test_queue_truncates_long_value_and_handles_null);
    RUN_TEST(test_queue_concurrent_producers);
    RUN_TEST(test_commands_are_not_applied_until_processed);
    RUN_TEST(test_commands_are_applied_in_order);
    RUN_TEST(test_engine_hours_command_goes_through_the_queue);
    RUN_TEST(test_full_queue_drops_extra_commands_without_blocking);
    RUN_TEST(test_source_claim_is_saved_when_not_pinned);
    RUN_TEST(test_source_claim_ignored_when_pinned);
    RUN_TEST(test_only_latest_source_claim_is_kept);
    RUN_TEST(test_source_claim_of_address_zero_is_not_lost);
    RUN_TEST(test_n2k_activity_bits);
    RUN_TEST(test_concurrent_posts_from_other_tasks_while_loop_drains);
    return UNITY_END();
}
