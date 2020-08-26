#ifndef _NETWORK_H
#define _NETWORK_H

#include "Utils.h"
#include "N2K.h"
#include "Conf.h"

class NetworkHub {

public:
    NetworkHub(unsigned int p, const char* d, data* cache, Configuration* conf, statistics* stats, N2K* n2k);
    ~NetworkHub();

    bool begin();

    bool end();

    bool send_udp(const char* message, unsigned int len);

    void loop(unsigned long t);
private:

    unsigned int port;
    const char* destination;
};



#endif // _NETWORK_H