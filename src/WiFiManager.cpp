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