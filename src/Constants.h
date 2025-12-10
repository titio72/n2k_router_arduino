#ifndef _CONTANTS_H_
#define _CONTANTS_H_


static const int UART_SPEEDS = 6;
static unsigned int UART_SPEED[] = {4800, 9600, 19200, 38400, 57600, 115200};

static unsigned char UART_SPEED_4800   = 0;
static unsigned char UART_SPEED_9600   = 1;
static unsigned char UART_SPEED_19200  = 2;
static unsigned char UART_SPEED_38400  = 3;
static unsigned char UART_SPEED_57600  = 4;
static unsigned char UART_SPEED_115200 = 5;

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

#ifndef TACHO_POLES
#define TACHO_POLES 12 // Default number of poles for tachometer
#endif
#ifndef TACHO_RPM_RATIO
#define TACHO_RPM_RATIO 1.5 // Default ratio for tachometer RPM calculation
#endif
#ifndef TACHO_RPM_ADJUSTMENT
#define TACHO_RPM_ADJUSTMENT 0.0 // Default adjustment for tachometer RPM calculation
#endif

#define MAX_RETRY 3 // number of retries to start an agent
#define N2K_BLINK_USEC 100000L /* micros */

#define DEFAULT_USE_GPS 0
#define DEFAULT_USE_BMP 0
#define DEFAULT_USE_BME 0
#define DEFAULT_USE_DHT 0
#define DEFAULT_SOG_2_STW 0
#define DEFAULT_USE_TIME 0
#define DEFAULT_USE_TACHO 0
#define DEFAULT_USE_VE_DIRECT 0
#define DEFAULT_RPM_ADJUSTMENT 1.00
#define DEFAULT_GPS_SPEED UART_SPEED_57600
#define DEFAULT_N2K_SOURCE 22

#define DEFAULT_BATTERY_CAPACITY 280

#define APP_LOG_TAG "APP"

#define BLE_DEFAULT_SERVICE_NAME "N2KRouter"
#define BLE_SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BLE_CONF_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BLE_DATA_UUID "55da66c7-801f-498d-b652-c57cb3f1b590"
#define BLE_COMMAND_UUID "68ad1094-0989-4e22-9f21-4df7ef390803"

//ble values
static const int32_t INVALID_32 = 0xFFFFFF7F;
static const uint32_t INVALID_U32 = 0xFFFFFFFF;
static const int16_t INVALID_16 = 0xFF7F;
static const uint16_t INVALID_U16 = 0xFFFF;

// EnvMessenger period
#define PERIOD_MICROS_ENV 2000000

#endif /* _CONTANTS_H_ */