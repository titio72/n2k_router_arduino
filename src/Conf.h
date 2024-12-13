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
#define DEFAULT_RPM_ADJUSTMENT 1.00
#define DEFAULT_GPS_SPEED UART_SPEED_57600
#define DEFAULT_N2K_SOURCE 22
#define DEFAULT_DEVICE_NAME "ABN2K"


#define GPS_ID 0
#define DHT_ID 1
#define BMP_ID 2
#define SYT_ID 3
#define RPM_ID 4
#define STW_ID 5

#define MAX_CONF 6

class N2KServices
{
public:
  void deserialize(uint8_t v);
  uint8_t serialize();

  bool from_string(const char* string);
  const char* to_string();

  N2KServices& operator =(const N2KServices &svc);

  bool use_gps = DEFAULT_USE_GPS;
  bool use_bmp = DEFAULT_USE_BMP;
  bool use_dht = DEFAULT_USE_DHT;
  bool send_time = DEFAULT_USE_TIME;
  bool sog_2_stw = DEFAULT_SOG_2_STW;
  bool use_tacho = DEFAULT_USE_TACHO;
  char buffer[MAX_CONF+1];
};

class Configuration {

public:
  Configuration();
  ~Configuration();

  void init();

  uint64_t get_engine_hours();
  void save_engine_hours(uint64_t h);

  double get_rpm_adjustment();
  void save_rpm_adjustment(double d);

  const char* get_device_name();
  void save_device_name(const char* name);

  unsigned char get_n2k_source();
  void save_n2k_source(unsigned char src);

  unsigned char get_uart_speed();
  void save_uart_speed(unsigned char speed);

  N2KServices& get_services();
  void save_services(N2KServices& s);

private:

  char cache_device_name[16];
  N2KServices cache_services;
  double cache_rpm_adj;
  bool initialized;
};


#endif