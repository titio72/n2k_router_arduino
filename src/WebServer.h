#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string.h>
#include "N2K.h"
#include "Conf.h"

class WEBServer {

public:
    void setup(data* cache, Configuration* conf, statistics* stats, N2K* n2k);
    void loop(unsigned long ms);

private:
    data* cache;
    Configuration* conf;
    statistics* stats;
    N2K* n2k;
    bool started;

    void handle_client(WiFiClient* serving_client, unsigned long ms);

};



#endif