#include "Constants.h"
#include "Network.h"
#include "WebServer.h"
#include "Log.h"
#include <string.h>

#ifdef ESP32_ARCH
#include <WiFi.h>
#include "WiFiManager.h"
WiFiManager wifi;
#endif

void NetworkHub::enable()
{
    if (!enabled)
    {
        Log::trace("[NET] Starting network services\n");
#ifdef ESP32_ARCH
        wifi.start(millis());
#endif
        web_ui->enable();
        enabled = true;
        Log::trace("[NET] Network services started\n");
    }
}

void NetworkHub::disable()
{
    if (enabled)
    {
        Log::trace("[NET] Stopping network services\n");
#ifdef ESP32_ARCH
        wifi.end();
#endif
        web_ui->disable();
        enabled = false;
        Log::trace("[NET] Network services stopped\n");
    }
}

void NetworkHub::loop(unsigned long t)
{
#ifdef ESP32_ARCH
    wifi.loop(t);
    if (wifi.is_connected())
        web_ui->loop(t);
#else
    web_ui->loop(t);
#endif
}

NetworkHub::NetworkHub(Context _ctx): ctx(_ctx), enabled(false) 
{
    web_ui = new WEBServer(ctx);
}

NetworkHub::~NetworkHub() 
{
    delete web_ui;
}

bool NetworkHub::is_enabled()
{
    return enabled;
}

void NetworkHub::setup()
{
    web_ui->setup();
}