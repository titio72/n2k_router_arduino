#ifndef N2K_ROUTER_H_
#define N2K_ROUTER_H_

#include <Utils.h>
#include <N2K.h>
#include "Agents.hpp"
#include "Data.h"

#ifndef NATIVE
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#endif

#ifndef N2K_SOURCE
#define N2K_SOURCE 22
#endif

class N2KSender
{
public:
    virtual bool sendCOGSOG(double sog, double cog, unsigned char sid) = 0;
    virtual bool sendSTW(double stw) = 0;
    virtual bool sendSystemTime(uint32_t t, unsigned char sid, uint16_t t_ms = 0) = 0;
    virtual bool sendPosition(double latitude, double longitude) = 0;
    virtual bool sendCabinTemp(const double temperature, unsigned char sid = 0xFF) = 0;
    virtual bool sendEnvironmentXRaymarine(const double pressure, const double humidity, const double temperature) = 0;
    virtual bool sendHumidity(const double humidity, unsigned char sid = 0xFF) = 0;
    virtual bool sendPressure(const double pressure, unsigned char sid = 0xFF) = 0;
    virtual bool sendElectronicTemperature(const double temp, unsigned char sid = 0xFF) = 0;
    virtual bool sendSeaTemperature(const double temp, unsigned char sid = 0xFF) = 0;
    virtual bool sendGNSSPosition(const GPSData &gps, unsigned char sid) = 0;
    virtual bool sendMagneticVariation(double variation, uint16_t days_since_1970) = 0;
    virtual bool sendGNNSStatus(const GPSData &data, unsigned char sid) = 0;
    virtual bool sendSatellites(const GPSData &data, unsigned char sid) = 0;
    virtual bool sendBattery(unsigned char sid, const double voltage, const double current, const double temperature, const unsigned char instance) = 0;
    virtual bool sendBatteryStatus(unsigned char sid, const double soc, const double capacity, const double ttg, const unsigned char instance) = 0;
    virtual bool sendEngineRPM(uint8_t engine_n, uint16_t rpm) = 0;
    virtual bool sendEngineHours(uint8_t engine_n, double hours) = 0;
    virtual bool sendMagneticHeading(double heading) = 0; //for tests only

    virtual N2KStats getStats() = 0;

    virtual unsigned char get_source() = 0;
};

class N2KSenderAbstract : public N2KSender
{
public:
    N2KSenderAbstract() {}

    virtual bool sendCOGSOG(double sog, double cog, unsigned char sid) override;
    virtual bool sendSTW(double sog) override;
    virtual bool sendSystemTime(uint32_t t, unsigned char sid, uint16_t t_ms = 0) override;
    virtual bool sendPosition(double latitude, double longitude) override;
    virtual bool sendCabinTemp(const double temperature, unsigned char sid = 0xFF) override;
    virtual bool sendEnvironmentXRaymarine(const double pressure, const double humidity, const double temperature) override;
    virtual bool sendHumidity(const double humidity, unsigned char sid = 0xFF) override;
    virtual bool sendPressure(const double pressure, unsigned char sid = 0xFF) override;
    virtual bool sendElectronicTemperature(const double temp, unsigned char sid = 0xFF) override;
    virtual bool sendSeaTemperature(const double temp, unsigned char sid = 0xFF) override;
    virtual bool sendGNSSPosition(const GPSData &gps, unsigned char sid) override;
    virtual bool sendGNNSStatus(const GPSData &data, unsigned char sid) override;
    virtual bool sendMagneticVariation(double variation, uint16_t days_since_1970) override;
    virtual bool sendSatellites(const GPSData &data, unsigned char sid) override;
    virtual bool sendBattery(unsigned char sid, const double voltage, const double current, const double temperature, const unsigned char instance) override;
    virtual bool sendBatteryStatus(unsigned char sid, const double soc, const double capacity, const double ttg, const unsigned char instance) override;
    virtual bool sendEngineRPM(uint8_t engine_n, uint16_t rpm) override;
    virtual bool sendEngineHours(uint8_t engine_n, double hours) override;
    virtual bool sendMagneticHeading(double heading) override; //for tests only

protected:
    N2KStats stats;
    virtual bool send_it(tN2kMsg &N2kMsg) = 0;
};

class N2K_router : public N2KSenderAbstract
{
public:
    N2K_router(n2k_source_change_handler source_handler = nullptr);

    void set_desired_source(unsigned char src);

    virtual unsigned char get_source() override;

    AB_AGENT

    N2KStats getStats() override;

protected:
    virtual bool send_it(tN2kMsg &N2kMsg);

private:
    N2K &n2k;
#ifndef NATIVE
    QueueHandle_t _send_queue = nullptr;
    TaskHandle_t _n2k_task = nullptr;
    static void n2k_task_fn(void *param);
#endif
};

#ifdef PIO_UNIT_TESTING
class NullN2KSender : public N2KSender
{
public:
    NullN2KSender()
    {
        stats.canbus = 1;
    }

    virtual bool sendCOGSOG(double sog, double cog, unsigned char sid) override
    {
        stats.sent++;
        return true;
    }
    virtual bool sendSTW(double sog) override
    {
        stats.sent++;
        return true;
    }
    virtual bool sendSystemTime(uint32_t t, unsigned char sid, uint16_t t_ms = 0) override
    {
        stats.sent++;
        return true;
    }
    virtual bool sendPosition(double latitude, double longitude) override
    {
        stats.sent++;
        return true;
    }
    virtual bool sendCabinTemp(const double temperature, unsigned char sid = 0xFF) override
    {
        stats.sent++;
        return true;
    }
    virtual bool sendEnvironmentXRaymarine(const double pressure, const double humidity, const double temperature) override
    {
        stats.sent++;
        return true;
    }
    virtual bool sendHumidity(const double humidity, unsigned char sid = 0xFF) override
    {
        stats.sent++;
        return true;
    }
    virtual bool sendPressure(const double pressure, unsigned char sid = 0xFF) override
    {
        stats.sent++;
        return true;
    }
    virtual bool sendElectronicTemperature(const double temp, unsigned char sid = 0xFF) override
    {
        stats.sent++;
        return true;
    }
    virtual bool sendGNSSPosition(const GPSData &gps, unsigned char sid) override
    {
        stats.sent++;
        return true;
    }
    virtual bool sendGNNSStatus(const GPSData &data, unsigned char sid) override
    {
        stats.sent++;
        return true;
    }
    virtual bool sendSatellites(const GPSData &data, unsigned char sid) override
    {
        stats.sent++;
        return true;
    }
    virtual bool sendBattery(unsigned char sid, const double voltage, const double current, const double temperature, const unsigned char instance) override
    {
        stats.sent++;
        battery++;
        return true;
    }
    virtual bool sendBatteryStatus(unsigned char sid, const double soc, const double capacity, const double ttg, const unsigned char instance) override
    {
        stats.sent++;
        batteryStatus++;
        return true;
    }
    virtual bool sendEngineRPM(uint8_t engine_n, uint16_t rpm) override
    {
        stats.sent++;
        return true;
    }
    virtual bool sendEngineHours(uint8_t engine_n, double hours) override
    {
        stats.sent++;
        return true;
    }

    virtual N2KStats getStats() { return stats; }

    virtual unsigned char get_source() { return N2K_SOURCE; }

    unsigned int battery = 0;
    unsigned int batteryStatus = 0;

private:
    N2KStats stats;
};
#endif

#endif
