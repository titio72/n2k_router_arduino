#ifndef _CONTEXT_H
#define _CONTEXT_H

#include "Utils.h"

class N2K;
class Configuration;

class Context
{
public:
    Context(N2K& _n2k, Configuration& _conf, statistics& _stats, data& _cache):
        n2k(_n2k), conf(_conf), stats(_stats), cache(_cache) {}

    N2K& n2k;
    Configuration& conf;
    statistics& stats;
    data& cache;
};

#endif