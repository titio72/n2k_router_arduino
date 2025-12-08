#include <unity.h>
#include <Log.h>

void setUpMeteoDHT();
void tearDownMeteoDHT();
void run_meteo_dht_tests();
void setUpMeteoBME();
void tearDownMeteoBME();
void run_meteo_bme_tests();

void setUp()
{
    setUpMeteoDHT();
    setUpMeteoBME();
}

void tearDown()
{
    tearDownMeteoBME();
    tearDownMeteoDHT();
}

int main()
{
    UNITY_BEGIN();
    run_meteo_dht_tests();
    run_meteo_bme_tests();
    UNITY_END();
    return 0;
}