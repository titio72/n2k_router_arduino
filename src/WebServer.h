#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "Context.h"

class WEBServer {

public:
    WEBServer(Context ctx);

    AB_AGENT

private:
    Context ctx;
    bool started;
};



#endif