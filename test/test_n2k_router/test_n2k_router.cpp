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

#pragma region Not-available encoding

// PGN 129026 COG&SOG rapid: byte 0 SID, byte 1 reference, bytes 2-3 COG (0.0001 rad), bytes 4-5 SOG (0.01 m/s)
static bool is_na16(const tN2kMsg &m, int index)
{
    return m.Data[index] == 0xFF && m.Data[index + 1] == 0xFF;
}

void test_cogsog_nan_cog_is_not_available_not_zero_degrees(void)
{
    CapturingN2KSender sender;
    sender.sendCOGSOG(5.0, NAN, 0);
    TEST_ASSERT_EQUAL(1, sender.sent_messages.size());
    TEST_ASSERT_TRUE(is_na16(sender.sent_messages[0], 2));  // COG
    TEST_ASSERT_FALSE(is_na16(sender.sent_messages[0], 4)); // SOG is real
}

void test_cogsog_nan_sog_is_not_available(void)
{
    CapturingN2KSender sender;
    sender.sendCOGSOG(NAN, 90.0, 0);
    TEST_ASSERT_EQUAL(1, sender.sent_messages.size());
    TEST_ASSERT_TRUE(is_na16(sender.sent_messages[0], 4));  // SOG
    TEST_ASSERT_FALSE(is_na16(sender.sent_messages[0], 2)); // COG is real
}

void test_cogsog_both_valid_and_zero_cog_is_a_real_value(void)
{
    CapturingN2KSender sender;
    sender.sendCOGSOG(5.0, 0.0, 0); // heading due north is a legitimate COG
    TEST_ASSERT_FALSE(is_na16(sender.sent_messages[0], 2));
    TEST_ASSERT_FALSE(is_na16(sender.sent_messages[0], 4));
    TEST_ASSERT_EQUAL_UINT8(0, sender.sent_messages[0].Data[2]);
    TEST_ASSERT_EQUAL_UINT8(0, sender.sent_messages[0].Data[3]);
}

void test_cogsog_both_nan_sends_nothing(void)
{
    CapturingN2KSender sender;
    TEST_ASSERT_FALSE(sender.sendCOGSOG(NAN, NAN, 0));
    TEST_ASSERT_EQUAL(0, sender.sent_messages.size());
}

// PGN 128259 boat speed: byte 0 SID, bytes 1-2 water referenced, bytes 3-4 ground referenced (0.01 m/s)
void test_stw_has_no_ground_referenced_speed(void)
{
    CapturingN2KSender sender;
    sender.sendSTW(5.0);
    TEST_ASSERT_EQUAL(1, sender.sent_messages.size());
    const tN2kMsg &m = sender.sent_messages[0];
    TEST_ASSERT_FALSE(is_na16(m, 1)); // water speed is real: 5 kn = 2.57 m/s
    TEST_ASSERT_EQUAL_UINT16(257, m.Data[1] | (m.Data[2] << 8));
    TEST_ASSERT_TRUE(is_na16(m, 3));  // ground speed: not available
}

// PGN 129539 DOPs are signed 16-bit (0.01): "not available" is 0x7FFF
static bool is_na_s16(const tN2kMsg &m, int index)
{
    return m.Data[index] == 0xFF && m.Data[index + 1] == 0x7F;
}

void test_gnss_status_with_nan_dops_is_not_available(void)
{
    CapturingN2KSender sender;
    GPSData gps;
    gps.fix = 3; // hdop/vdop/tdop stay NaN
    sender.sendGNNSStatus(gps, 0);
    TEST_ASSERT_EQUAL(1, sender.sent_messages.size());
    const tN2kMsg &m = sender.sent_messages[0];
    // byte 0 SID, byte 1 modes, bytes 2-3 HDOP, 4-5 VDOP, 6-7 TDOP
    TEST_ASSERT_TRUE(is_na_s16(m, 2));
    TEST_ASSERT_TRUE(is_na_s16(m, 4));
    TEST_ASSERT_TRUE(is_na_s16(m, 6));
}

void test_gnss_status_with_real_dops_is_sent_as_values(void)
{
    CapturingN2KSender sender;
    GPSData gps;
    gps.fix = 3;
    gps.hdop = 1.25;
    gps.vdop = 2.0;
    gps.tdop = NAN;
    sender.sendGNNSStatus(gps, 0);
    const tN2kMsg &m = sender.sent_messages[0];
    TEST_ASSERT_EQUAL_INT16(125, (int16_t)(m.Data[2] | (m.Data[3] << 8)));
    TEST_ASSERT_EQUAL_INT16(200, (int16_t)(m.Data[4] | (m.Data[5] << 8)));
    TEST_ASSERT_TRUE(is_na_s16(m, 6));
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

    RUN_TEST(test_cogsog_nan_cog_is_not_available_not_zero_degrees);
    RUN_TEST(test_cogsog_nan_sog_is_not_available);
    RUN_TEST(test_cogsog_both_valid_and_zero_cog_is_a_real_value);
    RUN_TEST(test_cogsog_both_nan_sends_nothing);
    RUN_TEST(test_stw_has_no_ground_referenced_speed);
    RUN_TEST(test_gnss_status_with_nan_dops_is_not_available);
    RUN_TEST(test_gnss_status_with_real_dops_is_sent_as_values);

    return UNITY_END();
}
