#ifndef _BLEConf_H
#define _BLEConf_H

#include "Utils.h"
#include "Agents.hpp"
#include <BTInterface.h>

typedef void (*command_callback)(char command, const char* command_value);

class BLEConf: public ABBLEWriteCallback
{
public:
    BLEConf(command_callback callback, InternalBLEState* internalState = nullptr);
    ~BLEConf();

    AB_AGENT

    void on_write(int handle, const char* value);

    const char* get_device_name();

    ByteBuffer &get_services_buffer()
    {
      return services_buffer;
    }

    ByteBuffer get_field_value_buffer(int handle)
    {
      ByteBuffer buf = ble.get_field_value(handle);
      return buf;
    }

private:
    bool initialized;
    bool enabled;
    BTInterface ble;
    ulong last_sent;
    command_callback c_back;
    ByteBuffer services_buffer;
    int ble_settings_handle;
    int ble_conf_handle;
};

#endif