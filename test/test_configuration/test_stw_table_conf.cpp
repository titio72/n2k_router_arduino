#include <unity.h>
#include <string.h>
#include <vector>
#include "Conf.h"
#include "StwCalibration.h"

namespace
{
// Recording persistence fake for the STW calibration table: remembers the last blob saved
class FakeStwPersistence : public StwCalibrationPersistence
{
public:
    std::vector<uint8_t> stored;
    bool init_ok = true;
    bool save_ok = true;
    int saves = 0;
    virtual bool init_persistence() override { return init_ok; }
    virtual bool save_stw_calibration(const uint8_t *blob, size_t len) override
    {
        if (!save_ok)
            return false;
        stored.assign(blob, blob + len);
        saves++;
        return true;
    }
    virtual size_t load_stw_calibration(uint8_t *blob, size_t cap) override
    {
        if (stored.empty() || stored.size() > cap)
            return 0;
        memcpy(blob, stored.data(), stored.size());
        return stored.size();
    }
};

// Always succeeds with a fresh Conf, to stay isolated from the EEPROM version-mismatch behaviour
class OkConf : public ConfigurationPersistence
{
public:
    virtual bool init_persistence() override { return true; }
    virtual bool save_configuration(const Conf &c) override { return true; }
    virtual bool load_configuration(Conf &c) override { c = Conf(); return true; }
};

void enable_table(Configuration &conf)
{
    N2KServices s = conf.get_services();
    s.set_use_stw_table(true);
    conf.save_services(s);
}
}

void test_get_stw_speed_table_off_uses_adjustment(void)
{
    MockConfiguration conf;
    conf.save_stw_paddle_adjustment(1.1);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1.1, conf.get_stw_speed(4.8));
}

void test_get_stw_speed_table_on_ignores_adjustment(void)
{
    MockConfiguration conf;
    conf.save_stw_paddle_adjustment(1.1);
    TEST_ASSERT_TRUE(conf.save_stw_table_point(1.0, 1.0));
    TEST_ASSERT_TRUE(conf.save_stw_table_point(2.0, 2.2));
    enable_table(conf);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1.0, conf.get_stw_speed(4.8));
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1.6, conf.get_stw_speed(7.2));
}

void test_get_stw_speed_table_on_but_empty(void)
{
    MockConfiguration conf;
    conf.save_stw_paddle_adjustment(1.5);
    enable_table(conf);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 2.0, conf.get_stw_speed(9.6));
}

void test_table_persisted_and_loaded(void)
{
    OkConf ok;
    FakeStwPersistence fake;
    {
        Configuration conf(&ok, nullptr, &fake);
        TEST_ASSERT_EQUAL_INT(CONFIG_RES_OK, conf.init());
        TEST_ASSERT_TRUE(conf.save_stw_table_point(1.0, 1.0));
        TEST_ASSERT_TRUE(conf.save_stw_table_point(2.0, 2.2));
        TEST_ASSERT_EQUAL_INT(2, fake.saves);
    }
    Configuration conf2(&ok, nullptr, &fake);
    TEST_ASSERT_EQUAL_INT(CONFIG_RES_OK, conf2.init());
    const StwCalibration &t = conf2.get_stw_table();
    TEST_ASSERT_EQUAL_UINT8(2, t.size());
    TEST_ASSERT_EQUAL_UINT16(100, t.point(0).raw100);
    TEST_ASSERT_EQUAL_UINT16(100, t.point(0).kn100);
    TEST_ASSERT_EQUAL_UINT16(200, t.point(1).raw100);
    TEST_ASSERT_EQUAL_UINT16(220, t.point(1).kn100);
}

void test_table_delete_and_clear_persisted(void)
{
    OkConf ok;
    FakeStwPersistence fake;
    Configuration conf(&ok, nullptr, &fake);
    conf.init();
    conf.save_stw_table_point(1.0, 1.0);
    conf.save_stw_table_point(2.0, 2.2);
    TEST_ASSERT_TRUE(conf.delete_stw_table_point(1.0));
    TEST_ASSERT_EQUAL_UINT8(1, conf.get_stw_table().size());
    {
        Configuration conf2(&ok, nullptr, &fake);
        conf2.init();
        TEST_ASSERT_EQUAL_UINT8(1, conf2.get_stw_table().size());
        TEST_ASSERT_EQUAL_UINT16(200, conf2.get_stw_table().point(0).raw100);
    }
    TEST_ASSERT_TRUE(conf.clear_stw_table());
    TEST_ASSERT_EQUAL_UINT8(0, conf.get_stw_table().size());
    Configuration conf3(&ok, nullptr, &fake);
    conf3.init();
    TEST_ASSERT_EQUAL_UINT8(0, conf3.get_stw_table().size());
}

void test_table_write_failure_leaves_table(void)
{
    OkConf ok;
    FakeStwPersistence fake;
    Configuration conf(&ok, nullptr, &fake);
    conf.init();
    TEST_ASSERT_TRUE(conf.save_stw_table_point(1.0, 1.0));
    TEST_ASSERT_TRUE(conf.save_stw_table_point(2.0, 2.2));

    fake.save_ok = false;
    TEST_ASSERT_FALSE(conf.save_stw_table_point(3.0, 3.0));
    TEST_ASSERT_EQUAL_UINT8(2, conf.get_stw_table().size());
    TEST_ASSERT_FALSE(conf.save_stw_table_point(1.0, 1.1)); // replace existing
    TEST_ASSERT_EQUAL_UINT16(100, conf.get_stw_table().point(0).kn100);
    TEST_ASSERT_FALSE(conf.delete_stw_table_point(1.0));
    TEST_ASSERT_EQUAL_UINT8(2, conf.get_stw_table().size());
    TEST_ASSERT_FALSE(conf.clear_stw_table());
    TEST_ASSERT_EQUAL_UINT8(2, conf.get_stw_table().size());
}

void test_invalid_point_not_persisted(void)
{
    OkConf ok;
    FakeStwPersistence fake;
    Configuration conf(&ok, nullptr, &fake);
    conf.init();
    TEST_ASSERT_TRUE(conf.save_stw_table_point(1.0, 1.0));
    int saves = fake.saves;
    TEST_ASSERT_FALSE(conf.save_stw_table_point(-1.0, 1.0));
    TEST_ASSERT_FALSE(conf.save_stw_table_point(2.0, 0.5)); // not monotonic
    TEST_ASSERT_FALSE(conf.save_stw_table_point(50.01, 1.0));
    TEST_ASSERT_EQUAL_INT(saves, fake.saves);
    TEST_ASSERT_EQUAL_UINT8(1, conf.get_stw_table().size());
}

void test_delete_missing_point_not_persisted(void)
{
    OkConf ok;
    FakeStwPersistence fake;
    Configuration conf(&ok, nullptr, &fake);
    conf.init();
    conf.save_stw_table_point(1.0, 1.0);
    int saves = fake.saves;
    TEST_ASSERT_FALSE(conf.delete_stw_table_point(1.6));
    TEST_ASSERT_EQUAL_INT(saves, fake.saves);
}

void test_corrupt_blob_loads_empty(void)
{
    OkConf ok;
    FakeStwPersistence fake;
    fake.stored = {0xFF, 0x09, 0x01, 0x02, 0x03};
    Configuration conf(&ok, nullptr, &fake);
    TEST_ASSERT_EQUAL_INT(CONFIG_RES_OK, conf.init());
    TEST_ASSERT_EQUAL_UINT8(0, conf.get_stw_table().size());
}

void test_table_persistence_init_failure_is_non_fatal(void)
{
    OkConf ok;
    FakeStwPersistence fake;
    fake.init_ok = false;
    Configuration conf(&ok, nullptr, &fake);
    TEST_ASSERT_EQUAL_INT(CONFIG_RES_OK, conf.init());
    TEST_ASSERT_EQUAL_UINT8(0, conf.get_stw_table().size());
}

void test_mock_configuration_counts_table_calls(void)
{
    MockConfiguration conf;
    conf.save_stw_table_point(1.0, 1.0);
    conf.save_stw_table_point(2.0, 2.2);
    conf.delete_stw_table_point(1.0);
    conf.clear_stw_table();
    TEST_ASSERT_EQUAL_INT(2, conf.save_stw_table_point_calls);
    TEST_ASSERT_EQUAL_INT(1, conf.delete_stw_table_point_calls);
    TEST_ASSERT_EQUAL_INT(1, conf.clear_stw_table_calls);
    conf.reset_call_counts();
    TEST_ASSERT_EQUAL_INT(0, conf.save_stw_table_point_calls);
    TEST_ASSERT_EQUAL_INT(0, conf.delete_stw_table_point_calls);
    TEST_ASSERT_EQUAL_INT(0, conf.clear_stw_table_calls);
}

void run_stw_table_conf_tests(void)
{
    RUN_TEST(test_get_stw_speed_table_off_uses_adjustment);
    RUN_TEST(test_get_stw_speed_table_on_ignores_adjustment);
    RUN_TEST(test_get_stw_speed_table_on_but_empty);
    RUN_TEST(test_table_persisted_and_loaded);
    RUN_TEST(test_table_delete_and_clear_persisted);
    RUN_TEST(test_table_write_failure_leaves_table);
    RUN_TEST(test_invalid_point_not_persisted);
    RUN_TEST(test_delete_missing_point_not_persisted);
    RUN_TEST(test_corrupt_blob_loads_empty);
    RUN_TEST(test_table_persistence_init_failure_is_non_fatal);
    RUN_TEST(test_mock_configuration_counts_table_calls);
}
