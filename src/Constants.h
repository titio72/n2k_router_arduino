#ifndef _CONTANTS_H_
#define _CONTANTS_H_


#ifndef BME_ADDRESS
#define BME_ADDRESS 0x76 // 0x76 or 0x77
#endif

#ifndef DHT_TYPE
#define DHT_TYPE DHT22
#endif

#ifndef GPS_TYPE
#define GPS_TYPE 0 // dummy
#endif

#ifndef DO_TACHOMETER
#define DO_TACHOMETER 0
#endif

#ifndef DO_VE_DIRECT
#define DO_VE_DIRECT 0
#endif

#ifndef DO_DISPLAY
#define DO_DISPLAY 0
#endif

#ifndef DO_LOGGER
#define DO_LOGGER 1 // serial logging compiled in; the use_logger service flag switches it on/off at runtime
#endif

#ifndef RESET_PIN
#define RESET_PIN -1 // no reset button
#endif
#ifndef RESET_HOLD_USEC
#define RESET_HOLD_USEC 2000000L // how long the reset button must be held (micros)
#endif

#ifndef TACHO_POLES
#define TACHO_POLES 12 // Default number of poles for tachometer
#endif
#ifndef TACHO_RPM_RATIO
#define TACHO_RPM_RATIO 1.5 // Default ratio for tachometer RPM calculation
#endif
#ifndef TACHO_RPM_ADJUSTMENT
#define TACHO_RPM_ADJUSTMENT 0.0 // Default adjustment for tachometer RPM calculation
#endif

#define RPM_ADJUSTMENT_SCALE 100.0
#define STW_PADDLE_ADJUSTMENT_SCALE 100.0
#define STW_PADDLE_ALPHA_SCALE 100.0
#define SEA_TEMP_ALPHA_SCALE 100.0
#define SEA_TEMP_ADJUSTMENT_SCALE 100.0

#define MAX_RETRY 3 // number of quick retries to start an agent
#define AGENT_RETRY_COOLDOWN_USEC 30000000UL // then one more attempt every 30 s
#define N2K_BLINK_USEC 100000L /* micros */

#define DEFAULT_USE_GPS 0
#define DEFAULT_USE_BME 0
#define DEFAULT_USE_DHT 0
#define DEFAULT_SOG_2_STW 0
#define DEFAULT_USE_TIME 0
#define DEFAULT_USE_TACHO 0
#define DEFAULT_USE_VE_DIRECT 0
#define DEFAULT_KEEP_N2K_SRC 0
#define DEFAULT_USE_TMP 0
#define DEFAULT_STW_PADDLE 0
#define DEFAULT_USE_LOGGER 1
#define DEFAULT_RPM_ADJUSTMENT 1.00
#define DEFAULT_STW_PADDLE_ALPHA 1.00
#define DEFAULT_SEA_TEMP_ALPHA 1.00
#define DEFAULT_SEA_TEMP_ADJUSTMENT 1.00
#define DEFAULT_STW_PADDLE_ADJUSTMENT 1.00
#define DEFAULT_N2K_SOURCE 22

#define DEFAULT_BATTERY_CAPACITY 280

#define APP_LOG_TAG "APP"

#define BLE_DEFAULT_SERVICE_NAME "N2KRouter"
#define BLE_SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BLE_CONF_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BLE_DATA_UUID "55da66c7-801f-498d-b652-c57cb3f1b590"
#define BLE_COMMAND_UUID "68ad1094-0989-4e22-9f21-4df7ef390803"
#define BLE_HEARTBEAT_UUID "31a627d4-90cd-43df-8c0d-460e77fd294b" // writable without pairing
#define BLE_INACTIVITY_TIMEOUT 30000000UL  // microseconds

//ble values
static const int32_t INVALID_32 = 0x7FFFFFFF;
static const uint32_t INVALID_U32 = 0xFFFFFFFF;
static const int16_t INVALID_16 = 0x7FFF;
static const uint16_t INVALID_U16 = 0xFFFF;

// EnvMessenger period
#define PERIOD_MICROS_ENV 2000000

#endif /* _CONTANTS_H_ */