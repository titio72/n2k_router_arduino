#ifndef _BLEConf_H
#define _BLEConf_H

#include <stdlib.h>
#include <BTInterface.h>
#include "Context.h"

class BLEConf: public ABBLEWriteCallback
{
public:
    BLEConf(Context ctx);
    ~BLEConf();

    AB_AGENT

    void on_write(int handle, const char* value);

private:
    bool enabled;
    Context ctx;
    BTInterface ble;
    ulong last_sent;
};

#endif