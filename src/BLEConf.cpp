#include "BLEConf.h"
#include "N2K_router.h"
#include "Utils.h"
#include "Conf.h"
#include <Log.h>
#include "DHTesp.h"

#define SERVICE_UUID    "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CONF_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define DATA_UUID       "55da66c7-801f-498d-b652-c57cb3f1b590"
#define COMMAND_UUID    "68ad1094-0989-4e22-9f21-4df7ef390803"

#define GPS_ID       0
#define DHT_ID       1
#define BMP_ID       2
#define SYT_ID       3
#define SIM_ID       4
#define STW_ID       5

#define MAX_CONF     6


#define MAX_DATA     6

BLEConf::BLEConf(Context _ctx) : enabled(false), ctx(_ctx), ble(SERVICE_UUID, "ABN2K"), last_sent(0)
{
}

BLEConf::~BLEConf()
{
}

void BLEConf::on_write(int handle, const char* value)
{
    if (handle==0)
    {
        Configuration& c = ctx.conf;
        Log::trace("[BLE] Changing conf %d %s\n", handle, value);
        int x = strlen(value);
        if (x>GPS_ID) c.use_gps    = (value[GPS_ID]=='1');
        Log::trace("[BLE] Reading conf GPS %d\n", c.use_gps);
        if (x>DHT_ID) c.use_dht  = (value[DHT_ID]=='1');
        Log::trace("[BLE] Reading conf DHT %d\n", c.use_dht);
        if (x>BMP_ID) c.use_bmp = (value[BMP_ID]=='1');
        Log::trace("[BLE] Reading conf BMP %d\n", c.use_bmp);
        if (x>SYT_ID) c.send_time  = (value[SYT_ID]=='1');
        Log::trace("[BLE] Reading conf SYT %d\n", c.send_time);
        if (x>SIM_ID) c.simulator  = (value[SIM_ID]=='1');
        Log::trace("[BLE] Reading conf SIM %d\n", c.simulator);
        if (x>STW_ID) c.sog_2_stw  = (value[STW_ID]=='1');
        Log::trace("[BLE] Reading conf STW %d\n", c.sog_2_stw);
        c.save();
    }
}

void BLEConf::setup()
{
    Log::trace("[BLE] Loading configuration\n");
    Configuration& c = ctx.conf;
    ble.add_field("data", DATA_UUID);
    ble.add_setting("conf", CONF_UUID);
    ble.add_setting("command", COMMAND_UUID);
    ble.set_write_callback(this);
    ble.setup();
    char cnf[MAX_CONF+1];
    cnf[MAX_CONF] = 0;
    cnf[GPS_ID] = c.use_gps?'1':'0';
    cnf[BMP_ID] = c.use_bmp?'1':'0';
    cnf[DHT_ID] = c.use_dht?'1':'0';
    cnf[SYT_ID] = c.send_time?'1':'0';
    cnf[SIM_ID] = c.simulator?'1':'0';
    cnf[STW_ID] = c.sog_2_stw?'1':'0';
    ble.set_setting_value(0, cnf);

    ble.begin();
    Log::trace("[BLE] Setup ok {%s}\n", cnf);
}

void addInt(uint8_t* dest, int &offset, int32_t data32) {
	uint8_t* temp = dest + offset;
	temp[0] = data32;
	temp[1] = data32 >> 8;
	temp[2] = data32 >> 16;
	temp[3] = data32 >> 24;
    offset += 4;
}

void addShort(uint8_t* dest, int &offset, int16_t data16) {
	uint8_t* temp = dest + offset;
	temp[0] = data16;
	temp[1] = data16 >> 8;
    offset += 2;
}

void addChar(uint8_t* dest, int &offset, int8_t data8) {
	uint8_t* temp = dest + offset;
	temp[0] = data8;
    offset += 1;
}

void BLEConf::loop(unsigned long ms)
{
    if (enabled && (ms-last_sent)>950)
    {
      last_sent = ms;
      N2KStats s(ctx.n2k.get_bus().getStats());

      int8_t _gpsFix = ctx.cache.gsa.fix;
      int32_t _atmo = (int32_t)(ctx.cache.pressure*10.0);
      int16_t _temp = (int16_t)(ctx.cache.temperature*10);
      int16_t _hum = (int16_t)(ctx.cache.humidity*100);
      int32_t _lat = (int32_t)(ctx.cache.rmc.lat * 1000000);
      int32_t _lon = (int32_t)(ctx.cache.rmc.lon * 1000000);
      int16_t _sog = (int16_t)(ctx.cache.rmc.sog * 100);
      int16_t _cog = (int16_t)(ctx.cache.rmc.cog * 10);
      int32_t _mem = get_free_mem();
      int8_t _canbus = s.canbus;
      int32_t _canbus_s = s.sent;
      int32_t _canbus_e = s.fail;
      static uint8_t* v = new uint8_t[128];
      int offset = 0;
      addChar(v, offset, _gpsFix);
      addInt(v, offset, _atmo);
      addShort(v, offset, _temp);
      addShort(v, offset, _hum);
      addInt(v, offset, _lat);
      addInt(v, offset, _lon);
      addInt(v, offset, _mem);
      addChar(v, offset, _canbus);
      addInt(v, offset, _canbus_s);
      addInt(v, offset, _canbus_e);
      // add at the end for UI backward compatibility
      addShort(v, offset, _sog);
      addShort(v, offset, _cog);
      ble.set_field_value(0, v, offset);
    }
}

bool BLEConf::is_enabled()
{
  return enabled;
}

void BLEConf::enable()
{
  if (!enabled)
  {
    enabled = true;
  }
}

void BLEConf::disable()
{
  if (enabled)
  {
    enabled = false;
  }
}
