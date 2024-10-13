#ifndef _BLEConf_H
#define _BLEConf_H

#include <stdlib.h>
#include <BTInterface.h>
#include "Context.h"

typedef void (*command_callback)(char command, const char* command_value);

class BLEConf: public ABBLEWriteCallback
{
public:
    BLEConf(Context ctx, command_callback callback = NULL);
    ~BLEConf();

    AB_AGENT

    void on_write(int handle, const char* value);

private:
    bool enabled;
    Context ctx;
    BTInterface ble;
    ulong last_sent;
    command_callback c_back;
};

#endif