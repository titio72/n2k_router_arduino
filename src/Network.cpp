#include "constants.h"
#include "Network.h"

#include <string.h>

#ifdef ESP32_ARCH
#include <WiFi.h>
#include "WiFiManager.h"
#include "WebServer.h"

WiFiManager wifi;
WEBServer webui;

NetworkHub::NetworkHub(unsigned int p, const char* d, data* cache, Configuration* conf, statistics* stats, N2K* n2k) {
    port = p;
    destination = strdup(d);
    webui.setup(cache, conf, stats, n2k);
}

NetworkHub::~NetworkHub() {
    delete destination;
}

bool NetworkHub::begin() {
    wifi.start();
    return true;
}

bool NetworkHub::end() {
    // TODO
    return true;
}

bool NetworkHub::send_udp(const char* message, unsigned int len) {
    return wifi.sendUDPPacket(message, len)==0;
}

void NetworkHub::loop(unsigned long t) {
    wifi.loop(t);
    webui.loop(t);
}
#else
#include "UDPServer.h"

UDPSrv srv(UDP_PORT, UDP_DEST);

NetworkHub::NetworkHub(unsigned int p, const char* d, data* cache, configuration* conf, statistics* stats, N2K* n2k) {
    port = p;
    destination = strdup(d);
}

NetworkHub::~NetworkHub() {
    delete destination;
}

bool NetworkHub::begin() {
    return true;
}

bool NetworkHub::end() {
    // TODO
    return true;
}

bool NetworkHub::send_udp(const char* message, unsigned int len) {
    srv.send(message);
}

void NetworkHub::loop(unsigned long t) {
}

#endif
