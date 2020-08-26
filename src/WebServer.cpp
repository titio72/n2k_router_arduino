
#include <Arduino.h>
#include <WiFi.h>
#include <string.h>
#include <EEPROM.h>
#include "constants.h"
#include "Utils.h"
#include "Log.h"
#include "WebServer.h"
#include "N2K.h"

String page("<!DOCTYPE html>  \
<html>  \
  \
<head>  \
    <meta name='viewport' content='width=device-width, initial-scale=1'>  \
    <link rel='icon' href='data:,'>  \
    <style>  \
        html {  \
            font-family: Helvetica;  \
            display: inline-block;  \
            margin: 0px auto;  \
            text-align: center;  \
        }  \
  \
        .conf_cell {  \
            text-align: left;  \
            font-weight: bolder;  \
        }  \
  \
        .conf_value {  \
            text-align: right;  \
        }  \
  \
        table, th, td {  \
            border-collapse: collapse;  \
            border: 1px solid black;  \
            padding: 10px; \
        }  \
          \
        table.center {  \
            margin-left:auto;   \
            margin-right:auto;  \
        }  \
  \
        th {  \
            background-color: gray;  \
        }  \
    </style>  \
</head>  \
  \
<body>  \
    <h1>ABoni N2K</h1>  \
    <div style='align-content: center;'>  \
        <table class='center'>  \
            <tr>  \
                <th class='conf_cell'>Conf</th>  \
                <th class='conf_value'>Setting</th>  \
                <th class='conf_cell'>Item</th>  \
                <th class='conf_value'>Value</th>  \
            </tr>  \
            <tr> \
                <td class='conf_cell'>GPS</td> \
                <td class='conf_value'><a href='__GPS_URL__'>__GPS__</a></td> \
                <td class='conf_cell'>Pressure</td>  \
                <td class='conf_value'>__PRESSURE__</td>  \
            </tr> \
            <tr> \
                <td class='conf_cell'>BMP280</td> \
                <td class='conf_value'><a href='__BMP280_URL__'>__BMP280__</a></td> \
                <td class='conf_cell'>Temp</td>  \
                <td class='conf_value'>__TEMPERATURE__</td>  \
            </tr> \
            <tr> \
                <td class='conf_cell'>DHTxx</td> \
                <td class='conf_value'><a href='__DHT_URL__'>__DHT__</a></td> \
                <td class='conf_cell'>Humidity</td>  \
                <td class='conf_value'>__HUMIDITY__</td>  \
            </tr> \
            <tr> \
                <td class='conf_cell'>Sys Time</td> \
                <td class='conf_value'><a href='__SYS_TIME_URL__'>__SYS_TIME__</a></td> \
                <td class='conf_cell'>CAN BUS RX</td>  \
                <td class='conf_value'>__RX__</td>  \
            </tr>  \
            <tr>  \
                <td class='conf_cell'>DHT Type</td> \
                <td class='conf_value'><a href='__DHT_TYPE_URL__'>__DHT_TYPE__</a></td> \
                <td class='conf_cell'>CAN BUS TX</td>  \
                <td class='conf_value'>__TX__ (__TX_ERR__)</td>  \
            </tr>  \
            <tr>  \
                <td class='conf_cell'>GPS UART</td> \
                <td class='conf_value'><a href='__GPS_UART_URL__'>__GPS_UART__</a></td> \
                <td class='conf_cell'>UDP TX</td>  \
                <td class='conf_value'>__UDP_TX__ (__UDP_TX_ERR__)</td>  \
            </tr>  \
            <tr>  \
                <td class='conf_cell'>GPS Fix</td> \
                <td class='conf_value'>__GPS_FIX__</td> \
                <td class='conf_cell'>&nbsp;</td>  \
                <td class='conf_value'>&nbsp;</td>  \
            </tr>  \
            <tr>  \
                <td class='conf_cell'>Internal Temp.</td> \
                <td class='conf_value'>__INTERNAL_TEMP__</td> \
                <td class='conf_cell'>Feee heap</td>  \
                <td class='conf_value'>__FREE_HEAP__</td>  \
            </tr>  \
        </table>  \
        <p><a href='/'>Refresh</a></p> \
    </div>  \
</body>  \
  \
</html>");


char* replace_and_free(char* orig, const char* pattern, const char* new_string, bool first) {
    char* c = replace(orig, pattern, new_string, first);
    free(orig);
    return c;
}


bool enable(bool& cfg, const char* service, const char* command) {
    if (strstr(command, service) && strstr(command, "able_")) {
        bool enable = strstr(command, "enable");
        Log::trace("%s %s\n", enable?"Enabling":"Disabling", service);
        cfg = enable;
        return true;
    } else {
        return false;
    }
}

bool set_uart(uint8_t& cfg, const char* command)
{
    for (int i = 0; i<UART_SPEEDS; i++) 
    {
        char temp[16];
        sprintf(temp, "set_uart_%s", UART_SPEED[i]);        
        if (strcmp(temp, command)==0) {
            cfg = i;
            return true;
        }
    }
    return false;
}

bool setDHT(uint8_t& cfg, const char* command) {
    if (strcmp("set_dht11", command)==0) {
        cfg = DHT11;
        return true;
    } else if (strcmp("set_dht22", command)==0) {
        cfg = DHT22;
        return true;
    } else {
        return false;
    }
}

char* read_param(const char* param, const char* s, int len) {
    for (int i = 0; i<len; i++) {
        char* start = (char*)s + i;
        if (strncmp(start, param, strlen(param))==0) {
            start += (strlen(param) + 1);
            return start;
        }
    }
    return NULL;
}

bool manage(String& header, Configuration* c, N2K* n2k) {
    //Log::trace("%s\n", header.c_str());
    //GET /?SYS_TIME HTTP/1.1
    int i = header.indexOf("HTTP");
    char* command = new char[i];
    memcpy(command, header.c_str() + 5, i - 6);
    command[i-6] = 0;
    Log::trace("Web UI {%s}\n", command);

    bool showPage = true;

    char* temp;
    if ((temp=strstr(command, "message"))) {
        showPage = false;
    //GET /message?pgn=123456&dest=255&priority=6&data=A1A2A3A4A5A6A7A8 HTTP/1.1
        int len = strlen(temp);
        char* x = temp + 8 * sizeof(char);
        for (int i = 0; i<len; i++) {
            if (x[i]=='&') x[i] = 0;
        }
        temp = read_param("pgn", x, len);
        ulong pgn = temp?atol(temp):0;
        
        temp = read_param("priority", x, len);
        int priority = temp?atoi(temp):8;
        
        temp = read_param("dest", x, len);
        int dest = temp?atoi(temp):255;
        
        temp = read_param("data", x, len);
        int data_len = 0;
        unsigned char* data = NULL;
        if (temp) {
            data_len = strlen(temp)/2;
            data = new unsigned char[data_len];
            char bf[3];
            for (int i = 0; i<strlen(temp); i+=2) {
                bf[0] = temp[i]; bf[1] = temp[i+1]; bf[2] = 0;
                unsigned char b = (unsigned char)strtol(bf, NULL, 16);
                data[i/2] = b;
            }
        }
        Log::trace("Request to send message pgn {%d} dest {%d} data {%s}\n", pgn, dest, temp);
        Log::trace("Message sent {%d}\n", n2k->sendMessage(dest, pgn, priority, data_len, data));
    } else if (enable(c->send_time, "systime", command) || enable(c->use_bmp280, "bmp280", command) || 
            enable(c->use_dht11, "dht11", command) || enable(c->use_gps, "gps", command) ||
            setDHT(c->dht11_dht22, command) || set_uart(c->uart_speed, command)) {
        c->save();
    }
    free(command);
    return showPage;
}


WiFiServer server(80);
WiFiClient* serving_client;
unsigned long client_connected_time = 0;

void WEBServer::setup(data* _cache, Configuration* _conf, statistics* _stats, N2K* _n2k) {
    cache = _cache;
    conf = _conf;
    stats = _stats;
    n2k = _n2k;
}

unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;
String header;


void WEBServer::handle_client(WiFiClient* serving_client, unsigned long ms) {
    if (serving_client) {
        currentTime = millis();
        previousTime = currentTime;
        Log::trace("New Client %s\n", serving_client->localIP().toString().c_str());
        String currentLine = "";
        while (serving_client->connected() && currentTime - previousTime <= timeoutTime) {
            currentTime = millis();
            if (serving_client->available()) {
                char c = serving_client->read();
                header += c;
                if (c == '\n') {
                    if (currentLine.length() == 0) {

                        // manage commands
                        bool sendPage = manage(header, conf, n2k);
                        
                        if (sendPage) {
                            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                            // and a content-type so the client knows what's coming, then a blank line:
                            serving_client->println("HTTP/1.1 200 OK");
                            serving_client->println("Content-type:text/html");
                            serving_client->println("Connection: close");
                            serving_client->println();

                            // fill page and send it to client
                            char buffer[128];
                            char* c;
                            c = replace(page.c_str(), "__GPS__", conf->use_gps?"Yes":"No", true);
                            c = replace_and_free(c, "__GPS_URL__", conf->use_gps?"disable_gps":"enable_gps", true);
                            c = replace_and_free(c, "__BMP280__", conf->use_bmp280?"Yes":"No", true);
                            c = replace_and_free(c, "__BMP280_URL__", conf->use_bmp280?"disable_bmp280":"enable_bmp280", true);
                            c = replace_and_free(c, "__DHT__", conf->use_dht11?"Yes":"No", true);
                            c = replace_and_free(c, "__DHT_URL__", conf->use_dht11?"disable_dht11":"enable_dht11", true);
                            
                            c = replace_and_free(c, "__DHT_TYPE__", DHTxx[conf->dht11_dht22], true);
                            c = replace_and_free(c, "__DHT_TYPE_URL__", conf->dht11_dht22?"set_dht11":"set_dht22", true);

                            char set_uart[16]; sprintf(set_uart, "set_uart_%s", UART_SPEED[(conf->uart_speed + 1)%UART_SPEEDS]);
                            c = replace_and_free(c, "__GPS_UART__", UART_SPEED[conf->uart_speed], true);
                            c = replace_and_free(c, "__GPS_UART_URL__", set_uart, true);
                            
                            c = replace_and_free(c, "__SYS_TIME__", conf->send_time?"Yes":"No", true);
                            c = replace_and_free(c, "__SYS_TIME_URL__", conf->send_time?"disable_systime":"enable_systime", true);
                            sprintf(buffer, "%.1f MB", cache->pressure/100.0); c = replace_and_free(c, "__PRESSURE__", buffer, true);
                            sprintf(buffer, "%.1f C&deg;", cache->temperature); c = replace_and_free(c, "__TEMPERATURE__", buffer, true);
                            sprintf(buffer, "%.1f %%", cache->humidity); c = replace_and_free(c, "__HUMIDITY__", buffer, true);
                            sprintf(buffer, "%lu", stats->can_received); c = replace_and_free(c, "__RX__", buffer, true);
                            sprintf(buffer, "%lu", stats->can_sent); c = replace_and_free(c, "__TX__", buffer, true);
                            sprintf(buffer, "%lu", stats->can_failed); c = replace_and_free(c, "__TX_ERR__", buffer, true);
                            sprintf(buffer, "%lu", stats->udp_sent); c = replace_and_free(c, "__UDP_TX__", buffer, true);
                            sprintf(buffer, "%lu", stats->udp_failed); c = replace_and_free(c, "__UDP_TX_ERR__", buffer, true);
                            sprintf(buffer, "%d", stats->gps_fix); c = replace_and_free(c, "__GPS_FIX__", buffer, true);
                            sprintf(buffer, "%.1f", cache->temperature_el); c = replace_and_free(c, "__INTERNAL_TEMP__", buffer, true);
                            sprintf(buffer, "%d", ESP.getFreeHeap()); c = replace_and_free(c, "__FREE_HEAP__", buffer, true);
                            serving_client->print(c); free(c);
                            serving_client->println();
                            serving_client->println();
                        }
                        else {
                            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                            // and a content-type so the client knows what's coming, then a blank line:
                            serving_client->println("HTTP/1.1 200 OK");
                            serving_client->println("Content-type:application/json");
                            serving_client->println("Connection: close");
                            serving_client->println();
                            serving_client->print("{\"res\":\"ok\"}");
                            serving_client->println();
                            serving_client->println();
                        }
                        break;
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {  // if you got anything else but a carriage return character,
                    currentLine += c;      // add it to the end of the currentLine
                }
            }
        }
        header = "";
        serving_client->stop();
        Log::trace("Client disconnected.\n");
    }
}

void WEBServer::loop(unsigned long ms) {

    if (!started) {
        Log::trace("[WEB] Initializing web interface\n");
        if (WiFi.status()==WL_CONNECTED) {
            server.begin();
            started = true;
        }
    } else {
        started = (WiFi.status()==WL_CONNECTED);
    }

    WiFiClient client = server.available();   // Listen for incoming clients
    if (client) {                             // If a new client connects,
        handle_client(&client, ms);
    }
}