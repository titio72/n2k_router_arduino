#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string.h>

class WEBServer {

public:
    void setup(data* cache, configuration* conf);
    void on_loop(unsigned long ms);
private:
    data* cache;
    configuration* conf;

    bool started;

    void handle_client(WiFiClient* serving_client, unsigned long ms);

};



#endif