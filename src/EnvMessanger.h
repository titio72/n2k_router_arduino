#ifndef _ENV_MSG_H
#define _ENV_MSG_H

#include <Utils.h>
#include "Context.h"

class EnvMessanger
{
public:
    EnvMessanger(Context& ctx);
    ~EnvMessanger();

    AB_AGENT

private:
    bool enabled;
    Context& ctx;
    unsigned long t0;
};


#endif
