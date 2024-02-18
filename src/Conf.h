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

#define DEFAULT_USE_GPS 0
#define DEFAULT_USE_BMP 0
#define DEFAULT_USE_DHT 0
#define DEFAULT_USE_TIME 0
#define DEFAULT_USE_NETWORK 1
#define DEFAULT_DHT 1
#define DEFAULT_GPS_SPEED 1
#define DEFAULT_SIMULATOR 0

class Configuration {

public:
  Configuration();
  ~Configuration();

  int save();

  int load();

public:
  bool use_gps;
  bool use_bmp280;
  bool use_dht11;
  bool send_time;
  bool simulator;
  bool network;
  unsigned char dht11_dht22;
  unsigned char uart_speed;

  bool sim_position           = false;
  bool sim_sogcog             = false;
  bool sim_wind               = true;
  bool sim_heading            = true;
  bool sim_speed              = true;
  bool sim_pressure           = true;
  bool sim_humidity           = true;
  bool sim_temperature        = true;
  bool sim_water_temperature  = true;
  bool sim_satellites         = false;
  bool sim_dops               = false;
  bool sim_attitude           = true;
  bool sim_depth              = true;
  bool sim_nav                = true;
};


#endif