#ifndef _BLEConf_H
#define _BLEConf_H

#include "Utils.h"
#include "Context.h"
#include <BTInterface.h>

typedef void (*command_callback)(char command, const char* command_value);

class BLEConf: public ABBLEWriteCallback
{
public:
    BLEConf(Context ctx, command_callback callback);
    ~BLEConf();

    AB_AGENT

    void on_write(int handle, const char* value);

    void set_device_name(const char* name);

private:
    bool enabled;
    Context ctx;
    BTInterface ble;
    ulong last_sent;
    command_callback c_back;
};

#endif