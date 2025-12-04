
#include <Log.h>
#include "BTInterface.h"
#include <string>
#include <stdint.h>

#ifndef NATIVE
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLECharacteristic.h>
#include <BLEUUID.h>

class MyServerCBack : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        Log::trace("[BLE] Connected to client\n");
        Log::trace("[BLE] Readvertising\n");
        pServer->getAdvertising()->start();
    };

    void onDisconnect(BLEServer *pServer)
    {
        Log::trace("[BLE] Disconneted from client\n");
    }
};

class MyCInCBack : public BLECharacteristicCallbacks
{
public:
    MyCInCBack(ABBLEWriteCallback **_c, std::vector<ABBLESetting> &_s) : c(_c), settings(_s) {}

    virtual void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
    {
        static char v[256];
        // bounded copy of incoming value to local buffer
        strncpy(v, pCharacteristic->getValue().c_str(), sizeof(v) - 1);
        v[sizeof(v) - 1] = '\0';
        //Log::tracex("BLE", "Characteristic write", "UUID {%s} value {%s}", pCharacteristic->getUUID().toString().c_str(), v);

        int i = 0;
        for (i = 0; i < settings.size(); i++)
        {
            BLEUUID uuid(settings[i].c_uuid);
            if (uuid.equals(pCharacteristic->getUUID()))
                break;
        }
        if (i < settings.size() && *c)
        {
            (*c)->on_write(i, v);
        }
    }

private:
    ABBLEWriteCallback **c;
    std::vector<ABBLESetting> &settings;
};

class InternalBLEState
{
public:
    InternalBLEState(const char* n, const char* id, ABBLEWriteCallback **_c, std::vector<ABBLESetting> &_s)
        : pServer(nullptr), pService(nullptr), name(n), uuid(id)
    {
        listener = new MyCInCBack(_c, _s);
        serverCBack = new MyServerCBack();
    }

    ~InternalBLEState()
    {
        delete listener;
        delete serverCBack;
    }

    std::string name;
    std::string uuid;

    void deinit()
    {
        BLEDevice::deinit();
    }

    void begin()
    {
        Log::tracex("BLE", "Starting BLE", "device {%s}", name.c_str());
        pService->start();
        BLESecurity *pSecurity = new BLESecurity();
        pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
        pSecurity->setCapability(ESP_IO_CAP_NONE);
        pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
        pServer->getAdvertising()->addServiceUUID(uuid.c_str());
        pServer->getAdvertising()->start();
    }

    void set_field_value(int handle, const char *value)
    {
        if (handle >= 0 && handle < characteristicsFields.size())
        {
            BLECharacteristic *c = characteristicsFields[handle];
            c->setValue(value);
            c->indicate();
        }
    }

    void set_field_value(int handle, uint16_t value)
    {
        if (handle>=0 && handle<characteristicsFields.size())
        {
            BLECharacteristic* c = characteristicsFields[handle];
            c->setValue(value);
            c->indicate();
        }
    }

    void set_field_value(int handle, void *value, int len)
    {
        if (handle >= 0 && handle < characteristicsFields.size())
        {
            BLECharacteristic *c = characteristicsFields[handle];
            c->setValue((uint8_t *)value, len);
            c->indicate();
        }
    }


    void set_setting_value(int handle, const char *value)
    {
        if (handle >= 0 && handle < characteristicsSettings.size())
        {
            BLECharacteristic *c = characteristicsSettings[handle];
            c->setValue(value);
        }
    }

    void set_setting_value(int handle, int value)
    {
        if (handle >= 0 && handle < characteristicsSettings.size())
        {
            static char temp[16];
            itoa(value, temp, 10);
            BLECharacteristic *c = characteristicsSettings[handle];
            c->setValue(temp);
        }
    }


    void createSettingCharacteristics(BLEService *pService, const char* uuid, BLECharacteristic **c, BLECharacteristicCallbacks *cback)
    {
        Log::tracex("BLE", "Creating bool characteristic", "UUID {%s} service {%s}", uuid, pService->getUUID().toString().c_str());
        *c = pService->createCharacteristic(uuid,
                                            BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
        (*c)->setReadProperty(true);
        (*c)->setWriteProperty(true);
        (*c)->setCallbacks(cback);
    }

    void createFieldCharacteristic(BLEService *pService, const char* uuid, BLECharacteristic **c)
    {
        Log::tracex("BLE", "Creating numeric characteristic", "UUID {%s} service {%s}", uuid, pService->getUUID().toString().c_str());
        *c = pService->createCharacteristic(uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_INDICATE);
        (*c)->setIndicateProperty(true);
        (*c)->setReadProperty(true);
        (*c)->addDescriptor(new BLE2902());
    }

    void setup(std::vector<ABBLEField> &fields, std::vector<ABBLESetting> &settings)
    {
        Log::tracex("BLE", "Setup", "device {%s}", name.c_str());
        BLEDevice::init(name);
        BLEDevice::setMTU(128);
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(serverCBack);
        pService = pServer->createService(uuid.c_str());
        Log::tracex("BLE", "Loading characteristics");
        for (int i = 0; i < settings.size(); i++)
        {
            ABBLESetting &s = settings[i];
            BLECharacteristic *c;
            createSettingCharacteristics(pService, s.c_uuid.c_str(), &c, listener);
            characteristicsSettings.push_back(c);
        }
        for (int i = 0; i < fields.size(); i++)
        {
            ABBLEField &s = fields[i];
            BLECharacteristic *c;
            createFieldCharacteristic(pService, s.c_uuid.c_str(), &c);
            characteristicsFields.push_back(c);
        }
        Log::tracex("BLE", "Loaded", "Settings {%d} Fields {%d}", settings.size(), fields.size());
    }

    void change_device_name(const char *n)
    {
        name = n;
        if (pServer)
        {
            pServer->getAdvertising()->stop();
            esp_err_t errRc = ::esp_ble_gap_set_device_name(n);
            if (errRc != ESP_OK)
            {
                Log::tracex("BLE", "Change device name", "error {%d} name {%s}", errRc, name);
            }
            else
            {
                Log::tracex("BLE", "Change device name", "name {%s}", name);
            }
            pServer->getAdvertising()->start();
        }
    }
private:
    BLEServer *pServer;
    BLEService *pService;
    BLECharacteristicCallbacks* listener;
    BLEServerCallbacks* serverCBack;
    std::vector<BLECharacteristic*> characteristicsSettings;
    std::vector<BLECharacteristic*> characteristicsFields;
};
#else
class InternalBLEState
{
public:
    InternalBLEState(const char* n, const char* id, ABBLEWriteCallback **_c, std::vector<ABBLESetting> &_s)
        : name(n), uuid(id)
    {
    }

    ~InternalBLEState()
    {
    }

    std::string name;
    std::string uuid;

    void begin()
    {
        Log::tracex("BLE_ULL", "Starting BLE", "device {%s}", name.c_str());
    }

    void set_field_value(int handle, const char *value)
    {

    }

    void set_field_value(int handle, uint16_t value)
    {

    }

    void set_field_value(int handle, void *value, int len)
    {

    }


    void set_setting_value(int handle, const char *value)
    {

    }

    void set_setting_value(int handle, int value)
    {

    }

    void setup(std::vector<ABBLEField> &fields, std::vector<ABBLESetting> &settings)
    {
        Log::tracex("BLE_NULL", "Setup", "device {%s}", name.c_str());
    }

    void change_device_name(const char *n)
    {
        name = n;
    }

    void deinit()
    {}
};

#endif

BTInterface::BTInterface(const char *uuid, const char *name): init(false)
{
    state = new InternalBLEState(name, uuid, &callback, settings);
}

BTInterface::~BTInterface()
{
    state->deinit();
    delete state;
}

int BTInterface::add_setting(const char *name, const char *uuid)
{
    ABBLESetting s(name, uuid);
    settings.push_back(s);
    return settings.size() - 1;
}

int BTInterface::add_field(const char *name, const char *uuid)
{
    ABBLEField f(name, uuid);
    fields.push_back(f);
    return fields.size() - 1;
}

void BTInterface::set_field_value(int handle, const char *value)
{
    state->set_field_value(handle, value);
}

void BTInterface::set_field_value(int handle, uint16_t value)
{
    state->set_field_value(handle, value);
}

void BTInterface::set_field_value(int handle, void *value, int len)
{
    state->set_field_value(handle, value, len);
}

void BTInterface::set_setting_value(int handle, const char *value)
{
    state->set_setting_value(handle, value);
}

void BTInterface::set_setting_value(int handle, int value)
{
    state->set_setting_value(handle, value);
}

void BTInterface::setup()
{
    if (!init)
    {
        init = true;
        state->setup(fields, settings);
    }
}

void BTInterface::begin()
{
    if (init) state->begin();
}

void BTInterface::loop(unsigned long milli_seconds)
{
}

void BTInterface::set_device_name(const char *name)
{
    state->change_device_name(name);
}
