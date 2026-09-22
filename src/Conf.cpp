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
#include <math.h>
#include <string.h>
#include <limits>
#include "Conf.h"
#ifndef NATIVE
#include <Preferences.h>
#endif

#define NO_CONF 0xFF

static const char *CONF_LOG_TAG = "CONF";

#pragma region Services

#define SVC_GPS_ID 0
#define SVC_DHT_ID 1
#define SVC_BME_ID 2
#define SVC_SYT_ID 3
#define SVC_RPM_ID 4
#define SVC_SOG2STW_ID 5
#define SVC_VED_ID 6
#define SVC_N2K_SRC_ID 7
#define SVC_TMP_ID 8
#define SVC_STW_PADDLE_ID 9
#define SVC_LOGGER_ID 10
#define MAX_CONF 11

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
        DEFAULT_USE_GPS * BIT_MASK(SVC_GPS_ID) |
        DEFAULT_USE_DHT * BIT_MASK(SVC_DHT_ID) |
        DEFAULT_USE_BME * BIT_MASK(SVC_BME_ID) |
        DEFAULT_USE_TIME * BIT_MASK(SVC_SYT_ID) |
        DEFAULT_USE_TACHO * BIT_MASK(SVC_RPM_ID) |
        DEFAULT_SOG_2_STW * BIT_MASK(SVC_SOG2STW_ID) |
        DEFAULT_USE_VE_DIRECT * BIT_MASK(SVC_VED_ID) |
        DEFAULT_KEEP_N2K_SRC * BIT_MASK(SVC_N2K_SRC_ID) |
        DEFAULT_USE_TMP * BIT_MASK(SVC_TMP_ID) |
        DEFAULT_STW_PADDLE * BIT_MASK(SVC_STW_PADDLE_ID) |
        DEFAULT_USE_LOGGER * BIT_MASK(SVC_LOGGER_ID);
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

SVC_ACCESSOR(use_gps, SVC_GPS_ID)
SVC_ACCESSOR(use_dht, SVC_DHT_ID)
SVC_ACCESSOR(use_bme, SVC_BME_ID)
SVC_ACCESSOR(send_time, SVC_SYT_ID)
SVC_ACCESSOR(use_tacho, SVC_RPM_ID)
SVC_ACCESSOR(sog_2_stw, SVC_SOG2STW_ID)
SVC_ACCESSOR(use_vedirect, SVC_VED_ID)
SVC_ACCESSOR(keep_n2k_src, SVC_N2K_SRC_ID)
SVC_ACCESSOR(use_tmp, SVC_TMP_ID)
SVC_ACCESSOR(use_stw_paddle, SVC_STW_PADDLE_ID)
SVC_ACCESSOR(use_logger, SVC_LOGGER_ID)

#pragma endregion

#pragma region Persistence & Logging
void do_log(Conf &conf, const char *action, bool success)
{
    Log::tracex(CONF_LOG_TAG, action, "\n use_gps {%d}\n send_time {%d}\n sog_2_stw {%d}\n use_bme {%d}\n use_dht {%d}\n use_tacho {%d}\n use_vedirect {%d}\n use_tmp {%d}\n use_stw_paddle {%d}\n n2k_src {%d}\n rpm_adjustment {%d}\n stw_alpha {%d}\n stw_adjustment {%d}\n device {%s}\n battery {%d}\n success {%d}",
                conf.services.is_use_gps(), conf.services.is_send_time(), conf.services.is_sog_2_stw(), conf.services.is_use_bme(), conf.services.is_use_dht(),
                conf.services.is_use_tacho(), conf.services.is_use_vedirect(),
                conf.services.is_use_tmp(), conf.services.is_use_stw_paddle(), conf.n2k_source, conf.rpm_adjustment, 
                conf.stw_paddle_alpha, conf.stw_paddle_adjustment,
                conf.device_name, conf.battery_capacity_Ah, success);
}

static bool eee_initialized = false;

static bool _init_persistence()
{
    bool res = EEE.begin(sizeof(Conf)); // engine hours are not stored here, see EngineHoursPersistenceNVS
    Log::tracex(CONF_LOG_TAG, "Init Persistence", "Size {%d} success {%d}", sizeof(Conf), res ? 1 : 0);
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

#ifndef NATIVE
/**
 * Engine hours live in NVS, under their own key, rather than after the Conf struct in the EEPROM buffer:
 *  - the location no longer depends on sizeof(Conf), so Conf can grow without moving/corrupting the hours
 *  - a missing key reads as 0 (an erased EEPROM area reads back as 0xFF..., i.e. absurd hours)
 *  - NVS spreads the once-a-minute writes over its pages instead of erasing the same flash sector each time
 */
class EngineHoursPersistenceNVS : public EngineHoursPersistence
{
public:
    virtual bool init_persistence() override
    {
        if (!open)
        {
            open = prefs.begin(NVS_NAMESPACE, false);
            Log::tracex(CONF_LOG_TAG, "Init NVS", "Namespace {%s} success {%d}", NVS_NAMESPACE, open ? 1 : 0);
        }
        return open;
    }

    virtual bool save_engine_hours(uint64_t milliseconds) override
    {
        return open && prefs.putULong64(NVS_KEY, milliseconds) == sizeof(uint64_t);
    }

    virtual uint64_t load_engine_hours() override
    {
        return open ? prefs.getULong64(NVS_KEY, 0) : 0;
    }

private:
    static constexpr const char *NVS_NAMESPACE = "n2krouter";
    static constexpr const char *NVS_KEY = "engine_ms";
    Preferences prefs;
    bool open = false;
};

static EngineHoursPersistenceNVS engineHoursPersistenceDefault;
#else
// desktop builds have no NVS: keep the value in memory
class EngineHoursPersistenceMemory : public EngineHoursPersistence
{
public:
    virtual bool init_persistence() override { return true; }
    virtual bool save_engine_hours(uint64_t milliseconds) override { value = milliseconds; return true; }
    virtual uint64_t load_engine_hours() override { return value; }

private:
    uint64_t value = 0;
};

static EngineHoursPersistenceMemory engineHoursPersistenceDefault;
#endif

#ifndef NATIVE
/**
 * The BLE pairing passkey lives in NVS, under its own key, same rationale as engine hours above:
 * independent of the Conf EEPROM layout, and a missing key reads as the factory default rather than
 * an erased-flash garbage value.
 */
class BlePasskeyPersistenceNVS : public BlePasskeyPersistence
{
public:
    virtual bool init_persistence() override
    {
        if (!open)
        {
            open = prefs.begin(NVS_NAMESPACE, false);
            Log::tracex(CONF_LOG_TAG, "Init NVS", "Namespace {%s} success {%d}", NVS_NAMESPACE, open ? 1 : 0);
        }
        return open;
    }

    virtual bool save_ble_passkey(uint32_t passkey) override
    {
        return open && prefs.putUInt(NVS_KEY, passkey) == sizeof(uint32_t);
    }

    virtual uint32_t load_ble_passkey() override
    {
        return open ? prefs.getUInt(NVS_KEY, BLE_PASSKEY_FACTORY_DEFAULT) : BLE_PASSKEY_FACTORY_DEFAULT;
    }

private:
    static constexpr const char *NVS_NAMESPACE = "n2krouter";
    static constexpr const char *NVS_KEY = "ble_pk";
    Preferences prefs;
    bool open = false;
};

static BlePasskeyPersistenceNVS blePasskeyPersistenceDefault;
#else
// desktop builds have no NVS: keep the value in memory, defaulting to 0 (open access) for tests
class BlePasskeyPersistenceMemory : public BlePasskeyPersistence
{
public:
    virtual bool init_persistence() override { return true; }
    virtual bool save_ble_passkey(uint32_t passkey) override { value = passkey; return true; }
    virtual uint32_t load_ble_passkey() override { return value; }

private:
    uint32_t value = 0;
};

static BlePasskeyPersistenceMemory blePasskeyPersistenceDefault;
#endif
#pragma endregion

#pragma region EngineHours
// 100 000 hours: anything above is corrupted storage, not a real engine
static const uint64_t MAX_PLAUSIBLE_ENGINE_HOURS_MS = 100000ULL * 3600ULL * 1000ULL;

EngineHours::EngineHours(EngineHoursPersistence *persistence)
    : engine_hours(0),
      initialized(false)
{
    if (persistence == nullptr)
        persistence = &engineHoursPersistenceDefault;
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
        if (engine_hours > MAX_PLAUSIBLE_ENGINE_HOURS_MS)
        {
            Log::tracex(CONF_LOG_TAG, "Init", "Implausible engine hours {%lu} - starting from 0", (uint32_t)(engine_hours / 1000));
            engine_hours = 0;
        }
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

/**
 * Convert a real value to its fixed-point storage form: rounds to nearest (plain truncation turns
 * 0.29 * 100 into 28) and clamps to the range of T (an out-of-range float-to-int cast is undefined).
 */
template <typename T>
static T to_fixed(double value, double scale)
{
    double v = round(value * scale);
    if (isnan(v))
        return 0;
    if (v < (double)std::numeric_limits<T>::min())
        return std::numeric_limits<T>::min();
    if (v > (double)std::numeric_limits<T>::max())
        return std::numeric_limits<T>::max();
    return (T)v;
}

Configuration::Configuration(ConfigurationPersistence *persistence, BlePasskeyPersistence *ble_passkey_persistence)
    : initialized(false)
{
    if (persistence == nullptr)
        persistence = &configurationPersistenceEEPROM;
    this->persistence = persistence;
    if (ble_passkey_persistence == nullptr)
        ble_passkey_persistence = &blePasskeyPersistenceDefault;
    this->ble_passkey_persistence = ble_passkey_persistence;
}

int Configuration::init()
{
    if (initialized)
        return CONFIG_RES_ALREADY_INITIALIZED;

    if (ble_passkey_persistence->init_persistence())
    {
        ble_passkey = ble_passkey_persistence->load_ble_passkey();
    }
    else
    {
        Log::tracex(CONF_LOG_TAG, "Init", "Failed to init BLE passkey persistence - defaulting to open access");
    }

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
    conf.rpm_adjustment = to_fixed<int16_t>(d, RPM_ADJUSTMENT_SCALE);
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

uint32_t Configuration::get_ble_passkey() const
{
    return ble_passkey;
}

bool Configuration::is_ble_passkey_default() const
{
    return get_ble_passkey() == BLE_PASSKEY_FACTORY_DEFAULT;
}

bool Configuration::save_ble_passkey(uint32_t pk)
{
    ble_passkey = pk;
    return ble_passkey_persistence->save_ble_passkey(pk);
}

bool Configuration::save_battery_capacity(uint16_t c)
{
    conf.battery_capacity_Ah = c;
    SAVE_CONF
}

double Configuration::get_sea_temp_alpha() const
{
    return (double)(conf.sea_temp_alpha) / SEA_TEMP_ALPHA_SCALE;
}

double Configuration::get_stw_paddle_alpha() const
{
    return (double)(conf.stw_paddle_alpha) / STW_PADDLE_ALPHA_SCALE;
}

double Configuration::get_sea_temp_adjustment() const
{
    return (double)(conf.sea_temp_adjustment) / SEA_TEMP_ADJUSTMENT_SCALE;
}

double Configuration::get_stw_paddle_adjustment() const
{
    return (double)(conf.stw_paddle_adjustment) / STW_PADDLE_ADJUSTMENT_SCALE;
}

bool Configuration::save_sea_temp_alpha(double a)
{
    conf.sea_temp_alpha = to_fixed<uint8_t>(a, SEA_TEMP_ALPHA_SCALE);
    SAVE_CONF
}

bool Configuration::save_stw_paddle_alpha(double a)
{
    conf.stw_paddle_alpha = to_fixed<uint8_t>(a, STW_PADDLE_ALPHA_SCALE);
    SAVE_CONF
}

bool Configuration::save_sea_temp_adjustment(double a)
{
    conf.sea_temp_adjustment = to_fixed<uint16_t>(a, SEA_TEMP_ADJUSTMENT_SCALE);
    SAVE_CONF
}

bool Configuration::save_stw_paddle_adjustment(double a)
{
    conf.stw_paddle_adjustment = to_fixed<uint16_t>(a, STW_PADDLE_ADJUSTMENT_SCALE);
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

class DummyBlePasskeyPersistence : public BlePasskeyPersistence
{
public:
    virtual bool init_persistence() override
    {
        return true;
    }

    virtual bool save_ble_passkey(uint32_t passkey) override
    {
        return true;
    }

    virtual uint32_t load_ble_passkey() override
    {
        return 0;
    }
} dummyBlePasskeyPersistence;

#ifdef PIO_UNIT_TESTING
MockConfiguration::MockConfiguration()
    : Configuration(&dummyPersistence, &dummyBlePasskeyPersistence)
{
}

MockEngineHours::MockEngineHours()
    : EngineHours(&dummyEngineHoursPersistence)
{
}
#endif