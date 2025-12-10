#include <unity.h>
#include <Log.h>

void setUpMeteoDHT();
void tearDownMeteoDHT();
void run_meteo_dht_tests();
void setUpMeteoBME();
void tearDownMeteoBME();
void run_meteo_bme_tests();
void setUpEnvMessenger();
void tearDownEnvMessenger();
void runEnvMessengerTests();

void setUp()
{
    setUpMeteoDHT();
    setUpMeteoBME();
    setUpEnvMessenger();
}

void tearDown()
{
    tearDownMeteoBME();
    tearDownMeteoDHT();
    tearDownEnvMessenger();
}

int main()
{
    UNITY_BEGIN();
    run_meteo_dht_tests();
    run_meteo_bme_tests();
    runEnvMessengerTests();
    UNITY_END();
    return 0;
}