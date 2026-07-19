#include <unity.h>
#include <vector>
#include "N2K_router.h"
#include "Data.h"

// Captures every message that would be sent to the bus, without touching hardware.
class CapturingN2KSender : public N2KSenderAbstract
{
public:
    std::vector<tN2kMsg> sent_messages;
    unsigned char source = 42;

    virtual unsigned char get_source() override { return source; }
    virtual N2KStats getStats() override { return stats; }

protected:
    virtual bool send_it(tN2kMsg &msg) override
    {
        sent_messages.push_back(msg);
        stats.sent++;
        return true;
    }
};

void setUp(void) {}
void tearDown(void) {}

#pragma region Source Address Tests

void test_send_engine_rpm_uses_claimed_source(void)
{
    CapturingN2KSender sender;
    sender.source = 37;

    sender.sendEngineRPM(0, 1500);

    TEST_ASSERT_EQUAL(1, sender.sent_messages.size());
    TEST_ASSERT_EQUAL_UINT8(37, sender.sent_messages[0].Source);
}

void test_send_engine_hours_uses_claimed_source(void)
{
    CapturingN2KSender sender;
    sender.source = 37;

    sender.sendEngineHours(0, 3600.0);

    TEST_ASSERT_EQUAL(1, sender.sent_messages.size());
    TEST_ASSERT_EQUAL_UINT8(37, sender.sent_messages[0].Source);
}

void test_send_battery_uses_claimed_source(void)
{
    CapturingN2KSender sender;
    sender.source = 37;

    sender.sendBattery(1, 12.6, 1.2, 25.0, 0);

    TEST_ASSERT_EQUAL(1, sender.sent_messages.size());
    TEST_ASSERT_EQUAL_UINT8(37, sender.sent_messages[0].Source);
}

void test_send_battery_status_uses_claimed_source(void)
{
    CapturingN2KSender sender;
    sender.source = 37;

    sender.sendBatteryStatus(1, 95.0, 280.0, 600.0, 0);

    TEST_ASSERT_EQUAL(1, sender.sent_messages.size());
    TEST_ASSERT_EQUAL_UINT8(37, sender.sent_messages[0].Source);
}

#pragma endregion

#pragma region Satellites Bounds Tests

void test_send_satellites_does_not_overread_past_nsat(void)
{
    CapturingN2KSender sender;
    sender.source = 10;

    GPSData gps;
    gps.nSat = 3;
    for (int i = 0; i < 3; i++)
    {
        gps.satellites[i].sat_id = 100 + i;
        gps.satellites[i].elev = 10;
        gps.satellites[i].az = 20;
        gps.satellites[i].db = 30;
        gps.satellites[i].used = 1;
    }
    // One slot past the reported count, populated with a distinct "poison" value.
    // If sendSatellites reads past nSat, this gets appended as a 4th satellite.
    gps.satellites[3].sat_id = 9999;
    gps.satellites[3].elev = 0;
    gps.satellites[3].az = 0;
    gps.satellites[3].db = 30;
    gps.satellites[3].used = 0;

    bool result = sender.sendSatellites(gps, 5);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, sender.sent_messages.size());

    int idx = 2; // NumberOfSVs byte, per SetN2kPGN129540 layout
    uint8_t reported_count = sender.sent_messages[0].GetByte(idx);
    TEST_ASSERT_EQUAL_UINT8(3, reported_count);
}

#pragma endregion

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_send_engine_rpm_uses_claimed_source);
    RUN_TEST(test_send_engine_hours_uses_claimed_source);
    RUN_TEST(test_send_battery_uses_claimed_source);
    RUN_TEST(test_send_battery_status_uses_claimed_source);

    RUN_TEST(test_send_satellites_does_not_overread_past_nsat);

    return UNITY_END();
}
