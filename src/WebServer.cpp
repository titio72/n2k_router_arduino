
#include <Arduino.h>
#include <WiFi.h>
#include "constants.h"
#include "Utils.h"
#include "WebServer.h"
#include <string.h>
#include <EEPROM.h>

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
        .button {  \
            background-color: #4CAF50;  \
            border: none;  \
            color: white;  \
            padding: 16px 40px;  \
            text-decoration: none;  \
            font-size: 30px;  \
            margin: 2px;  \
            cursor: pointer;  \
        }  \
  \
        .button2 {  \
            background-color: #555555;  \
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
                <td class='conf_cell'>DHT11</td> \
                <td class='conf_value'><a href='__DHT11_URL__'>__DHT11__</a></td> \
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
                <td>&nbsp;</td> \
                <td>&nbsp;</td> \
                <td class='conf_cell'>CAN BUS TX</td>  \
                <td class='conf_value'>__TX__ (__TX_ERR__)</td>  \
            </tr>  \
            <tr>  \
                <td>&nbsp;</td> \
                <td>&nbsp;</td> \
                <td class='conf_cell'>UDP TX</td>  \
                <td class='conf_value'>__UDP_TX__ (__UDP_TX_ERR__)</td>  \
            </tr>  \
            <tr>  \
                <td>&nbsp;</td> \
                <td>&nbsp;</td> \
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
    if (strstr(command, service)) {
        bool enable = strstr(command, "enable");
        debug_print("%s %s\n", enable?"Enabling":"Disabling", service);
        cfg = enable;
        return true;
    } else {
        return false;
    }
}


void manage(String& header, configuration* c) {
    //GET /?SYS_TIME HTTP/1.1
    int i = header.indexOf("HTTP");
    char* command = new char[i];
    memcpy(command, header.c_str(), i * sizeof(char));
    command[i-1] = 0;
    debug_print("Web UI {%s}\n", command);

    if (!enable(c->send_time, "systime", command))
        if (!enable(c->use_bmp280, "bmp280", command))
            if (!enable(c->use_dht11, "dht11", command))
                if (!enable(c->use_gps, "gps", command)) return;

    int new_conf = (c->use_gps?1:0) + (c->use_bmp280?2:0) + (c->use_dht11?4:0) + (c->send_time?8:0) + 16;
    debug_print("Writing new configuration {%d}\n", (uint8_t)new_conf);
    EEPROM.write(0, (uint8_t)new_conf);
    EEPROM.commit();
}


WiFiServer server(80);WiFiClient* serving_client;
unsigned long client_connected_time = 0;

void WEBServer::setup(data* _cache, configuration* _conf, statistics* _stats) {
    cache = _cache;
    conf = _conf;
    stats = _stats;

    if (!started) {
        debug_println("Initializing web interface");
        started = true;
        server.begin();
    }
    
}
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
String header;


void WEBServer::handle_client(WiFiClient* serving_client, unsigned long ms) {
    if (serving_client) {
        currentTime = millis();
        previousTime = currentTime;
        debug_print("New Client %s\n", serving_client->localIP().toString().c_str());          // print a message out in the serial port
        String currentLine = "";                // make a String to hold incoming data from the client
        while (serving_client->connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
            currentTime = millis();
            if (serving_client->available()) {             // if there's bytes to read from the client,
                char c = serving_client->read();             // read a byte, then
                header += c;
                if (c == '\n') {                    // if the byte is a newline character
                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0) {

                        manage(header, conf);

                        char buffer[128];

                        char* c;
                        c = replace(page.c_str(), "__GPS__", conf->use_gps?"Yes":"No", true);
                        c = replace_and_free(c, "__GPS_URL__", conf->use_gps?"disable_gps":"enable_gps", true);

                        c = replace_and_free(c, "__BMP280__", conf->use_bmp280?"Yes":"No", true);
                        c = replace_and_free(c, "__BMP280_URL__", conf->use_bmp280?"disable_bmp280":"enable_bmp280", true);

                        c = replace_and_free(c, "__DHT11__", conf->use_dht11?"Yes":"No", true);
                        c = replace_and_free(c, "__DHT11_URL__", conf->use_dht11?"disable_dht11":"enable_dht11", true);

                        c = replace_and_free(c, "__SYS_TIME__", conf->send_time?"Yes":"No", true);
                        c = replace_and_free(c, "__SYS_TIME_URL__", conf->send_time?"disable_systime":"enable_systime", true);

                        sprintf(buffer, "%.1f MB", cache->pressure/100.0);
                        c = replace_and_free(c, "__PRESSURE__", buffer, true);

                        sprintf(buffer, "%.1f C&deg;", cache->temperature);
                        c = replace_and_free(c, "__TEMPERATURE__", buffer, true);

                        sprintf(buffer, "%.1f %%", cache->humidity);
                        c = replace_and_free(c, "__HUMIDITY__", buffer, true);

                        sprintf(buffer, "%lu", stats->can_received);
                        c = replace_and_free(c, "__RX__", buffer, true);

                        sprintf(buffer, "%lu", stats->can_sent);
                        c = replace_and_free(c, "__TX__", buffer, true);

                        sprintf(buffer, "%lu", stats->can_failed);
                        c = replace_and_free(c, "__TX_ERR__", buffer, true);

                        sprintf(buffer, "%lu", stats->udp_sent);
                        c = replace_and_free(c, "__UDP_TX__", buffer, true);

                        sprintf(buffer, "%lu", stats->udp_failed);
                        c = replace_and_free(c, "__UDP_TX_ERR__", buffer, true);

                        uint32_t free_heap = ESP.getFreeHeap();
                        sprintf(buffer, "%d", free_heap);
                        c = replace_and_free(c, "__FREE_HEAP__", buffer, true);
                        
                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                        // and a content-type so the client knows what's coming, then a blank line:
                        serving_client->println("HTTP/1.1 200 OK");
                        serving_client->println("Content-type:text/html");
                        serving_client->println("Connection: close");
                        serving_client->println();
                        if (header.indexOf("GET /26/on") >= 0) {
                        // do something
                        }

                        serving_client->print(c);
                        serving_client->println();
                        free(c);

                        // The HTTP response ends with another blank line
                        serving_client->println();
                        // Break out of the while loop

                        break;
                    } else { // if you got a newline, then clear currentLine
                        currentLine = "";
                    }
                } else if (c != '\r') {  // if you got anything else but a carriage return character,
                    currentLine += c;      // add it to the end of the currentLine
                }
            }
        }
        // Clear the header variable
        header = "";
        // Close the connection
        serving_client->stop();
        debug_println("Client disconnected.");
    }

}

void WEBServer::on_loop(unsigned long ms) {
    WiFiClient client = server.available();   // Listen for incoming clients
    if (client) {                             // If a new client connects,
        handle_client(&client, ms);
    }
}