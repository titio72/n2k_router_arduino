#ifdef ESP32_ARCH
#include <Arduino.h>
#include <WiFi.h>
#else
#include <stdio.h>
#include <arpa/inet.h>
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <netinet/tcp.h>
#endif
#include "Constants.h"
#include "WebServer.h"
#include "Utils.h"
#include "Log.h"
#include "N2K.h"
#include "Conf.h"

#pragma region PAGE
const char *page_head = "<head>  \
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
</head>";
#pragma endregion

#ifdef ESP32_ARCH
WiFiServer server(80);
WiFiClient *serving_client;
#else
int server_socket;
struct sockaddr_in server_address;

class SocketClient
{
public:
    int socket;

    SocketClient(int _socket): socket(_socket)
    {}

    int write(const char* to_send)
    {
        size_t res = ::write(socket, to_send, strlen(to_send));
        if (res==-1)
        {
            Log::trace("[WEB] Error errno{%d}\n", errno);
        }
        return res;
    }
};
#endif

unsigned long client_connected_time = 0;
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

bool enable_service(bool &cfg, const char *service, const char *command)
{
    if (strstr(command, service) && strstr(command, "able_"))
    {
        bool enable = strstr(command, "enable");
        Log::trace("[WEB] %s %s\n", enable ? "Enabling" : "Disabling", service);
        cfg = enable;
        return true;
    }
    else
    {
        return false;
    }
}

bool set_uart(uint8_t &cfg, const char *command)
{
    for (int i = 0; i < UART_SPEEDS; i++)
    {
        char temp[16];
        sprintf(temp, "set_uart_%s", UART_SPEED[i]);
        if (strcmp(temp, command) == 0)
        {
            cfg = i;
            return true;
        }
    }
    return false;
}

bool setDHT(uint8_t &cfg, const char *command)
{
    if (strcmp("set_dht11", command) == 0)
    {
        cfg = CONF_DHT11;
        return true;
    }
    else if (strcmp("set_dht22", command) == 0)
    {
        cfg = CONF_DHT22;
        return true;
    }
    else
    {
        return false;
    }
}

char *read_param(const char *param, const char *s, int len)
{
    for (int i = 0; i < len; i++)
    {
        char *start = (char *)s + i;
        if (strncmp(start, param, strlen(param)) == 0)
        {
            start += (strlen(param) + 1);
            return start;
        }
    }
    return NULL;
}

bool manage(const char *header, Configuration &c, N2K &n2k)
{
    // Log::trace("%s\n", header.c_str());
    // GET /?SYS_TIME HTTP/1.1
    Log::trace("[WEB] -----------\n%s\n[WEB] --------------\n", header);
    int i = indexOf(header, "HTTP");
    if (i != -1)
    {
        char *command = new char[i];
        memcpy(command, header + 5, i - 6);
        command[i - 6] = 0;
        Log::trace("[WEB] handling command {%s}\n", command);

        bool showPage = true;

        char *temp;
        if ((temp = strstr(command, "message")))
        {
            showPage = false;
            // GET /message?pgn=123456&dest=255&priority=6&data=A1A2A3A4A5A6A7A8 HTTP/1.1
            int len = strlen(temp);
            char *x = temp + 8 * sizeof(char);
            for (int i = 0; i < len; i++)
            {
                if (x[i] == '&')
                    x[i] = 0;
            }
            temp = read_param("pgn", x, len);
            ulong pgn = temp ? atol(temp) : 0;
            temp = read_param("priority", x, len);
            int priority = temp ? atoi(temp) : 8;
            temp = read_param("dest", x, len);
            int dest = temp ? atoi(temp) : 255;
            temp = read_param("data", x, len);
            int data_len = 0;
            unsigned char *data = NULL;
            if (temp)
            {
                data_len = strlen(temp) / 2;
                data = new unsigned char[data_len];
                char bf[3];
                for (int i = 0; i < strlen(temp); i += 2)
                {
                    bf[0] = temp[i];
                    bf[1] = temp[i + 1];
                    bf[2] = 0;
                    unsigned char b = (unsigned char)strtol(bf, NULL, 16);
                    data[i / 2] = b;
                }
            }
            Log::trace("Request to send message pgn {%d} dest {%d} data {%s}\n", pgn, dest, temp);
            Log::trace("Message sent {%d}\n", n2k.sendMessage(dest, pgn, priority, data_len, data));
        }
        else if (
            enable_service(c.send_time, "systime", command) ||
            enable_service(c.use_bmp280, "bmp280", command) ||
            enable_service(c.use_dht11, "dht11", command) ||
            enable_service(c.use_gps, "gps", command) ||
            setDHT(c.dht11_dht22, command) ||
            set_uart(c.uart_speed, command))
        {
            c.save();
        }
        else
        {
            enable_service(c.simulator, "simulator", command);
            enable_service(c.sim_position, "sim_position", command);
            enable_service(c.sim_sogcog, "sim_sogcog", command);
            enable_service(c.sim_wind, "sim_wind", command);
            enable_service(c.sim_heading, "sim_heading", command);
            enable_service(c.sim_speed, "sim_speed", command);
            enable_service(c.sim_pressure, "sim_pressure", command);
            enable_service(c.sim_humidity, "sim_humidity", command);
            enable_service(c.sim_temperature, "sim_temperature", command);
            enable_service(c.sim_water_temperature, "sim_wtemperature" , command);
            enable_service(c.sim_satellites, "sim_satellites", command);
            enable_service(c.sim_dops, "sim_dops", command);
            enable_service(c.sim_attitude, "sim_attitude", command);
            enable_service(c.sim_depth, "sim_depth", command);
            enable_service(c.sim_nav, "sim_nav", command);
        }
        free(command);
        return showPage;
    }
    else
    {
        return false;
    }
}

template <typename T>
T& operator<<(T& client, const char* res)
{
    client.write(res);
    return client;
}

template<typename T>
T& write_header(T& client, const char* content_type)
{
    return client << "HTTP/1.1 200 OK\nContent-type:" << content_type << "\nConnection: close\n\n";
}

template<typename T>
T& write(T& client, const char* txt)
{
    return client << txt;
}

template<typename T>
T& write_cell_u(T& client, const char* label, const char* url, const char* value)
{
    return client << "<td class='conf_cell'>" << label << "</td><td class='conf_value'><a href='" << url << "'>" << value << "</a></td>";
}

template<typename T>
T& write_cell_s(T &client, const char* label, const char* value)
{
    return client << "<td class='conf_cell'>" << label << "</td><td class='conf_value'>" << value << "</td>";
}

template<typename T>
T& write_2cell_s(T &client, const char* label, const char* value)
{
    return client << "<td class='conf_cell'>" << label << "</td><td colspan='3' class='conf_value'>" << value << "</td>";
}

template<typename T, typename V>
T& write_cell_v(T& client, const char* label, const char* format, V value)
{
    char c[64];
    sprintf(c, format, value);
    return write_cell_s(client, label, c);
}

template<typename T>
void generate_page(T& client, Configuration &conf, statistics &stats, data &cache)
{
    char tmp[32];
    write_header(client, "text/html");
    write(client, "<!DOCTYPE html>\n");
    write(client, "<html>");
    write(client, page_head);
    write(client, "<body>");
    write(client, "<h1>ABoni N2K</h1>  \
        <div style='align-content: center;'>  \
                <table class='center'>  \
            <tr>  \
                <th class='conf_cell'>Conf</th>  \
                <th class='conf_value'>Setting</th>  \
                <th class='conf_cell'>Item</th>  \
                <th class='conf_value'>Value</th>  \
            </tr>");
    write(client, "<tr>");
    write_cell_u(client, "GPS", conf.use_gps ? "disable_gps" : "enable_gps", conf.use_gps ? "Yes" : "No");
    write_cell_v(client, "Device Temp", "%.1f C&deg;", cache.temperature_el);
    write(client, "</tr><tr>");
    write_cell_u(client, "BMPxxx", conf.use_bmp280 ? "disable_bmp280" : "enable_bmp280", conf.use_bmp280 ? "Yes" : "No");
    write_cell_v(client, "Pressure", "%.1f MB", cache.pressure / 100.0);
    write(client, "</tr><tr>");
    write_cell_u(client, "DHTxx", conf.use_dht11 ? "disable_dht11" : "enable_dht11", conf.use_dht11 ? "Yes" : "No");
    write_cell_v(client, "Cabint Temp", "%.1f C&deg;", cache.temperature);
    write(client, "</tr><tr>");
    write_cell_u(client, "Sys Time", conf.send_time ? "disable_systime" : "enable_systime", conf.send_time ? "Yes" : "No");
    write_cell_v(client, "Humidity", "%.1f %%", cache.humidity);
    write(client, "</tr><tr>");
    write_cell_u(client, "DHT Type", conf.dht11_dht22 ? "set_dht11" : "set_dht22", DHTxx[conf.dht11_dht22]);
    write_cell_v(client, "CAN Rx", "%lu", stats.can_received);
    write(client, "</tr><tr>");
    sprintf(tmp, "set_uart_%s", UART_SPEED[(conf.uart_speed + 1) % UART_SPEEDS]);
    write_cell_u(client, "GPS UART", tmp, UART_SPEED[conf.uart_speed]);
    write_cell_v(client, "CAN Tx", "%lu", stats.can_sent);
    write(client, "</tr><tr>");
    format_thousands_sep(tmp, get_free_mem());
    write_cell_s(client, "Heap", tmp);
    write_cell_v(client, "CAN Tx Fail", "%u", stats.can_failed);
    write(client, "</tr><tr>");
    write_cell_u(client, "Simulator", conf.simulator?"disable_simulator":"enable_simulator",  conf.simulator?"Yes":"No");
    write_cell_v(client, "GPS Fix", "%u", stats.gps_fix);
    if (conf.simulator)
    {
        write(client, "</tr><tr>");
        write_cell_u(client, "Sim Position", conf.sim_position?"disable_sim_position":"enable_sim_position", conf.sim_position?"Yes":"No");
        write_cell_u(client, "Sim SOG-COG", conf.sim_sogcog?"disable_sim_sogcog":"enable_sim_sogcog", conf.sim_sogcog?"Yes":"No");
        write(client, "</tr><tr>");
        write_cell_u(client, "Sim Wind", conf.sim_wind?"disable_sim_wind":"enable_sim_wind", conf.sim_wind?"Yes":"No");
        write_cell_u(client, "Sim Heading", conf.sim_heading?"disable_sim_heading":"enable_sim_heading", conf.sim_heading?"Yes":"No");
        write(client, "</tr><tr>");
        write_cell_u(client, "Sim Speed", conf.sim_speed?"disable_sim_speed":"enable_sim_speed", conf.sim_speed?"Yes":"No");
        write_cell_u(client, "Sim Pressure", conf.sim_pressure?"disable_sim_pressure":"enable_sim_pressure", conf.sim_pressure?"Yes":"No");
        write(client, "</tr><tr>");
        write_cell_u(client, "Sim Humidity", conf.sim_humidity?"disable_sim_humidity":"enable_sim_humidity", conf.sim_humidity?"Yes":"No");
        write_cell_u(client, "Sim Temperature", conf.sim_temperature?"disable_sim_temperature":"enable_sim_temperature", conf.sim_temperature?"Yes":"No");
        write(client, "</tr><tr>");
        write_cell_u(client, "Sim Water Temp.", conf.sim_water_temperature?"disable_sim_wtemperature":"enable_sim_wtemperature", conf.sim_water_temperature?"Yes":"No");
        write_cell_u(client, "Sim Satellites", conf.sim_satellites?"disable_sim_satellites":"enable_sim_satellites", conf.sim_satellites?"Yes":"No");
        write(client, "</tr><tr>");
        write_cell_u(client, "Sim DOPS", conf.sim_dops?"disable_sim_dops":"enable_sim_dops", conf.sim_dops?"Yes":"No");
        write_cell_u(client, "Sim Attitude", conf.sim_attitude?"disable_sim_attitude":"enable_sim_attitude", conf.sim_attitude?"Yes":"No");
        write(client, "</tr><tr>");
        write_cell_u(client, "Sim Depth", conf.sim_depth?"disable_sim_depth":"enable_sim_depth", conf.sim_depth?"Yes":"No");
        write_cell_u(client, "Sim Nav", conf.sim_nav?"disable_sim_nav":"enable_sim_nav", conf.sim_nav?"Yes":"No");
    }
    write(client, "</tr><tr>");
    sprintf(tmp, "%4d-%02d-%02d %02d:%02d:%02d", cache.rmc.y, cache.rmc.M, cache.rmc.d, cache.rmc.h, cache.rmc.m, cache.rmc.s);
    write_2cell_s(client, "UTC", tmp);
    write(client, "</tr>");
    write(client, "</table>");
    write(client, "<p><a href='/'>Refresh</a></p>");
    write(client, "</body></html>\n\n\n");
}

template<typename T>
void generate_ok(T& client)
{
    write_header(client, "application/json");
    write(client, "{\"res\":\"ok\"}\n\n\n");
}

#ifdef ESP32_ARCH
String get_client_header(WiFiClient &serving_client, unsigned long ms)
{
    if (serving_client)
    {
        currentTime = ms;
        previousTime = currentTime;
        String currentLine = "";
        String header = "";
        while (serving_client.connected() && currentTime - previousTime <= timeoutTime)
        {
            currentTime = millis();
            if (serving_client.available())
            {
                char c = serving_client.read();
                header += c;
                if (c == '\n')
                {
                    if (currentLine.length()==0) // stop when there's an empty line
                        return header;
                    else
                        currentLine = "";
                }
                else if (c != '\r')
                {                     // if you got anything else but a carriage return character,
                    currentLine += c; // add it to the end of the currentLine
                }
            }
        }
    }
    return "";
}

void handle_client(WiFiClient &serving_client, unsigned long ms, Context& ctx)
{
    String header = get_client_header(serving_client, ms);
    if (header.length()!=0)
    {
        bool sendPage = manage(header.c_str(), ctx.conf, ctx.n2k);
        if (sendPage)
            generate_page(serving_client, ctx.conf, ctx.stats, ctx.cache);
        else
            generate_ok(serving_client);
    }
}
#else
void handle_client(int socket, unsigned long ms, Context& ctx)
{
    static char buffer[48];
    int bytes_received;
    if ((bytes_received = recv(socket, buffer, sizeof(buffer)-1, MSG_DONTWAIT)) > 0)
    {
        SocketClient client(socket);
        buffer[bytes_received] = 0;

        if (manage(buffer, ctx.conf, ctx.n2k))
            generate_page(client, ctx.conf, ctx.stats, ctx.cache);
        else
            generate_ok(client);
   }
}
#endif

WEBServer::WEBServer(Context _ctx): ctx(_ctx), started(false)
{}

void WEBServer::setup()
{}

void WEBServer::loop(unsigned long ms)
{
#ifdef ESP32_ARCH
    if (!started)
    {
        Log::trace("[WEB] Initializing web interface\n");
        if (WiFi.status() == WL_CONNECTED)
        {
            server.begin();
            started = true;
        }
    }
    else
    {
        started = (WiFi.status() == WL_CONNECTED);
    }

    WiFiClient client = server.available(); // Listen for incoming clients
    if (client)
    { // If a new client connects,
        Log::trace("New Client %s\n", client.localIP().toString().c_str());
        handle_client(client, ms, ctx);
        client.stop();
        Log::trace("Client disconnected.\n");
    }
#else
    struct sockaddr_in client_address;
    socklen_t client_address_len;

    int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
    if (client_socket >= 0)
    {
        Log::trace("[WEB] New Client {%s:%d}\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
        handle_client(client_socket, ms, ctx);
        shutdown(client_socket, SHUT_WR);
        close(client_socket);
        Log::trace("[WEB] Client disconnected\n");
    }
#endif
}

void WEBServer::enable()
{
#ifndef ESP32_ARCH
    if (!started)
    {
        Log::trace("[WEB] Server starting on port %d\n", DEFAULT_SERVER_PORT);
        // Create a socket
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0)
        {
            perror("socket");
            exit(1);
        }

        int reuseaddr = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0) {
            perror("setsockopt");
            close(server_socket);
            exit(1);
        }

        // Bind the socket to an address
        fcntl(server_socket, F_SETFL, O_NONBLOCK);
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(DEFAULT_SERVER_PORT);
        server_address.sin_addr.s_addr = INADDR_ANY;
        if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        {
            perror("bind");
            exit(1);
        }

        //Listen for connections
        if (listen(server_socket, 5) < 0)
        {
            perror("listen");
            exit(1);
        }

        started = true;
        Log::trace("[WEB] Server started on port %d\n", DEFAULT_SERVER_PORT);
    }
#endif
}

void WEBServer::disable()
{
#ifndef ESP32_ARCH
    if (started)
    {
        Log::trace("[WEB] Server stopping on port %d\n", DEFAULT_SERVER_PORT);

        if (server_socket)
        {
            shutdown(server_socket, SHUT_RDWR);
            close(server_socket);
            server_socket = -1;
        }
        started = false;
        Log::trace("[WEB] Server stopped on port %d\n", DEFAULT_SERVER_PORT);
    }
#endif
}

bool WEBServer::is_enabled()
{
    return started;
}