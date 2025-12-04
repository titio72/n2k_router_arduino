#ifdef PIO_UNIT_TESTING
  #include <MockEEPROM.h>
  #define EEE mockEEPROM
#else
  #ifdef NATIVE
    #include <MockEEPROM.h>
    #define EEE mockEEPROM
  #else
    #include <EEPROM.h>
    #define EEE EEPROM
  #endif
#endif
#include <Log.h>
#include "Conf.h"

#define NO_CONF 0xFF

static const char* CONF_LOG_TAG = "CONF";

#pragma region N2KServices

#define GPS_ID 0
#define DHT_ID 1
#define BME_ID 2
#define SYT_ID 3
#define RPM_ID 4
#define STW_ID 5
#define VED_ID 6
#define MAX_CONF 7

#define SVC_ACCESSOR(name, id)                        \
    bool N2KServices::is_##name() const               \
    {                                                 \
        return (conf & ((1 << id) & 0xFF)) != 0;      \
    }                                                 \
    void N2KServices::set_##name(bool v)              \
    {                                                 \
        if (v)                                        \
        {                                             \
            conf |= (1 << id);                        \
        }                                             \
        else                                          \
        {                                             \
            conf &= ~(1 << id);                       \
        }                                             \
    }

#define BIT_MASK(id) (uint8_t)(1 << id)

N2KServices::N2KServices()
{
    conf =
        DEFAULT_USE_GPS * BIT_MASK(GPS_ID) |
        DEFAULT_USE_DHT * BIT_MASK(DHT_ID) |
        DEFAULT_USE_BME * BIT_MASK(BME_ID) |
        DEFAULT_USE_TIME * BIT_MASK(SYT_ID) |
        DEFAULT_USE_TACHO * BIT_MASK(RPM_ID) |
        DEFAULT_SOG_2_STW * BIT_MASK(STW_ID) |
        DEFAULT_USE_VE_DIRECT * BIT_MASK(VED_ID);
}

uint8_t N2KServices::size() const
{
    return MAX_CONF;
}

void N2KServices::deserialize(uint8_t v)
{
    conf = v;
}

uint8_t N2KServices::serialize() const
{
    return conf;
}

N2KServices &N2KServices::operator=(const N2KServices &svc)
{
    conf = svc.conf;
    return *this;
}

bool N2KServices::from_string(const char *value)
{
    conf = 0;
    for (int i = 0; i < MAX_CONF; i++)
    {
        if (value[i]!='0') conf |= BIT_MASK(i);
    }
    return true;
}

bool N2KServices::to_string(char *dest, size_t len) const
{
    if (len < MAX_CONF + 1)
    {
        return false;
    }
    else
    {
        dest[MAX_CONF] = '\0';
        for (int i = 0; i < MAX_CONF; i++)
        {
            dest[i] = (conf & BIT_MASK(i)) ? '1' : '0';
        }
        return true;
    }
}

SVC_ACCESSOR(use_gps, GPS_ID)
SVC_ACCESSOR(use_dht, DHT_ID)
SVC_ACCESSOR(use_bme, BME_ID)
SVC_ACCESSOR(send_time, SYT_ID)
SVC_ACCESSOR(use_tacho, RPM_ID)
SVC_ACCESSOR(sog_2_stw, STW_ID)
SVC_ACCESSOR(use_vedirect, VED_ID)

#pragma endregion

ConfigurationRW::ConfigurationRW() : initialized(false)
{
}

ConfigurationRW::~ConfigurationRW() {}

void do_log(Conf &conf, const char* action, bool success)
{
    Log::tracex(CONF_LOG_TAG, action, "gps {%d} bme {%d} dht {%d} time {%d} tacho {%d} stw {%d} ved {%d} \
        n2k src {%d} rpm adjustment {%d} device {%s} battery {%d} success {%d}",
        conf.services.is_use_gps(), conf.services.is_use_bme(), conf.services.is_use_dht(), conf.services.is_send_time(),
        conf.services.is_use_tacho(), conf.services.is_sog_2_stw(), conf.services.is_use_vedirect(),
        conf.n2k_source, conf.rpm_adjustment, conf.device_name, conf.battery_capacity_Ah, success);
}

int ConfigurationRW::init()
{
    if (initialized)
        return CONFIG_RES_ALREADY_INITIALIZED;

    initialized = true;
    size_t s = sizeof(conf) + sizeof(uint64_t); // conf + engine hours
    if (!EEE.begin(s))
    {
        Log::tracex(CONF_LOG_TAG, "Init", "Failed to init EEPROM");
        return CONFIG_RES_EEPROM_FAIL;
    }

    uint8_t conf_version = EEE.read(0);
    if (conf_version != CONF_VERSION)
    {
        Log::tracex(CONF_LOG_TAG, "Init", "Conf version check failed - start with defaults");
        if (!save()) return CONFIG_RES_EEPROM_FAIL;
        if (EEE.commit()) return CONFIG_RES_VERSION_MISMATCH; else return CONFIG_RES_EEPROM_FAIL;
    }
    else
    {
        EEE.readBytes(0, &conf, sizeof(conf));
        do_log(conf, "Conf read", true);
        return CONFIG_RES_OK;
    }
}

bool ConfigurationRW::save()
{
    Log::tracex(CONF_LOG_TAG, "Writing conf", "Bytes {%d}", sizeof(conf));
    size_t s = EEE.writeBytes(0, &conf, sizeof(conf));
    if (s != sizeof(conf))
    {
        Log::tracex(CONF_LOG_TAG, "Conf write error", "wrote {%d} expected {%d}", s, sizeof(conf));
        return false;
    }
    if (EEE.commit())
    {
        do_log(conf, "Conf written", true);
        return true;
    }
    else
    {
        Log::tracex(CONF_LOG_TAG, "Conf commit error", "commit failed");
        return false;
    }
}

const N2KServices &ConfigurationRW::get_services() const
{
    return conf.services;
}

bool ConfigurationRW::save_services(N2KServices &s)
{
    conf.services = s;
    return save();
}

MeteoSource ConfigurationRW::get_pressure_source() const
{
    if (conf.services.is_use_bme())
    {
        return METEO_BME;
    }
    else
    {
        return METEO_NONE;
    }
}

MeteoSource ConfigurationRW::get_temperature_source() const
{
    if (conf.services.is_use_dht())
    {
        return METEO_DHT;
    }
    else if (conf.services.is_use_bme())
    {
        return METEO_BME;
    }
    else
    {
        return METEO_NONE;
    }
}

MeteoSource ConfigurationRW::get_temperature_el_source() const
{
    if (conf.services.is_use_bme() && conf.services.is_use_dht())
    {
        return METEO_BME;
    }
    else
    {
        return METEO_NONE;
    }
}

MeteoSource ConfigurationRW::get_humidity_source() const
{
    if (conf.services.is_use_dht())
    {
        return METEO_DHT;
    }
    else if (conf.services.is_use_bme())
    {
        return METEO_BME;
    }
    else
    {
        return METEO_NONE;
    }
}

uint64_t ConfigurationRW::get_engine_hours() const
{
    uint64_t hh = EEE.readULong64(/* read at the end of teh configuration */ sizeof(conf));
    Log::tracex(CONF_LOG_TAG, "Read", "engine time {%lu-%03d}", (uint32_t)(hh / 1000), (uint16_t)hh % 1000);
    return hh;
}

bool ConfigurationRW::save_engine_hours(uint64_t h)
{
    size_t r = EEE.writeULong64(sizeof(conf) /* write at the end of teh configuration */, h);
    bool res = (r!=0) && EEE.commit();
    Log::debugx(CONF_LOG_TAG, "Write", "engine time {%lu-%d %lu} success {%d}", (uint32_t)(h / 1000), (uint16_t)(h % 1000), res);
    return r;
}

unsigned char ConfigurationRW::get_uart_speed() const
{
    return DEFAULT_GPS_SPEED;
}

bool ConfigurationRW::save_uart_speed(unsigned char s)
{
    // do nothing - for compatibility
    return true;
}

unsigned char ConfigurationRW::get_n2k_source() const
{
    return conf.n2k_source;
}

bool ConfigurationRW::save_n2k_source(unsigned char src)
{
    conf.n2k_source = src;
    return save();
}

double ConfigurationRW::get_rpm_adjustment() const
{
    return (double)(conf.rpm_adjustment) / 1000.0;
}

bool ConfigurationRW::save_rpm_adjustment(double d)
{
    conf.rpm_adjustment = (int32_t)(d * 1000.0);
    return save();
}

const char *ConfigurationRW::get_device_name() const
{
    return conf.device_name;
}

bool ConfigurationRW::save_device_name(const char *name)
{
    strncpy(conf.device_name, name, sizeof(conf.device_name) - 1);
    conf.device_name[sizeof(conf.device_name) - 1] = '\0';
    return save();
}

uint16_t ConfigurationRW::get_batter_capacity() const
{
    return conf.battery_capacity_Ah;
}

bool ConfigurationRW::save_batter_capacity(uint16_t c)
{
    conf.battery_capacity_Ah = c;
    return save();
}
