#ifndef _CONF_H
#define _CONF_H

#define B4800 0
#define B9600 1
#define B19200 2
#define B38400 3
#define B57600 4
#define B115200 5

#define CONF_DHT11 0
#define CONF_DHT22 1

static const int UART_SPEEDS = 6;
static const char* UART_SPEED[] = {"4800", "9600", "19200", "38400", "57600", "115200"};
static const char* DHTxx[] = {"DHT11", "DHT22"};

class Configuration {

public:
  Configuration();
  ~Configuration();

  int save();

  int load();

public:
  bool use_gps = true;
  bool use_bmp280 = false;
  bool use_dht11 = false;
  bool send_time = true;
  bool wifi_broadcast = true;
  unsigned char dht11_dht22 = CONF_DHT11;
  unsigned char uart_speed = B9600;
  unsigned char src = ABSOURCE;
};


#endif