#ifndef _CONTEXT_H
#define _CONTEXT_H

#include "Utils.h"

class N2K_router;
class Configuration;

class Context
{
public:
    Context(N2K_router& _n2k, Configuration& _conf, data& _cache):
        n2k(_n2k), conf(_conf), cache(_cache) {}

    N2K_router& n2k;
    Configuration& conf;
    data& cache;
};

#endif