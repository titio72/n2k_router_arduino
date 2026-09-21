#ifndef _DEFERRED_EVENTS_H
#define _DEFERRED_EVENTS_H

/**
 * Hand-off from the tasks that must not touch shared state to the main loop task.
 *
 * Threading rule: Configuration, EngineHours and the Data cache are only ever read and written by the
 * main loop task. Callbacks that fire on other tasks (BLE writes on the NimBLE host task, source claims and
 * "message sent" on the N2K task) only *post* to a DeferredEvents; _loop() drains it. That keeps flash writes
 * (EEPROM/NVS) out of the BLE and N2K tasks and needs no locking around the getters/setters.
 *
 * post_* may be called from any task. take_*, pop_command and the process_* helpers are for the loop task only.
 */

#include <atomic>
#include <stdint.h>
#include <string.h>
#include <Log.h>
#include "Conf.h"
#include "Data.h"
#include "CommandHandler.hpp"

#ifdef NATIVE
#include <mutex>
class DeferredLock
{
public:
    void lock() { m.lock(); }
    void unlock() { m.unlock(); }

private:
    std::mutex m;
};
#else
#include <Arduino.h>
// The critical sections below only copy a few dozen bytes, so a spinlock (interrupts off on this core) is
// cheaper than a mutex and safe to take from any task.
class DeferredLock
{
public:
    void lock() { portENTER_CRITICAL(&mux); }
    void unlock() { portEXIT_CRITICAL(&mux); }

private:
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
};
#endif

#define DEFERRED_COMMAND_VALUE_SIZE 64 // longest command payload; longer ones are truncated
#define DEFERRED_COMMAND_QUEUE_SIZE 8

struct DeferredCommand
{
    char command;
    char value[DEFERRED_COMMAND_VALUE_SIZE];
};

/** Fixed-size FIFO of BLE commands, safe for many producers and one consumer. */
class CommandQueue
{
public:
    /** @return false (and counts a drop) if the queue is full */
    bool push(char command, const char *value)
    {
        lock.lock();
        bool ok = count < DEFERRED_COMMAND_QUEUE_SIZE;
        if (ok)
        {
            DeferredCommand &c = items[(head + count) % DEFERRED_COMMAND_QUEUE_SIZE];
            c.command = command;
            strncpy(c.value, value ? value : "", DEFERRED_COMMAND_VALUE_SIZE - 1);
            c.value[DEFERRED_COMMAND_VALUE_SIZE - 1] = '\0';
            count++;
        }
        lock.unlock();
        if (!ok)
            dropped++;
        return ok;
    }

    bool pop(DeferredCommand &out)
    {
        lock.lock();
        bool ok = count > 0;
        if (ok)
        {
            out = items[head];
            head = (head + 1) % DEFERRED_COMMAND_QUEUE_SIZE;
            count--;
        }
        lock.unlock();
        return ok;
    }

    /** number of commands rejected because the queue was full since the last call */
    unsigned int take_dropped() { return dropped.exchange(0); }

private:
    DeferredLock lock;
    DeferredCommand items[DEFERRED_COMMAND_QUEUE_SIZE];
    int head = 0;
    int count = 0;
    std::atomic<unsigned int> dropped{0};
};

class DeferredEvents
{
public:
    // ---- producers: any task ----

    bool post_command(char command, const char *value) { return commands.push(command, value); }

    /** N2K task: the bus assigned us another source address. Only the latest claim is kept. */
    void post_source_claim(unsigned char old_source, unsigned char new_source)
    {
        source_claim.store(((int)old_source << 8) | new_source);
    }

    /** N2K task: a message went out on the bus (or failed to). */
    void post_n2k_activity(bool success) { n2k_activity.fetch_or(success ? ACTIVITY_SENT : ACTIVITY_FAILED); }

    // ---- consumer: loop task only ----

    bool pop_command(DeferredCommand &out) { return commands.pop(out); }
    unsigned int take_dropped_commands() { return commands.take_dropped(); }

    bool take_source_claim(unsigned char &old_source, unsigned char &new_source)
    {
        int v = source_claim.exchange(NO_CLAIM);
        if (v == NO_CLAIM)
            return false;
        old_source = (v >> 8) & 0xFF;
        new_source = v & 0xFF;
        return true;
    }

    /** @return a bit mask of ACTIVITY_SENT / ACTIVITY_FAILED seen since the last call */
    unsigned int take_n2k_activity() { return n2k_activity.exchange(0); }

    static const unsigned int ACTIVITY_SENT = 1;
    static const unsigned int ACTIVITY_FAILED = 2;

private:
    static const int NO_CLAIM = -1;
    CommandQueue commands;
    std::atomic<int> source_claim{NO_CLAIM};
    std::atomic<unsigned int> n2k_activity{0};
};

/** Loop task: run every queued BLE command. @return the number of commands executed. */
inline int process_deferred_commands(DeferredEvents &events, Configuration &conf, EngineHours &engineHours, Data &data)
{
    int n = 0;
    DeferredCommand c;
    while (events.pop_command(c))
    {
        CommandHandler::on_command(c.command, c.value, conf, engineHours, data);
        n++;
    }
    unsigned int dropped = events.take_dropped_commands();
    if (dropped)
    {
        Log::tracex(CMD_LOG_TAG, "Commands dropped", "Count {%u} (queue full)", dropped);
    }
    return n;
}

/** Loop task: persist a newly claimed N2K source address (unless the user pinned it). */
inline bool process_deferred_source_claim(DeferredEvents &events, Configuration &conf)
{
    unsigned char old_source, new_source;
    if (!events.take_source_claim(old_source, new_source))
        return false;
    bool keep = conf.get_services().is_keep_n2k_src();
    if (!keep)
    {
        conf.save_n2k_source(new_source);
    }
    Log::tracex("APP", "New claimed n2k source", " New Source {%d} Old Source {%d} Save {%d}",
                new_source, old_source, keep ? 0 : 1);
    return true;
}

#endif
