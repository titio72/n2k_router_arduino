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

#define DEFAULT_USE_GPS 0
#define DEFAULT_USE_BMP 0
#define DEFAULT_USE_DHT 0
#define DEFAULT_SOG_2_STW 0
#define DEFAULT_USE_TIME 0
#define DEFAULT_GPS_SPEED 2
#define DEFAULT_SIMULATOR 0
#define DEFAULT_N2K_SOURCE 22

class Configuration {

public:
  Configuration();
  ~Configuration();

  bool save();

  bool load();

public:
  bool use_gps;
  bool use_bmp;
  bool use_dht;
  bool send_time;
  bool sog_2_stw;
  bool simulator;
  unsigned char n2k_source;
  unsigned char uart_speed;

  bool sim_position           = true;
  bool sim_sogcog             = true;
  bool sim_time               = true;
  bool sim_wind               = true;
  bool sim_heading            = true;
  bool sim_speed              = true;
  bool sim_pressure           = true;
  bool sim_humidity           = true;
  bool sim_temperature        = true;
  bool sim_water_temperature  = true;
  bool sim_satellites         = true;
  bool sim_dops               = true;
  bool sim_attitude           = true;
  bool sim_depth              = true;
  bool sim_nav                = true;
};


#endif