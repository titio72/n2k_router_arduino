#include "BLEConf.h"
#include "N2K_router.h"
#include "Utils.h"
#include "Conf.h"
#include <Log.h>

static const char* BLE_LOG_TAG = "BLE";

#define BLE_UPDATE_PERIOD 1000000

BLEConf::BLEConf(command_callback cback, InternalBLEState* internalState)
    : enabled(false), ble(BLE_SERVICE_UUID, BLE_DEFAULT_SERVICE_NAME, internalState), last_sent(0), 
    c_back(cback), ble_settings_handle(-1), ble_conf_handle(-1), initialized(false), services_buffer(128)
{
}

BLEConf::~BLEConf()
{
}

const char* BLEConf::get_device_name()
{
  return ble.get_device_name();
}

void BLEConf::on_write(int handle, const char *value)
{
  if (!initialized) return;

  Log::tracex(BLE_LOG_TAG, "Command", "Handle {%d} Command {%s}", handle, value);
  if (handle == ble_conf_handle)
  {
    const char command = 'S';
    if (c_back)
      c_back(command, value);
  }
  else if (handle == ble_settings_handle)
  {
    const char command = value[0];
    const char *command_value = (value + sizeof(char));
    if (c_back)
      c_back(command, command_value);
  }
}

void copy_from_conf(Configuration &conf, char* device_name, size_t l)
{
  strncpy(device_name, conf.get_device_name(), l - 1);
  device_name[l - 1] = '\0';
}

void BLEConf::setup(Context &ctx)
{
  if (initialized) return;

  initialized = true;
  Log::tracex(BLE_LOG_TAG, "Setup", "Start BLE");
  ble.set_device_name(ctx.conf.get_device_name());
  const N2KServices &c = ctx.conf.get_services();
  ble.add_field("data", BLE_DATA_UUID);
  ble_conf_handle = ble.add_setting("conf", BLE_CONF_UUID);
  ble_settings_handle = ble.add_setting("command", BLE_COMMAND_UUID);
  ble.set_write_callback(this);
  ble.setup();
  char cstr[c.size() + 1];
  c.to_string(cstr, sizeof(cstr));
  ble.set_setting_value(0, cstr);
  ble.begin();
  Log::tracex(BLE_LOG_TAG, "Setup", "Conf {%s}", cstr);
}

template <typename T>
static inline T to_int(double value, double factor, T invalid_value)
{
  if (isnan(value))
    return invalid_value;
  return (T)(value * factor);
}

void fill_buffer(ByteBuffer& buffer, Context &ctx)
{
  N2KStats s(ctx.n2k.getStats());

    double p = ctx.data_cache.get_pressure(ctx.conf);
    double h = ctx.data_cache.get_humidity(ctx.conf);
    double t = ctx.data_cache.get_temperature(ctx.conf);

    const int8_t _gpsFix = ctx.data_cache.gps.fix;
    const uint32_t _atmo =  to_int(p, 10.0, INVALID_U32);
    const int16_t _temp = to_int(t, 10.0, INVALID_16);
    const int16_t _hum = to_int(h, 100.0, INVALID_16);
    const int32_t _lat = to_int(ctx.data_cache.gps.latitude_signed, 1000000.0, INVALID_32);
    const int32_t _lon = to_int(ctx.data_cache.gps.longitude_signed, 1000000.0, INVALID_32);
    const int16_t _sog = to_int(ctx.data_cache.gps.sog, 100.0, INVALID_16);
    const int16_t _cog = to_int(ctx.data_cache.gps.cog, 10.0, INVALID_16);
    const int16_t _current = to_int(ctx.data_cache.battery_svc.current, 100.0, INVALID_16);
    const int16_t _voltage = to_int(ctx.data_cache.battery_svc.voltage, 100.0, INVALID_16);
    const int16_t _soc = to_int(ctx.data_cache.battery_svc.soc, 100.0, INVALID_16);
    const uint32_t _rpmAdj = to_int(ctx.conf.get_rpm_adjustment(), 10000.0, INVALID_U32);
    const int32_t _timestamp = ctx.data_cache.gps.gps_unix_time;
    const uint16_t _rpm = ctx.data_cache.engine.rpm;
    const int32_t _mem = get_free_mem();
    const int8_t _canbus = s.canbus;
    const int32_t _canbus_s = s.sent;
    const int32_t _canbus_e = s.fail;
    const uint32_t _engine_time = static_cast<uint32_t>(ctx.data_cache.engine.engine_time / 1000L); // send engine time in seconds
    const uint8_t _services = ctx.conf.get_services().serialize();
    const uint8_t _n2k_source = ctx.conf.get_n2k_source();

    buffer.reset() 
        << _gpsFix        // 1 1
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
}

void BLEConf::loop(unsigned long ms, Context &ctx)
{
  if (enabled && check_elapsed(ms, last_sent, BLE_UPDATE_PERIOD))
  {
    if (strcmp(ble.get_device_name(), ctx.conf.get_device_name())) ble.set_device_name(ctx.conf.get_device_name());
    fill_buffer(services_buffer, ctx);
    ble.set_field_value(0, services_buffer.data(), services_buffer.length());
  }
}

bool BLEConf::is_enabled()
{
  return enabled;
}

void BLEConf::enable()
{
  if (initialized)
    enabled = true;
}

void BLEConf::disable()
{
  enabled = false;
}
