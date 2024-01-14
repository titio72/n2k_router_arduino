#include <Arduino.h> // needed for the WiFi communication
#include <WiFi.h>    // needed for the WiFi communication
#include "WiFiManager.h"
#include "Constants.h"
#include "Utils.h"
#include "Log.h"

#define WIFI_TIMEOUT_MS 15000      // 20 second WiFi connection timeout
#define WIFI_RECOVER_TIME_MS 60000 // Wait 30 seconds after a failed connection attempt

bool WiFiManager::is_connected()
{
  return conn_stat == 2;
}

void WiFiManager::start(ulong t)
{
  static ulong last_check = 0;
  static ulong last_try = 0;
  if (conn_stat==0) {
    Log::trace("[WIFI] Connecting {%s}\n", MY_SSID);
    WiFi.begin(MY_SSID, MY_PSWD);
    last_try = t;
    conn_stat = 1;
  } else if (conn_stat==1) {
    if ((t-last_check)>500) {
      last_check = t;
      if (WiFi.status()==WL_CONNECTED) {
        Log::trace("[WIFI] Connected {%s}\n", WiFi.localIP().toString().c_str());
        conn_stat = 2;
      } else if ((t-last_try)>WIFI_TIMEOUT_MS) {
        Log::trace("[WIFI] Timeout\n");
        WiFi.disconnect();
        last_try = t;
        conn_stat = 0;
      }
    }
  } else if (conn_stat==2) {
    if ((t-last_check)>500 && WiFi.status()!=WL_CONNECTED) {
      Log::trace("[WIFI] disconnected\n");
      WiFi.disconnect();
      conn_stat = 0;
    }
  }
}

void WiFiManager::end()
{
   WiFi.disconnect();
}

void WiFiManager::loop(unsigned long ms)
{
  start(ms);
}