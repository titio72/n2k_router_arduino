#ifndef _BTINTERFACE_H
#define _BTINTERFACE_H

#include <vector>
#include <BLEUUID.h>
#include <Arduino.h>

class BLEServer;
class BLEService;
class BLECharacteristic;
class BLECharacteristicCallbacks;
class BLEServerCallbacks;
class Configuration;

class ABBLEWriteCallback {
public:
    virtual void on_write(int handle, const char* value) = 0;
};

class ABBLESetting {
public:
    ABBLESetting(const char* n, const char* id): uuid(id)
    {
        strcpy(name, n);
    }

    char name[16];
    BLEUUID uuid;
};

class ABBLEField {
public:
    ABBLEField(const char* n, const char* id): uuid(id)
    {
        strcpy(name, n);
    }

    char name[16];
    BLEUUID uuid;
};

class BTInterface {
    public:
        BTInterface(const char* uuid, const char* device_name = NULL);
        ~BTInterface();
        void setup();
        void begin();
        void loop(unsigned long ms);

        void add_setting(const char* name, const char* uuid);
        void add_field(const char* name, const char* uuid);

        void set_setting_value(int handle, const char* value);
        void set_setting_value(int handle, int value);
        void set_field_value(int handle, const char* value);
        void set_field_value(int handle, void* value, int len);

        void set_write_callback(ABBLEWriteCallback* cback) { callback = cback; }

    private:
        char device_name[16];
        BLEUUID serviceUUID;
        BLEServer *pServer;
        BLEService *pService;
        BLECharacteristicCallbacks* listener;
        BLEServerCallbacks* serverCBack;

        ABBLEWriteCallback* callback;

        std::vector<ABBLEField> fields;
        std::vector<ABBLESetting> settings;
        std::vector<BLECharacteristic*> characteristicsSettings;
        std::vector<BLECharacteristic*> characteristicsFields;

};

#endif