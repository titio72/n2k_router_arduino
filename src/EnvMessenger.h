#ifndef _ENV_MSG_H
#define _ENV_MSG_H

#include <Utils.h>
#include "Agents.hpp"

class EnvMessenger
{
public:
    EnvMessenger();
    ~EnvMessenger();

    AB_AGENT

private:
    bool enabled;
    unsigned long t0;
};


#endif
