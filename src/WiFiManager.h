#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

class WiFiManager {

public:
    void start(unsigned long ms);
    void end();
    void loop(unsigned long ms);
    bool is_connected();

private:
    uint8_t conn_stat = 0;                   // Connection status for WiFi:
                                             //
                                             // status |   WiFi   |
                                             // -------+----------+
                                             //      0 |   down   |
                                             //      1 | starting |
                                             //      2 |    up    |
};

#endif
