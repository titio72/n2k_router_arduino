#ifndef _AGENTS_HPP
#define _AGENTS_HPP
#include <Log.h>
#include "Context.h"
#include "Constants.h"

#define AB_AGENT \
void enable(); \
void disable(); \
bool is_enabled(); \
void loop(unsigned long time, Context &ctx ); \
void setup(Context &ctx); \



template <typename T>
bool handle_agent_enable(T &agent, bool enable, unsigned short *retry, const char *desc = "")
{
  if (!agent.is_enabled())
  {
    if (retry == NULL || (*retry) < MAX_RETRY)
    {
      agent.enable();
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
void handle_agent_loop(T &agent, Context &ctx, bool enable, unsigned short *retry, unsigned long micros, const char *desc = "")
{
  if (enable)
  {
    if (handle_agent_enable(agent, enable, retry, desc))
    {
      agent.loop(micros, ctx);
    }
  }
  else
  {
    agent.disable();
    if (retry)
    {
      (*retry) = 0;
    }
  }
}



#endif