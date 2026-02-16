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

static const char *CONF_LOG_TAG = "CONF";

#pragma region N2KServices

#define GPS_ID 0
#define DHT_ID 1
#define BME_ID 2
#define SYT_ID 3
#define RPM_ID 4
#define SOG2STW_ID 5
#define VED_ID 6
#define N2K_SRC_ID 7
#define TMP_ID 8
#define STW_PADDLE_ID 9
#define MAX_CONF 10

#define SVC_ACCESSOR(name, id)                   \
    bool N2KServices::is_##name() const          \
    {                                            \
        return (conf & ((1 << id) & 0xFFFF)) != 0; \
    }                                            \
    void N2KServices::set_##name(bool v)         \
    {                                            \
        if (v)                                   \
        {                                        \
            conf |= (1 << id);                   \
        }                                        \
        else                                     \
        {                                        \
            conf &= ~(1 << id);                  \
        }                                        \
    }

#define BIT_MASK(id) (uint16_t)(1 << id)

N2KServices::N2KServices()
{
    conf =
        DEFAULT_USE_GPS * BIT_MASK(GPS_ID) |
        DEFAULT_USE_DHT * BIT_MASK(DHT_ID) |
        DEFAULT_USE_BME * BIT_MASK(BME_ID) |
        DEFAULT_USE_TIME * BIT_MASK(SYT_ID) |
        DEFAULT_USE_TACHO * BIT_MASK(RPM_ID) |
        DEFAULT_SOG_2_STW * BIT_MASK(SOG2STW_ID) |
        DEFAULT_USE_VE_DIRECT * BIT_MASK(VED_ID) |
        DEFAULT_KEEP_N2K_SRC * BIT_MASK(N2K_SRC_ID) |
        DEFAULT_USE_TMP * BIT_MASK(TMP_ID) |
        DEFAULT_STW_PADDLE * BIT_MASK(STW_PADDLE_ID);
}

uint8_t N2KServices::size() const
{
    return MAX_CONF;
}

void N2KServices::deserialize(uint16_t v)
{
    conf = v;
}

uint16_t N2KServices::serialize() const
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
    int l = strlen(value);
    for (int i = 0; i < MAX_CONF && i < l; i++)
    {
        if (value[i] != '0')
            conf |= BIT_MASK(i);
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
SVC_ACCESSOR(sog_2_stw, SOG2STW_ID)
SVC_ACCESSOR(use_vedirect, VED_ID)
SVC_ACCESSOR(keep_n2k_src, N2K_SRC_ID)
SVC_ACCESSOR(use_tmp, TMP_ID)
SVC_ACCESSOR(use_stw_paddle, STW_PADDLE_ID)

#pragma endregion

#pragma region Persistence & Logging
void do_log(Conf &conf, const char *action, bool success)
{
    Log::tracex(CONF_LOG_TAG, action, "\n use_gps {%d}\n send_time {%d}\n sog_2_stw {%d}\n use_bme {%d}\n use_dht {%d}\n use_tacho {%d}\n use_vedirect {%d}\n use_tmp {%d}\n use_stw_paddle {%d}\n n2k_src {%d}\n rpm_adjustment {%d}\n device {%s}\n battery {%d}\n success {%d}",
                conf.services.is_use_gps(), conf.services.is_send_time(), conf.services.is_sog_2_stw(), conf.services.is_use_bme(), conf.services.is_use_dht(),
                conf.services.is_use_tacho(), conf.services.is_use_vedirect(),
                conf.services.is_use_tmp(), conf.services.is_use_stw_paddle(), conf.n2k_source, conf.rpm_adjustment, conf.device_name, conf.battery_capacity_Ah, success);
}

static bool eee_initialized = false;

static bool _init_persistence()
{
    bool res = EEE.begin(sizeof(Conf) + sizeof(uint64_t)); // extra space for engine hours
    Log::tracex(CONF_LOG_TAG, "Init Persistence", "Size {%d} success {%d}", sizeof(Conf) + sizeof(uint64_t), res ? 1 : 0);
    eee_initialized = res;
    return res;
}

class ConfigurationPersistenceEEPROM : public ConfigurationPersistence
{
public:
    virtual bool init_persistence() override
    {
        return _init_persistence();
    }

    virtual bool save_configuration(const Conf &conf) override
    {
        size_t written = 0;
        written += EEE.writeBytes(0, (const void *)&conf, sizeof(Conf));
        bool res = (written == sizeof(Conf)) && EEE.commit();
        do_log((Conf &)conf, "Save", res);
        return res;
    }

    virtual bool load_configuration(Conf &conf) override
    {
        size_t read = 0;
        read += EEE.readBytes(0, (void *)&conf, sizeof(Conf));
        bool res = (read == sizeof(Conf));
        do_log(conf, "Load", res);
        return res;
    }
};

static ConfigurationPersistenceEEPROM configurationPersistenceEEPROM;

class EngineHoursPersistenceEEPROM : public EngineHoursPersistence
{
public:
    virtual bool init_persistence() override
    {
        return _init_persistence();
    }

    virtual bool save_engine_hours(uint64_t milliseconds) override
    {
        size_t r = EEE.writeULong64(sizeof(Conf) /* write at the end of the configuration */, milliseconds);
        bool res = (r != 0) && EEE.commit();
        //Log::tracex(CONF_LOG_TAG, "Write", "engine time {%lu-%d} success {%d}", (uint32_t)(milliseconds / 1000), (uint16_t)(milliseconds % 1000), res?1:0);
        return r;
    }

    virtual uint64_t load_engine_hours() override
    {
        uint64_t hh = EEE.readULong64(/* read at the end of the configuration */ sizeof(Conf));
        return hh;
    }
};

static EngineHoursPersistenceEEPROM engineHoursPersistenceEEPROM;
#pragma endregion

#pragma region EngineHours
EngineHours::EngineHours(EngineHoursPersistence *persistence)
    : engine_hours(0),
      initialized(false)
{
    if (persistence == nullptr)
        persistence = &engineHoursPersistenceEEPROM;
    this->persistence = persistence;
}

int EngineHours::init()
{
    if (initialized)
        return CONFIG_RES_ALREADY_INITIALIZED;

    if (persistence->init_persistence())
    {
        Log::tracex(CONF_LOG_TAG, "Init", "Persistence initialized, loading engine hours");
        engine_hours = persistence->load_engine_hours();
        Log::tracex(CONF_LOG_TAG, "Init", "Loaded engine hours {%lu-%d}", (uint32_t)(engine_hours / 1000), (uint16_t)(engine_hours % 1000));
        initialized = true;
        return CONFIG_RES_OK;
    }
    else
    {
        Log::tracex(CONF_LOG_TAG, "EngineHours Init", "Failed to init persistence");
        return CONFIG_RES_EEPROM_FAIL;
    }
}

uint64_t EngineHours::get_engine_hours() const
{
    return engine_hours;
}

bool EngineHours::save_engine_hours(uint64_t h)
{
    engine_hours = h;
    return persistence->save_engine_hours(h);
}
#pragma endregion

#pragma region Configuration

#define SAVE_CONF return persistence->save_configuration(conf);

Configuration::Configuration(ConfigurationPersistence *persistence)
    : initialized(false)
{
    if (persistence == nullptr)
        persistence = &configurationPersistenceEEPROM;
    this->persistence = persistence;
}

int Configuration::init()
{
    if (initialized)
        return CONFIG_RES_ALREADY_INITIALIZED;

    if (!persistence->init_persistence())
    {
        Log::tracex(CONF_LOG_TAG, "Init", "Failed to init persistence");
        return CONFIG_RES_EEPROM_FAIL;
    }
    else
    {
        Log::tracex(CONF_LOG_TAG, "Init", "Persistence initialized, loading configuration");
        if (!persistence->load_configuration(conf))
        {
            Log::tracex(CONF_LOG_TAG, "Init", "Failed to load configuration from persistence");
            return CONFIG_RES_EEPROM_FAIL;
        }
        if (conf.conf_version != CONF_VERSION)
        {
            Log::tracex(CONF_LOG_TAG, "Init", "Conf version check failed - start with defaults");
            conf = Conf(); // reset to defaults
            if (!persistence->save_configuration(conf))
            {
                Log::tracex(CONF_LOG_TAG, "Init", "Failed to save default configuration to persistence");
                return CONFIG_RES_EEPROM_FAIL;
            }
            else
            {
                Log::tracex(CONF_LOG_TAG, "Init", "Default configuration saved to persistence");
                initialized = true;
                return CONFIG_RES_VERSION_MISMATCH;
            }
        }
        initialized = true;
        return CONFIG_RES_OK;
    }
}

const N2KServices &Configuration::get_services() const
{
    return conf.services;
}

bool Configuration::save_services(N2KServices &s)
{
    conf.services = s;
    SAVE_CONF
}

MeteoSource Configuration::get_pressure_source() const
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

MeteoSource Configuration::get_temperature_source() const
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

MeteoSource Configuration::get_temperature_el_source() const
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

MeteoSource Configuration::get_humidity_source() const
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

unsigned char Configuration::get_n2k_source() const
{
    return conf.n2k_source;
}

bool Configuration::save_n2k_source(unsigned char src)
{
    conf.n2k_source = src;
    SAVE_CONF
}

double Configuration::get_rpm_adjustment() const
{
    return (double)(conf.rpm_adjustment) / RPM_ADJUSTMENT_SCALE;
}

bool Configuration::save_rpm_adjustment(double d)
{
    conf.rpm_adjustment = (int16_t)(d * RPM_ADJUSTMENT_SCALE);
    SAVE_CONF
}

const char *Configuration::get_device_name() const
{
    return conf.device_name;
}

bool Configuration::save_device_name(const char *name)
{
    strncpy(conf.device_name, name, sizeof(conf.device_name) - 1);
    conf.device_name[sizeof(conf.device_name) - 1] = '\0';
    SAVE_CONF
}

uint16_t Configuration::get_batter_capacity() const
{
    return conf.battery_capacity_Ah;
}

bool Configuration::save_batter_capacity(uint16_t c)
{
    conf.battery_capacity_Ah = c;
    SAVE_CONF
}

double Configuration::get_sea_temp_alpha() const
{
    return (double)(conf.sea_temp_alpha) / 100.0;
}

double Configuration::get_stw_paddle_alpha() const
{
    return (double)(conf.stw_paddle_alpha) / 100.0;
}

double Configuration::get_sea_temp_adjustment() const
{
    return (double)(conf.sea_temp_adjustment) / 10000.0;
}

double Configuration::get_stw_paddle_adjustment() const
{
    return (double)(conf.stw_paddle_adjustment) / 10000.0;
}

bool Configuration::save_sea_temp_alpha(double a)
{
    conf.sea_temp_alpha = (uint8_t)(a * 100.0);
    SAVE_CONF
}

bool Configuration::save_stw_paddle_alpha(double a)
{
    conf.stw_paddle_alpha = (uint8_t)(a * 100.0);
    SAVE_CONF
}

bool Configuration::save_sea_temp_adjustment(double a)
{
    conf.sea_temp_adjustment = (uint16_t)(a * 10000.0);
    SAVE_CONF
}

bool Configuration::save_stw_paddle_adjustment(double a)
{
    conf.stw_paddle_adjustment = (uint16_t)(a * 10000.0);
    SAVE_CONF
}

#pragma endregion

class DummyPersistence : public ConfigurationPersistence
{
public:
    virtual bool init_persistence() override
    {
        return true;
    }

    virtual bool save_configuration(const Conf &conf) override
    {
        return true;
    }

    virtual bool load_configuration(Conf &conf) override
    {
        conf = Conf();
        return true;
    }

    Conf conf;
} dummyPersistence;

class DummyEngineHoursPersistence : public EngineHoursPersistence
{
public:
    virtual bool init_persistence() override
    {
        return true;
    }

    virtual bool save_engine_hours(uint64_t hours) override
    {
        return true;
    }

    virtual uint64_t load_engine_hours() override
    {
        return 0;
    }
} dummyEngineHoursPersistence;

#ifdef PIO_UNIT_TESTING
MockConfiguration::MockConfiguration()
    : Configuration(&dummyPersistence)
{
}

MockEngineHours::MockEngineHours()
    : EngineHours(&dummyEngineHoursPersistence)
{
}
#endif