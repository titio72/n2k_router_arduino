#include <unity.h>
#include <string.h>
#include "BMV712.h"
#include "MockPort.hpp"
#include "Conf.h"
#include "Context.h"
#include "N2K_router.h"
#include "VeDirect.h"
#include <Log.h>

void setUp(void)
{
    Log::enable();
}

void tearDown(void)
{
}

#pragma region BMV712 Construction Tests

void test_bmv712_construction(void)
{
    MOCK_CONTEXT
    
    MockPort mock_port("BMV_PORT");
    
    BMV712 bmv(mock_port);
    
    // Should construct without error
    TEST_ASSERT_TRUE(true);
}

void test_bmv712_initial_state(void)
{
    MOCK_CONTEXT
    
    MockPort mock_port("BMV_PORT");
    
    BMV712 bmv(mock_port);
    
    // Initial state should be invalid until first complete message
    const VEDirectObject &obj = bmv.getLastValidValue();
    TEST_ASSERT_FALSE(obj.is_valid());
}

#pragma endregion

#pragma region BMV712 Listener Tests

void test_bmv712_read_valid_message(void)
{
    MOCK_CONTEXT   
    MockPort mock_port("BMV_PORT");
    BMV712 bmv(mock_port);
    bmv.setup(context);
    bmv.enable();
    
    unsigned long t = 1250000000; // arbitrary time
    
    // Simulate complete message
    const char* lines =
        "\r\nPID\t0xA381\
        \r\nV\t13406\
        \r\nVS\t13152\
        \r\nI\t0\
        \r\nP\t0\
        \r\nCE\t-89423\
        \r\nSOC\t689\
        \r\nTTG\t-1\
        \r\nAlarm\tOFF\
        \r\nRelay\tOFF\
        \r\nAR\t0\
        \r\nBMV\t712 Smart\
        \r\nFW\t0413\
        \r\nMON\t0\
        \r\nChecksum\ty";

    mock_port.simulate_data(lines);
    bmv.loop(t + 500000/* arbitrary time .5s*/, context);
    
    double v = NAN;
    TEST_ASSERT_TRUE(bmv.getLastValidValue().is_valid());
    TEST_ASSERT_TRUE(bmv.getLastValidValue().get_number_value(v, 0.001, BMV_VOLTAGE));
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 13.406, v);
}

void test_bmv712_read_valid_message_split_in_two(void)
{
    MOCK_CONTEXT   
    MockPort mock_port("BMV_PORT");
    BMV712 bmv(mock_port);
    bmv.setup(context);
    bmv.enable();
    
    unsigned long t = 1250000000; // arbitrary time
    
    // Simulate complete message
    const char* lines =
        "\r\nPID\t0xA381\
        \r\nV\t13406\
        \r\nVS\t13152\
        \r\nI\t0\
        \r\nP\t0\
        \r\nCE\t-89423\
        \r\nSOC\t689\
        \r\nTTG\t-1\r\n";

    mock_port.simulate_data(lines);
    bmv.loop(t + 500000/* arbitrary time .5s*/, context);

    const char* lines2 = 
        "Alarm\tOFF\
        \r\nRelay\tOFF\
        \r\nAR\t0\
        \r\nBMV\t712 Smart\
        \r\nFW\t0413\
        \r\nMON\t0\
        \r\nChecksum\ty";

    mock_port.simulate_data(lines2);
    bmv.loop(t + 1000000/* arbitrary time 1s*/, context);
    
    double v = NAN;
    TEST_ASSERT_TRUE(bmv.getLastValidValue().is_valid());
    TEST_ASSERT_TRUE(bmv.getLastValidValue().get_number_value(v, 0.001, BMV_VOLTAGE));
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 13.406, v);
}
#pragma endregion

void run_bmv712_tests(void)
{
    // Construction tests
    RUN_TEST(test_bmv712_construction);
    RUN_TEST(test_bmv712_initial_state);

    // Listener tests
    RUN_TEST(test_bmv712_read_valid_message);
    RUN_TEST(test_bmv712_read_valid_message_split_in_two);
}

void setup()
{
    UNITY_BEGIN();
    run_bmv712_tests();
    UNITY_END();
}

void loop() {}

int main()
{
    setup();
    return 0;
}
/*
Data d;
NullN2KSender n2kSender;
MockConfiguration mockConf;
Context ccc = {n2kSender, mockConf, d};
MockPort mock_port("BMV_PORT");
BMV712 bmv(mock_port);
    
void setup()
{
    Serial.begin(115200);
    Log::enable();
    bmv.setup(ccc);
    bmv.enable();
}
int i = 0;
void loop() {
    unsigned long t = millis();
    static unsigned long t0 = t;
    
    if ((t-t0>500))
    {
        i = (i+1) % 2;
        bmv.loop(t, ccc);

        if (i==0)
        {
            const char* lines[] = {
                "",
                "PID\t0xA381",
                "V\t13406",
                "VS\t13152",
                "I\t0",
                "P\t0",
                "CE\t-89423",
                "SOC\t689",
                "TTG\t-1",
                "Alarm\tOFF",
                "Relay\tOFF",
                "AR\t0",
                "BMV\t712 Smart",
                "FW\t0413",
                "MON\t0"
            };
            mock_port.simulate_lines(lines, 15);
            mock_port.simulate_data("Checksum\ty");
        }

        t0 = t;
        if (bmv.getLastValidValue().is_valid())
        {
            double v = NAN;
            bool res = bmv.getLastValidValue().get_number_value(v, 0.001, BMV_VOLTAGE);
            Serial.printf("%ul %.3f\n", t);
        }
        else
        {
            Serial.printf("%ul Invalid\n", t);
        } 
    }


}
    */