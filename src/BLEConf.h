#ifndef _BLEConf_H
#define _BLEConf_H

#include "Utils.h"
#include "Agents.hpp"
#include <BTInterface.h>

typedef void (*command_callback)(char command, const char* command_value);

class BLEConf: public ABBLEWriteCallback
{
public:
    BLEConf(command_callback callback);
    ~BLEConf();

    AB_AGENT

    void on_write(int handle, const char* value);

private:
    bool enabled;
    BTInterface ble;
    ulong last_sent;
    command_callback c_back;
    char device_name[16] = {0};
};

#endif