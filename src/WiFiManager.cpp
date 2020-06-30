#include <Arduino.h>                // needed for the WiFi communication
#include <WiFi.h>                // needed for the WiFi communication
#include "WiFiManager.h"
#include "constants.h"
#include "Utils.h"

bool WiFiManager::is_connected() {
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::start() {
  if (WiFi.status() != WL_CONNECTED) {
    if (conn_stat!=1) {
      WiFi.begin(SSID, PSWD);
      conn_stat = 1;
    }
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      debug_print("Connecting to WiFi.. %s\n", SSID);
    }
    debug_print("Connected to the WiFi network %s\n", WiFi.localIP().toString().c_str());   
    conn_stat = 2;
  }
}

void WiFiManager::on_loop(unsigned long ms) {       

}

int WiFiManager::sendUDPPacket(const uint8_t* bfr, int l) {
    if (is_connected()) {
      static uint8_t term[] = {13,10};
      udp.beginPacket(UDP_DEST, UDP_PORT);
      udp.write(bfr, l);
      udp.write(term, 2);
      int rs = udp.endPacket();
      if (TRACE_UDP) debug_print("UDP %s {%s}\n", rs?"Sent":"Fail", bfr);
      return rs;
    } else {
      debug_println("Cannot send UDP message while disconnected");
      return 0;
    }
}
