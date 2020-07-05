#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string.h>
#include "N2K.h"

class WEBServer {

public:
    void setup(data* cache, configuration* conf, statistics* stats, N2K* n2k);
    void on_loop(unsigned long ms);

private:
    data* cache;
    configuration* conf;
    statistics* stats;
    N2K* n2k;
    bool started;

    void handle_client(WiFiClient* serving_client, unsigned long ms);

};



#endif