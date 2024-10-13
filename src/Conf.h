#ifndef _CONF_H
#define _CONF_H

static const int UART_SPEEDS = 6;
static unsigned int UART_SPEED[] = {4800, 9600, 19200, 38400, 57600, 115200};

static unsigned char UART_SPEED_4800   = 0;
static unsigned char UART_SPEED_9600   = 1;
static unsigned char UART_SPEED_19200  = 2;
static unsigned char UART_SPEED_38400  = 3;
static unsigned char UART_SPEED_57600  = 4;
static unsigned char UART_SPEED_115200 = 5;

#define DEFAULT_USE_GPS 0
#define DEFAULT_USE_BMP 0
#define DEFAULT_USE_DHT 0
#define DEFAULT_SOG_2_STW 0
#define DEFAULT_USE_TIME 0
#define DEFAULT_USE_TACHO 0
#define DEFAULT_RPM_CALIBRATION 1.00
#define DEFAULT_GPS_SPEED UART_SPEED_57600
#define DEFAULT_SIMULATOR 0
#define DEFAULT_N2K_SOURCE 22

class Configuration {

public:
  Configuration();
  ~Configuration();

  bool save();

  bool load();

  uint64_t get_engine_hours();

  void save_engine_hours(uint64_t h);

  double get_rpm_adjustment();

  void save_rpm_adjustment(double d);

public:
  bool use_gps;
  bool use_bmp;
  bool use_dht;
  bool send_time;
  bool sog_2_stw;
  bool use_tacho;
  bool simulator;
  unsigned char n2k_source;
  unsigned char uart_speed;
  double rpm_calibration;

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