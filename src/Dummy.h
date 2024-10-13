#ifndef _Dummy_H
#define _Dummy_H

#include <Utils.h>

class Dummy
{
public:
    Dummy();

    AB_AGENT

    void dumpStats() {}

private:
    bool enabled;
};


#endif
