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



template <typename T>
bool handle_agent_enable(T &agent, bool enable, Context &ctx, unsigned short *retry, const char *desc = "")
{
  if (!agent.is_enabled())
  {
      if (retry == NULL || (*retry) < MAX_RETRY)
      {
      agent.enable(ctx);
      if (agent.is_enabled())
      {
        if (retry)
          (*retry) = 0;
      }
      else
      {
        if (retry)
        {
          (*retry)++;
          if ((*retry) >= MAX_RETRY)
            Log::tracex(APP_LOG_TAG, "Exceeded enable retry", "Module {%s}", desc);
        }
      }
    }
  }
  return agent.is_enabled();
}

template <typename T>
unsigned long handle_agent_loop_simple(T &agent, Context &ctx, bool enable, unsigned short *retry, unsigned long now_micros, const char *desc = "")
{
  unsigned long t = NOW_MICROS;
  if (enable)
  {
    agent.loop(now_micros, ctx);
  }
  return NOW_MICROS - t;
}

template <typename T>
unsigned long handle_agent_loop(T &agent, Context &ctx, bool enable, unsigned short *retry, unsigned long now_micros, const char *desc = "")
{
  unsigned long t = NOW_MICROS;
  if (enable)
  {
    if (handle_agent_enable(agent, enable, ctx, retry, desc))
    {
      agent.loop(now_micros, ctx);
    }
  }
  else
  {
    agent.disable(ctx);
    if (retry)
    {
      (*retry) = 0;
    }
  }
  return NOW_MICROS - t;
}



#endif