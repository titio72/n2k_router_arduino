#ifndef _CONTEXT_H
#define _CONTEXT_H

#include "Data.h"

class N2K_router;
class Configuration;
class Data;

class Context
{
public:
    Context(N2K_router& _n2k, Configuration& _conf, Data& _cache):
        n2k(_n2k), conf(_conf), cache(_cache) {}

    N2K_router& n2k;
    Configuration& conf;
    Data& cache;
};

#endif