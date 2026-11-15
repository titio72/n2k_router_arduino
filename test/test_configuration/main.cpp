#include <unity.h>
#include <Log.h>

void setUpN2k_services();
void setUpConfiguration();
void tearDownConfiguration();
void tearDownN2k_services();
void run_N2k_services_tests();
void run_configuration_tests();
void run_stw_table_conf_tests();

void setUp()
{
    setUpN2k_services();
    setUpConfiguration();
}

void tearDown()
{
    tearDownConfiguration();
    tearDownN2k_services();
}

int main()
{
    Log::disable();
    UNITY_BEGIN();
    run_N2k_services_tests();
    run_configuration_tests();
    run_stw_table_conf_tests();
    UNITY_END();
}