#include "BLEConf.h"
#include "N2K_router.h"
#include "Utils.h"
#include "Conf.h"
#include <Log.h>
#include "DHTesp.h"

#define INVALID_32 0xFFFFFF7F
#define INVALID_U32 0xFFFFFFFF
#define INVALID_16 0xFF7F
#define INVALID_U16 0xFFFF

#pragma region BLEBuffer
// BLEBuffer is a simple buffer to store data to be sent over BLE
// It is used to pack data into a single buffer to be sent over BLE
// It is not thread safe, so it should be used in a single thread
class BLEBuffer
{
public:
  BLEBuffer() : offset(0)
  {
  }

  int offset;
  uint8_t buffer[64];

  uint8_t *get_buffer()
  {
    return buffer;
  }

  int get_offset()
  {
    return offset;
  }

  BLEBuffer &reset()
  {
    offset = 0;
    return *this;
  }

  BLEBuffer &operator<<(uint32_t data32)
  {
    add4Int(data32);
    return *this;
  }

  BLEBuffer &operator<<(int32_t data32)
  {
    add4Int(data32);
    return *this;
  }

  BLEBuffer &add4Int(int32_t data32)
  {
    buffer[offset + 0] = data32;
    buffer[offset + 1] = data32 >> 8;
    buffer[offset + 2] = data32 >> 16;
    buffer[offset + 3] = data32 >> 24;
    offset += 4;
    return *this;
  }

  BLEBuffer &operator<<(uint16_t data16)
  {
    add2Int(data16);
    return *this;
  }

  BLEBuffer &operator<<(int16_t data16)
  {
    add2Int(data16);
    return *this;
  }

  BLEBuffer &add2Int(int16_t data16)
  {
    buffer[offset + 0] = data16;
    buffer[offset + 1] = data16 >> 8;
    offset += 2;
    return *this;
  }

  BLEBuffer &operator<<(uint8_t data8)
  {
    add1Int(data8);
    return *this;
  }

  BLEBuffer &operator<<(int8_t data8)
  {
    add1Int(data8);
    return *this;
  }

  BLEBuffer &add1Int(int8_t data8)
  {
    buffer[offset + 0] = data8;
    offset += 1;
    return *this;
  }
};
#pragma endregion

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CONF_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define DATA_UUID "55da66c7-801f-498d-b652-c57cb3f1b590"
#define COMMAND_UUID "68ad1094-0989-4e22-9f21-4df7ef390803"

#define MAX_DATA 7

#define BLE_LOG_TAG "BLE"

#define BLE_UPDATE_PERIOD 1000000

BLEConf::BLEConf(const Context& _ctx, command_callback cback)
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
    if (c_back)
      c_back(command, command_value);
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

void BLEConf::loop(unsigned long ms)
{
  if (enabled && check_elapsed(ms, last_sent, BLE_UPDATE_PERIOD))
  {
    N2KStats s(ctx.n2k.get_bus().getStats());

    double p = ctx.cache.get_pressure(ctx.conf);
    double h = ctx.cache.get_humidity(ctx.conf);
    double t = ctx.cache.get_temperature(ctx.conf);

    const int8_t _gpsFix = ctx.cache.gsa.fix;
    const uint32_t _atmo = isnan(p) ? INVALID_U32 : (uint32_t)(p * 10.0);
    const int16_t _temp = isnan(t) ? INVALID_16 : (int16_t)(t * 10);
    const int16_t _hum = isnan(h) ? INVALID_16 : (int16_t)(h * 100);
    const int32_t _lat = isnan(ctx.cache.rmc.lat) ? INVALID_32 : (int32_t)(ctx.cache.rmc.lat * 1000000);
    const int32_t _lon = isnan(ctx.cache.rmc.lon) ? INVALID_32 : (int32_t)(ctx.cache.rmc.lon * 1000000);
    const int16_t _sog = isnan(ctx.cache.rmc.sog) ? INVALID_16 : (int16_t)(ctx.cache.rmc.sog * 100);
    const int16_t _cog = isnan(ctx.cache.rmc.cog) ? INVALID_16 : (int16_t)(ctx.cache.rmc.cog * 10);
    const int16_t _current = isnan(ctx.cache.battery.current) ? INVALID_16 : (int16_t)(ctx.cache.battery.current * 100);
    const int16_t _voltage = isnan(ctx.cache.battery.voltage) ? INVALID_16 : (int16_t)(ctx.cache.battery.voltage * 100);
    const int16_t _soc = isnan(ctx.cache.battery.soc) ? INVALID_U16 : (int16_t)(ctx.cache.battery.soc * 100);
    const uint32_t _rpmAdj = isnan(ctx.conf.get_rpm_adjustment()) ? INVALID_U32 : (uint32_t)(ctx.conf.get_rpm_adjustment() * 10000);
    const int32_t _timestamp = ctx.cache.rmc.unix_time;
    const uint16_t _rpm = ctx.cache.engine.rpm;
    const int32_t _mem = get_free_mem();
    const int8_t _canbus = s.canbus;
    const int32_t _canbus_s = s.sent;
    const int32_t _canbus_e = s.fail;
    const uint32_t _engine_time = static_cast<uint32_t>(ctx.cache.engine.engine_time / 1000L); // send engine time in seconds
    const uint8_t _services = ctx.conf.get_services().serialize();
    const uint8_t _n2k_source = ctx.conf.get_n2k_source();

    static BLEBuffer b;
    b.reset() << _gpsFix  // 1 1
        << _atmo          // 4 5
        << _temp          // 2 7
        << _hum           // 2 9
        << _lat           // 4 13
        << _lon           // 4 17
        << _mem           // 4 21
        << _canbus        // 1 22
        << _canbus_s      // 4 26
        << _canbus_e      // 4 30
        << _sog           // 2 32
        << _cog           // 2 34
        << _rpm           // 2 36
        << _engine_time   // 4 40
        << _timestamp     // 4 44
        << _services      // 1 45
        << _rpmAdj        // 4 49
        << _current       // 2 51
        << _voltage       // 2 53
        << _soc           // 2 55
        << _n2k_source;   // 1 56

    ble.set_field_value(0, b.get_buffer(), b.get_offset());
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