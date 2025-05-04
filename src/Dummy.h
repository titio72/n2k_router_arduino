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

class DummyTachometer: public Dummy
{
public:
    DummyTachometer(): Dummy() {}

    void set_engine_time(uint64_t t, bool save) {}
    void set_adjustment(double adj, bool save) {}
    void calibrate(int rpm) {}
};
#endif
