#include <Arduino.h> // needed for the WiFi communication
#include <WiFi.h>    // needed for the WiFi communication
#include "WiFiManager.h"
#include "constants.h"
#include "Utils.h"
#include "Log.h"

#define WIFI_TIMEOUT_MS 20000      // 20 second WiFi connection timeout
#define WIFI_RECOVER_TIME_MS 30000 // Wait 30 seconds after a failed connection attempt

static bool connecting = false;

bool WiFiManager::is_connected()
{
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::start()
{
  static ulong last_try = 0;
  if (WiFi.status()!=WL_CONNECTED && (last_try==0 || (millis()-last_try)>WIFI_RECOVER_TIME_MS)) {
    Log::trace("[WIFI] Connecting {%s}\n", MY_SSID);
    WiFi.begin(MY_SSID, MY_PSWD);
    connecting = true;
    ulong t0 = millis();
    last_try = t0;
    while (WiFi.status()!=WL_CONNECTED || (millis()-t0)>WIFI_TIMEOUT_MS) {
      delay(500);
      Log::trace("[WIFI] Connecting....\n");
    }
    if (WiFi.status()==WL_CONNECTED)
      Log::trace("[WIFI] Connected {%s}\n", WiFi.localIP().toString().c_str());
    else 
      Log::trace("[WIFI] Connection failed {%s}\n", MY_SSID);
  }
}

void WiFiManager::loop(unsigned long ms)
{
  start();
}

int WiFiManager::sendUDPPacket(const char *bfr, unsigned int l)
{
  if (is_connected())
  {
    static uint8_t term[] = {13, 10};
    udp.beginPacket(UDP_DEST, UDP_PORT);
    udp.write((const uint8_t *)bfr, l);
    udp.write(term, 2);
    int rs = udp.endPacket();
    if (TRACE_UDP)
      Log::trace("UDP %s {%s}\n", rs ? "Sent" : "Fail", bfr);
    return rs;
  }
  else
  {
    Log::trace("Cannot send UDP message while disconnected\n");
    return 0;
  }
}
