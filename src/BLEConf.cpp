#include "BLEConf.h"
#include "N2K_router.h"
#include "Utils.h"
#include "Conf.h"
#include <Log.h>
#include "DHTesp.h"

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CONF_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define DATA_UUID "55da66c7-801f-498d-b652-c57cb3f1b590"
#define COMMAND_UUID "68ad1094-0989-4e22-9f21-4df7ef390803"

#define MAX_DATA 7

#define BLE_LOG_TAG "BLE"

#define BLE_UPDATE_PERIOD 1000000

BLEConf::BLEConf(Context _ctx, command_callback cback)
    : enabled(false), ctx(_ctx), ble(SERVICE_UUID, nullptr), last_sent(0), c_back(cback)
{
}

BLEConf::~BLEConf()
{
}

void BLEConf::on_write(int handle, const char *value)
{
  Log::tracex(BLE_LOG_TAG, "Command", "Handle {%d} Command {%s}", handle, value);
  if (handle == 0)
  {
    N2KServices c = ctx.conf.get_services();
    c.from_string(value);
    ctx.conf.save_services(c);
  }
  else if (handle == 1)
  {
    const char command = value[0];
    const char *command_value = (value + sizeof(char));
    if (c_back) c_back(command, command_value);
  }
}

void BLEConf::setup()
{
  Log::tracex(BLE_LOG_TAG, "Setup", "Start BLE");
  ble.set_device_name(ctx.conf.get_device_name());
  N2KServices &c = ctx.conf.get_services();
  ble.add_field("data", DATA_UUID);
  ble.add_setting("conf", CONF_UUID);
  ble.add_setting("command", COMMAND_UUID);
  ble.set_write_callback(this);
  ble.setup();
  ble.set_setting_value(0, c.to_string());
  ble.begin();
  Log::tracex(BLE_LOG_TAG, "Setup", "Conf {%s}", c.to_string());
}

void add4Int(uint8_t *dest, int &offset, int32_t data32)
{
  uint8_t *temp = dest + offset;
  temp[0] = data32;
  temp[1] = data32 >> 8;
  temp[2] = data32 >> 16;
  temp[3] = data32 >> 24;
  offset += 4;
}

void add2Int(uint8_t *dest, int &offset, int16_t data16)
{
  uint8_t *temp = dest + offset;
  temp[0] = data16;
  temp[1] = data16 >> 8;
  offset += 2;
}

void add1Int(uint8_t *dest, int &offset, int8_t data8)
{
  uint8_t *temp = dest + offset;
  temp[0] = data8;
  offset += 1;
}

#define INVALID_32 0xFFFFFF7F
#define INVALID_U32 0xFFFFFFFF
#define INVALID_16 0xFF7F
#define INVALID_U16 0xFFFF

void BLEConf::loop(unsigned long ms)
{
  if (enabled && check_elapsed(ms, last_sent, BLE_UPDATE_PERIOD))
  {
    N2KStats s(ctx.n2k.get_bus().getStats());

    const int8_t _gpsFix = ctx.cache.gsa.fix;
    const int32_t _atmo = isnan(ctx.cache.pressure) ? INVALID_U32 : (int32_t)(ctx.cache.pressure * 10.0);
    const int16_t _temp = isnan(ctx.cache.temperature) ? INVALID_16 : (int16_t)(ctx.cache.temperature * 10);
    const int16_t _hum = isnan(ctx.cache.humidity) ? INVALID_U16 : (int16_t)(ctx.cache.humidity * 100);
    const int32_t _lat = isnan(ctx.cache.rmc.lat) ? INVALID_32 : (int32_t)(ctx.cache.rmc.lat * 1000000);
    const int32_t _lon = isnan(ctx.cache.rmc.lon) ? INVALID_32 : (int32_t)(ctx.cache.rmc.lon * 1000000);
    const int16_t _sog = isnan(ctx.cache.rmc.sog) ? INVALID_U32 : (int16_t)(ctx.cache.rmc.sog * 100);
    const int16_t _cog = isnan(ctx.cache.rmc.cog) ? INVALID_U32 : (int16_t)(ctx.cache.rmc.cog * 10);
    const int16_t _current = isnan(ctx.cache.current) ? INVALID_16 : (int16_t)(ctx.cache.current * 100);
    const int16_t _voltage = isnan(ctx.cache.voltage) ? INVALID_16 : (int16_t)(ctx.cache.voltage * 100);
    const int16_t _soc = isnan(ctx.cache.soc) ? INVALID_U16 : (int16_t)(ctx.cache.soc * 100);
    const int32_t _rpmAdj = isnan(ctx.conf.get_rpm_adjustment()) ? INVALID_32 : (uint32_t)(ctx.conf.get_rpm_adjustment() * 10000);
    const int32_t _timestamp = ctx.cache.rmc.unix_time;
    const int16_t _rpm = ctx.cache.rpm;
    const int32_t _mem = get_free_mem();
    const int8_t _canbus = s.canbus;
    const int32_t _canbus_s = s.sent;
    const int32_t _canbus_e = s.fail;
    const uint32_t _engine_time = static_cast<uint32_t>(ctx.cache.engine_time / 1000L); // send engine time in seconds
    const uint8_t _services = ctx.conf.get_services().serialize();

    static uint8_t v[128];
    int offset = 0;

    add1Int(v, offset, _gpsFix);      // 1
    add4Int(v, offset, _atmo);        // 4 5
    add2Int(v, offset, _temp);        // 2 7
    add2Int(v, offset, _hum);         // 2 9
    add4Int(v, offset, _lat);         // 4 13
    add4Int(v, offset, _lon);         // 4 17
    add4Int(v, offset, _mem);         // 4 21
    add1Int(v, offset, _canbus);      // 1 22
    add4Int(v, offset, _canbus_s);    // 4 26
    add4Int(v, offset, _canbus_e);    // 4 30
    add2Int(v, offset, _sog);         // 2 32
    add2Int(v, offset, _cog);         // 2 34
    add2Int(v, offset, _rpm);         // 2 36
    add4Int(v, offset, _engine_time); // 4 40
    add4Int(v, offset, _timestamp);   // 4 44
    add1Int(v, offset, _services);    // 1 48
    add4Int(v, offset, _rpmAdj);      // 4 49
    add2Int(v, offset, _current);     // 2 53
    add2Int(v, offset, _voltage);     // 2 55
    add2Int(v, offset, _soc);         // 2 57
    ble.set_field_value(0, v, offset);
  }
}

bool BLEConf::is_enabled()
{
  return enabled;
}

void BLEConf::enable()
{
  enabled = true;
}

void BLEConf::disable()
{
  enabled = false;
}

void BLEConf::set_device_name(const char *name)
{
  ble.set_device_name(name);
  ctx.conf.save_device_name(name);
}