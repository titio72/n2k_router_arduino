#ifndef _AGENTS_HPP
#define _AGENTS_HPP
#include <Log.h>
#include "Context.h"
#include "Constants.h"

#ifdef NATIVE
#define NOW_MICROS 0
#else
#include <Arduino.h>
#define NOW_MICROS micros()
#endif



#define AB_AGENT \
void enable(Context &ctx); \
void disable(Context &ctx); \
bool is_enabled(); \
void loop(unsigned long time, Context &ctx ); \
void setup(Context &ctx); \



/**
 * Try to enable an agent that is not enabled yet.
 *
 * After MAX_RETRY consecutive failures the quick retries stop. If retry_at is given, a single new attempt is
 * made every AGENT_RETRY_COOLDOWN_USEC afterwards (a sensor that was still warming up, or a transient I2C
 * error at boot, would otherwise leave the agent off until a reboot or a config toggle).
 */
template <typename T>
bool handle_agent_enable(T &agent, bool enable, Context &ctx, unsigned short *retry, const char *desc = "",
                         unsigned long now_micros = 0, unsigned long *retry_at = NULL)
{
  if (!agent.is_enabled())
  {
    if (retry && retry_at && (*retry) >= MAX_RETRY && (*retry_at) != 0 && (long)(now_micros - (*retry_at)) >= 0)
    {
      (*retry) = MAX_RETRY - 1; // one more attempt
    }

    if (retry == NULL || (*retry) < MAX_RETRY)
    {
      agent.enable(ctx);
      if (agent.is_enabled())
      {
        if (retry)
          (*retry) = 0;
        if (retry_at)
          (*retry_at) = 0;
      }
      else
      {
        if (retry)
        {
          (*retry)++;
          if ((*retry) >= MAX_RETRY)
          {
            if (retry_at)
            {
              (*retry_at) = now_micros + AGENT_RETRY_COOLDOWN_USEC;
              if ((*retry_at) == 0) (*retry_at) = 1; // 0 means "not scheduled"
            }
            Log::tracex(APP_LOG_TAG, "Exceeded enable retry", "Module {%s}", desc);
          }
        }
      }
    }
  }
  return agent.is_enabled();
}

template <typename T>
unsigned long handle_agent_loop(T &agent, Context &ctx, bool enable, unsigned short *retry, unsigned long now_micros,
                                const char *desc = "", unsigned long *retry_at = NULL)
{
  unsigned long t = NOW_MICROS;
  if (enable)
  {
    if (handle_agent_enable(agent, enable, ctx, retry, desc, now_micros, retry_at))
    {
      agent.loop(now_micros, ctx);
    }
  }
  else if (agent.is_enabled())
  {
    agent.disable(ctx);
    if (retry)
    {
      (*retry) = 0;
    }
    if (retry_at)
    {
      (*retry_at) = 0;
    }
  }
  return NOW_MICROS - t;
}



#endif