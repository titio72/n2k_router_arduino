#ifndef CONSTANTS_H
#define CONSTANTS_H

//define ESP_C3

#ifdef ESP32_ARCH
    #ifdef ESP_C3
    #define RXD2 8
    #define TXD2 9
    #define I2C_SDA 4
    #define I2C_SCL 5
    #define DHTPIN 3
    #define LEDPIN1 2
    #define SERIAL_GPS Serial1
    #define ESP32_CAN_TX_PIN GPIO_NUM_7
    #define ESP32_CAN_RX_PIN GPIO_NUM_6
    #else
    #define RXD2 16
    #define TXD2 17
    #define I2C_SDA 21
    #define I2C_SCL 22
    #define DHTPIN 19
    #define LEDPIN1 15
    #define ESP32_CAN_TX_PIN GPIO_NUM_5 // Set CAN TX port to 5
    #define ESP32_CAN_RX_PIN GPIO_NUM_4 // Set CAN RX port to 4
    #define SERIAL_GPS Serial2
    #endif
#else
    #define SOCKET_CAN_PORT socket_name
    // #define SOCKET_CAN_PORT "vcan0"
#endif

#define MY_SSID "ABHalo"
#define MY_PSWD "z6UND5q7VT37Bmnz"

#define SEALEVELPRESSURE_HPA (1013.25)
#define WIFI 0

#define DEFAULT_N2K_SRC 22

#define DEFAULT_SERVER_PORT 5111

#define DEFAULT_GPS_DEVICE "/tmp/ttyV0"

#endif
