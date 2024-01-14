#ifndef _NETWORK_H
#define _NETWORK_H

#include "Context.h"

class WEBServer;

class NetworkHub {

public:
    NetworkHub(Context context);
    ~NetworkHub();

    AB_AGENT

    void close();

private:
    Context ctx;
    bool enabled;
    WEBServer* web_ui;
};



#endif // _NETWORK_H