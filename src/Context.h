#ifndef _CONTEXT_H
#define _CONTEXT_H

#include "Data.h"

class N2KSender;
class Configuration;
class Data;

class Context
{
public:
    Context(N2KSender& _n2k, Configuration& _conf, Data& _cache):
        n2k(_n2k), conf(_conf), data_cache(_cache) {}


    Configuration& conf;
    Data& data_cache;
    N2KSender& n2k;
};


#define MOCK_CONTEXT \
Data data; \
NullN2KSender n2kSender; \
MockConfiguration mockConf; \
Context context = {n2kSender, mockConf, data};

#endif