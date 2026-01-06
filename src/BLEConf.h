#ifndef _BLEConf_H
#define _BLEConf_H

#include "Utils.h"
#include "Agents.hpp"
#include <BTInterface.h>

#define BUFFER_OFFSET_VERSION      0  // version
#define BUFFER_OFFSET_GPS_FIX      1
#define BUFFER_OFFSET_ATMO         2
#define BUFFER_OFFSET_TEMP         6
#define BUFFER_OFFSET_HUM          8
#define BUFFER_OFFSET_LAT          10
#define BUFFER_OFFSET_LON          14
#define BUFFER_OFFSET_MEM          18
#define BUFFER_OFFSET_CANBUS       22
#define BUFFER_OFFSET_CANBUS_S     23
#define BUFFER_OFFSET_CANBUS_E     27
#define BUFFER_OFFSET_SOG          31
#define BUFFER_OFFSET_COG          33
#define BUFFER_OFFSET_RPM          35
#define BUFFER_OFFSET_ENGINE_TIME  37
#define BUFFER_OFFSET_TIMESTAMP    41
#define BUFFER_OFFSET_SERVICES     45
#define BUFFER_OFFSET_RPM_ADJ      47
#define BUFFER_OFFSET_CURRENT      51
#define BUFFER_OFFSET_VOLTAGE      53
#define BUFFER_OFFSET_SOC          55
#define BUFFER_OFFSET_N2K_SOURCE   57
#define BUFFER_OFFSET_STW          58
#define BUFFER_OFFSET_WATER_TEMP   60
#define BUFFER_OFFSET_STW_ADJ      62
#define BUFFER_OFFSET_SEA_TEMP_ADJ 66

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
    unsigned long last_activity;
};

#endif