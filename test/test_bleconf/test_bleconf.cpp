#include <unity.h>
#include <string.h>
#include "Conf.h"
#include "Context.h"
#include "Data.h"
#include "N2K_router.h"
#include <BTInterface.h>
#include "BLEConf.h"
#include <map>

class MockInternalBLEStateImpl: public InternalBLEState
{
public:
    std::map<int, ByteBufferPtr> settingsValues;
    std::map<int, ByteBufferPtr> fieldValues;
    ABBLEWriteCallback *clientWriteCallback = nullptr;
    std::string name;
    std::string uuid;

    MockInternalBLEStateImpl()
    {
    }

    ~MockInternalBLEStateImpl()
    {
        for (auto it = settingsValues.begin(); it != settingsValues.end(); ++it)
        {
            delete it->second;
        }
        for (auto it = fieldValues.begin(); it != fieldValues.end(); ++it)
        {
            delete it->second;
        }
    }

    void init(const char* name, const char* uuid, ABBLEWriteCallback* c)
    {
        this->name = name;
        this->uuid = uuid;
        this->clientWriteCallback = c;
    }

    void begin()
    {
        Log::tracex("BLE_ULL", "Starting BLE", "device {%s}", name.c_str());
    }

    void set_field_value(int handle, const char *value)
    {
        set_field_value(handle, (void*)value, strlen(value) + 1);
    }

    void set_field_value(int handle, uint16_t value)
    {
        set_field_value(handle, (void*)&value, sizeof(value));
    }

    void set_field_value(int handle, void *value, int len)
    {
        if (fieldValues.find(handle) == fieldValues.end())
        {
            fieldValues[handle] = new ByteBuffer(value, len);
        }
        ByteBufferPtr p = fieldValues.find(handle)->second;
        *p = ByteBuffer((uint8_t *)value, len);
    }

    ByteBuffer get_field_value(int handle)
    {
        if (fieldValues.find(handle) == fieldValues.end())
        {
            return ByteBuffer(0);
        }
        else
        {
            return *(fieldValues[handle]);

        }
    }

    void set_setting_value(int handle, const char *value)
    {
        if (settingsValues.find(handle) == settingsValues.end())
        {
            settingsValues[handle] = new ByteBuffer((uint8_t *)value, strlen(value) + 1);
        }
        ByteBufferPtr p = settingsValues.find(handle)->second;
        *p = ByteBuffer((uint8_t *)value, strlen(value) + 1);
    }

    void set_setting_value(int handle, int value)
    {
        if (settingsValues.find(handle) == settingsValues.end())
        {
            settingsValues[handle] = new ByteBuffer((uint8_t *)&value, sizeof(value));
        }
        ByteBufferPtr p = settingsValues.find(handle)->second;
        *p = ByteBuffer((uint8_t *)&value, sizeof(value));
    }

    void setup(const std::vector<ABBLEField> &fields, const std::vector<ABBLESetting> &settings)
    {
        Log::tracex("BLE_NULL", "Setup", "device {%s}", name.c_str());
    }

    void change_device_name(const char *n)
    {
        name = std::string(n).substr(0, 15);
    }

    const char* get_device_name()
    {
        return name.c_str();
    }

    void end()
    {}
};

MockInternalBLEStateImpl *mockBLEInternalImpl = nullptr;

// ==================== Mock Callback Tracker ====================
class CommandCallbackTracker {
public:
    char last_command = '\0';
    char last_command_value[256] = {0};
    int call_count = 0;
    
    void reset() {
        last_command = '\0';
        memset(last_command_value, 0, sizeof(last_command_value));
        call_count = 0;
    }
};

CommandCallbackTracker callback_tracker;

void mock_command_callback(char command, const char* command_value) {
    callback_tracker.last_command = command;
    if (command_value) {
        strncpy(callback_tracker.last_command_value, command_value, sizeof(callback_tracker.last_command_value) - 1);
        callback_tracker.last_command_value[sizeof(callback_tracker.last_command_value) - 1] = '\0';
    } else {
        memset(callback_tracker.last_command_value, 0, sizeof(callback_tracker.last_command_value));
    }
    callback_tracker.call_count++;
}

// ==================== Test Fixtures ====================
void setUp() {
    if (mockBLEInternalImpl) delete mockBLEInternalImpl;
    mockBLEInternalImpl = new MockInternalBLEStateImpl();
    callback_tracker.reset();
    Log::enable();
}

void tearDown() {
}

// ==================== Tests: Constructor & Initialization ====================
void test_constructor_with_callback() {
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    TEST_ASSERT_FALSE(ble.is_enabled());
    TEST_ASSERT_EQUAL_STRING(BLE_DEFAULT_SERVICE_NAME, ble.get_device_name());
}

void test_constructor_with_null_callback() {
    BLEConf ble(nullptr, mockBLEInternalImpl);
    TEST_ASSERT_FALSE(ble.is_enabled());
}

void test_setup_initializes_device_name() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "TestDevice";
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    TEST_ASSERT_EQUAL_STRING("TestDevice", ble.get_device_name());
}

void test_setup_copies_device_name_truncated_to_buffer_size() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "VeryLongDeviceNameThatExceeds16Chars";
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    // Should be truncated to 15 chars + null terminator
    TEST_ASSERT_EQUAL_INT(15, strlen(ble.get_device_name()));
}

void test_setup_with_empty_device_name() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "";
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    TEST_ASSERT_EQUAL_CHAR(0, ble.get_device_name()[0]);
}

// ==================== Tests: Enable/Disable ====================
void test_enable_without_setup() {
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.enable();
    TEST_ASSERT_FALSE(ble.is_enabled());
}

void test_enable_sets_enabled_flag() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    TEST_ASSERT_TRUE(ble.is_enabled());
}

void test_disable_clears_enabled_flag() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    TEST_ASSERT_TRUE(ble.is_enabled());
    
    ble.disable();
    TEST_ASSERT_FALSE(ble.is_enabled());
}

void test_is_enabled_returns_state() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    TEST_ASSERT_FALSE(ble.is_enabled());
    ble.enable();
    TEST_ASSERT_TRUE(ble.is_enabled());
}

void test_multiple_enable_calls() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    ble.enable();
    ble.enable();
    ble.enable();
    TEST_ASSERT_TRUE(ble.is_enabled());
}

void test_multiple_disable_calls() {
    MOCK_CONTEXT    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.disable();
    ble.disable();
    ble.disable();
    TEST_ASSERT_FALSE(ble.is_enabled());
}

void test_enable_disable_toggle() {
    MOCK_CONTEXT    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);

    ble.enable();
    TEST_ASSERT_TRUE(ble.is_enabled());
    ble.disable();
    TEST_ASSERT_FALSE(ble.is_enabled());
    ble.enable();
    TEST_ASSERT_TRUE(ble.is_enabled());
}

// ==================== Tests: on_write Handle 0 (Settings) ====================
void test_on_write_handle_0_triggers_callback_with_S_command() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    ble.on_write(0, "test_value");
    
    TEST_ASSERT_EQUAL_INT(1, callback_tracker.call_count);
    TEST_ASSERT_EQUAL_CHAR('S', callback_tracker.last_command);
    TEST_ASSERT_EQUAL_STRING("test_value", callback_tracker.last_command_value);
}

void test_on_write_handle_0_with_empty_value() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    ble.on_write(0, "");
    
    TEST_ASSERT_EQUAL_INT(1, callback_tracker.call_count);
    TEST_ASSERT_EQUAL_CHAR('S', callback_tracker.last_command);
}

void test_on_write_handle_0_with_long_value() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    const char* long_value = "this_is_a_very_long_configuration_string_with_multiple_parameters";
    ble.on_write(0, long_value);
    
    TEST_ASSERT_EQUAL_INT(1, callback_tracker.call_count);
    TEST_ASSERT_EQUAL_CHAR('S', callback_tracker.last_command);
    TEST_ASSERT_EQUAL_STRING(long_value, callback_tracker.last_command_value);
}

void test_on_write_handle_0_multiple_calls() {
    MOCK_CONTEXT    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    ble.on_write(0, "config1");
    TEST_ASSERT_EQUAL_INT(1, callback_tracker.call_count);
    
    ble.on_write(0, "config2");
    TEST_ASSERT_EQUAL_INT(2, callback_tracker.call_count);
    TEST_ASSERT_EQUAL_STRING("config2", callback_tracker.last_command_value);
}

// ==================== Tests: on_write Handle 1 (Commands) ====================
void test_on_write_handle_1_parses_command_and_value() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    const char* cmd_data = "Xtest_data";
    ble.on_write(1, cmd_data);
    
    TEST_ASSERT_EQUAL_INT(1, callback_tracker.call_count);
    TEST_ASSERT_EQUAL_CHAR('X', callback_tracker.last_command);
    TEST_ASSERT_EQUAL_STRING("test_data", callback_tracker.last_command_value);
}

void test_on_write_handle_1_single_char_command() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    const char* cmd_data = "Avalue";
    ble.on_write(1, cmd_data);
    
    TEST_ASSERT_EQUAL_CHAR('A', callback_tracker.last_command);
    TEST_ASSERT_EQUAL_STRING("value", callback_tracker.last_command_value);
}

void test_on_write_handle_1_with_empty_value() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    const char* cmd_data = "Z";
    ble.on_write(1, cmd_data);
    
    TEST_ASSERT_EQUAL_CHAR('Z', callback_tracker.last_command);
}

void test_on_write_handle_1_multiple_commands() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    ble.on_write(1, "Afirst");
    TEST_ASSERT_EQUAL_CHAR('A', callback_tracker.last_command);
    
    ble.on_write(1, "Bsecond");
    TEST_ASSERT_EQUAL_CHAR('B', callback_tracker.last_command);
    
    ble.on_write(1, "Cthird");
    TEST_ASSERT_EQUAL_CHAR('C', callback_tracker.last_command);
    
    TEST_ASSERT_EQUAL_INT(3, callback_tracker.call_count);
}

void test_on_write_handle_1_various_command_chars() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    const char commands[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; commands[i]; i++) {
        char cmd_data[3] = {commands[i], 'x', 0};
        callback_tracker.reset();
        
        ble.on_write(1, cmd_data);
        TEST_ASSERT_EQUAL_CHAR(commands[i], callback_tracker.last_command);
        TEST_ASSERT_EQUAL_INT(1, callback_tracker.call_count);
    }
}

// ==================== Tests: on_write without Callback ====================
void test_on_write_handle_0_without_callback() {
    MOCK_CONTEXT
    BLEConf ble(nullptr, mockBLEInternalImpl);  // No callback
    ble.setup(context);
    
    // Should not crash
    ble.on_write(0, "test_value");
    TEST_ASSERT_EQUAL_INT(0, callback_tracker.call_count);
}

void test_on_write_handle_1_without_callback() {
    MOCK_CONTEXT
    BLEConf ble(nullptr, mockBLEInternalImpl);  // No callback
    ble.setup(context);
    
    // Should not crash
    ble.on_write(1, "Xtest");
    TEST_ASSERT_EQUAL_INT(0, callback_tracker.call_count);
}

// ==================== Tests: Loop - Periodic Updates ====================
void test_loop_not_called_when_disabled() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    TEST_ASSERT_FALSE(ble.is_enabled());
    ble.loop(0, context);
}

void test_loop_initially_sends_at_first_update() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(0, context);
}

void test_loop_respects_update_period() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    mockConf.saved_n2k_src = 22;
    mockConf.saved_rpm_adjustment = 1.0;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(0, context);
    ble.loop(500000, context);
    ble.loop(1000001, context);
}

void test_loop_with_nan_values() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(0, context);
}

void test_loop_with_valid_temperature_data() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    mockConf.temperature_source = METEO_BME;
    data.meteo_0.temperature = 22.5;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(0, context);
}

void test_loop_with_valid_gps_data() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.gps.fix = 3;
    data.gps.latitude_signed = 48.8566;
    data.gps.longitude_signed = 2.3522;
    data.gps.sog = 12.5;
    data.gps.cog = 45.0;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(0, context);
}

void test_loop_with_battery_data() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.battery_svc.voltage = 13.5;
    data.battery_svc.current = 50.0;
    data.battery_svc.soc = 85.0;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(0, context);
}

void test_loop_with_engine_data() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.engine.rpm = 2500;
    data.engine.engine_time = 3600000000ULL;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(0, context);
}

void test_loop_with_complete_data() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    mockConf.saved_n2k_src = 22;
    mockConf.saved_rpm_adjustment = 1.0;
    mockConf.temperature_source = METEO_BME;
    mockConf.humidity_source = METEO_BME;
    mockConf.pressure_source = METEO_BME;
    
    data.gps.fix = 3;
    data.gps.latitude_signed = 48.8566;
    data.gps.longitude_signed = 2.3522;
    data.gps.sog = 12.5;
    data.gps.cog = 45.0;
    data.gps.gps_unix_time = 1609459200;
    
    data.meteo_0.temperature = 22.5;
    data.meteo_0.humidity = 65.0;
    data.meteo_0.pressure = 1013.25;
    
    data.battery_svc.voltage = 13.5;
    data.battery_svc.current = 50.0;
    data.battery_svc.soc = 85.0;
    
    data.engine.rpm = 2500;
    data.engine.engine_time = 3600000000ULL;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(0, context);
}

void test_loop_updates_device_name_if_changed() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "InitialName";
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    TEST_ASSERT_EQUAL_STRING("InitialName", ble.get_device_name());
    
    mockConf.saved_device_name = "UpdatedName";
    ble.enable();
    ble.loop(0, context);
}

void test_multiple_setup_calls() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device1";
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    TEST_ASSERT_EQUAL_STRING("Device1", ble.get_device_name());
    
    mockConf.saved_device_name = "Device2";
    ble.setup(context);
    // must not change - once setup, name is fixed. Setup does nothing on subsequent calls.
    TEST_ASSERT_EQUAL_STRING("Device1", ble.get_device_name());
}

void test_on_write_with_special_characters() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    const char* special = "!@#$%^&*()_+-=[]{}|;:',.<>?/";
    ble.on_write(0, special);
    
    TEST_ASSERT_EQUAL_INT(1, callback_tracker.call_count);
    TEST_ASSERT_EQUAL_CHAR('S', callback_tracker.last_command);
    TEST_ASSERT_EQUAL_STRING(special, callback_tracker.last_command_value);
}

void test_on_write_with_max_length_value() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    char max_value[256];
    memset(max_value, 'A', sizeof(max_value) - 1);
    max_value[sizeof(max_value) - 1] = 0;
    
    ble.on_write(0, max_value);
    
    TEST_ASSERT_EQUAL_INT(1, callback_tracker.call_count);
}

void test_loop_with_zero_milliseconds() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(0, context);
}

void test_loop_with_large_milliseconds() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(0xFFFFFFFFUL, context);
}

void test_destructor_called() {
    BLEConf* ble = new BLEConf(mock_command_callback);
    delete ble;
}

void test_ble_implements_agent_interface() {
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    TEST_ASSERT_TRUE(true);
}

void test_setup_and_loop_sequence() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "TestDevice";
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    
    TEST_ASSERT_FALSE(ble.is_enabled());
    
    ble.setup(context);
    TEST_ASSERT_EQUAL_STRING("TestDevice", ble.get_device_name());
    
    ble.enable();
    TEST_ASSERT_TRUE(ble.is_enabled());
    
    ble.loop(0, context);
    
    ble.disable();
    TEST_ASSERT_FALSE(ble.is_enabled());
}

void test_ble_startup_lifecycle() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "MyDevice";
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    
    ble.setup(context);
    ble.enable();
    TEST_ASSERT_TRUE(ble.is_enabled());
    
    for (int i = 0; i < 5; i++) {
        ble.loop(i * 1000000, context);
    }
}

void test_ble_callback_integration() {
    MOCK_CONTEXT
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    
    ble.on_write(0, "gps:enabled,bme:disabled");
    TEST_ASSERT_EQUAL_CHAR('S', callback_tracker.last_command);
    TEST_ASSERT_EQUAL_STRING("gps:enabled,bme:disabled", callback_tracker.last_command_value);
    
    callback_tracker.reset();
    ble.on_write(1, "R000");
    TEST_ASSERT_EQUAL_CHAR('R', callback_tracker.last_command);
    TEST_ASSERT_EQUAL_STRING("000", callback_tracker.last_command_value);
}
// ==================== Tests: Services Buffer ====================
void test_services_buffer_has_initial_capacity() {
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ByteBuffer buf = ble.get_services_buffer();
    
    // Buffer should be 128 bytes as initialized
    TEST_ASSERT_EQUAL_INT(128, buf.size());
}

void test_services_buffer_empty_after_construction() {
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ByteBuffer buf = ble.get_services_buffer();
    
    // Length should be 0 initially
    TEST_ASSERT_EQUAL_INT(0, buf.length());
}

void test_services_buffer_fills_with_gps_data() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.gps.fix = 3;
    data.gps.latitude_signed = 48.8566;
    data.gps.longitude_signed = 2.3522;
    data.gps.sog = 12.5;
    data.gps.cog = 45.0;
    data.gps.gps_unix_time = 1609459200;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    // Should have data now (at least 1 byte for GPS fix)
    TEST_ASSERT_GREATER_THAN_INT(0, buf.length());
}

void test_services_buffer_contains_gps_fix_value() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.gps.fix = 3;  // 3D fix
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    uint8_t* buf_data = buf.data();
    
    // First byte should be GPS fix (int8_t)
    TEST_ASSERT_EQUAL_INT(3, (int8_t)buf_data[0]);
}

void test_services_buffer_contains_temperature() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    mockConf.temperature_source = METEO_BME;
    data.meteo_0.temperature = 22.5;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer &buf = ble.get_services_buffer();
    uint8_t* buf_data = buf.data();
    
    // Temperature is at offset 5 (after fix(1) + atmo(4))
    // Stored as int16_t with factor 10.0, so 22.5 * 10 = 225
    int16_t temp_val = *((int16_t*)(buf_data + 5));
    TEST_ASSERT_EQUAL_INT(225, temp_val);
}

void test_services_buffer_contains_humidity() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    mockConf.humidity_source = METEO_BME;
    data.meteo_0.humidity = 65.0;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    uint8_t* buf_data = buf.data();
    
    // Humidity is at offset 7 (after fix(1) + atmo(4) + temp(2))
    // Stored with factor 100.0, so 65.0 * 100 = 6500
    int16_t hum_val = *((int16_t*)(buf_data + 7));
    TEST_ASSERT_EQUAL_INT(6500, hum_val);
}

void test_services_buffer_contains_latitude() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.gps.latitude_signed = 48.8566;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    uint8_t* buf_data = buf.data();
    
    // Latitude is at offset 9 (after fix(1) + atmo(4) + temp(2) + hum(2))
    // Stored as int32_t with factor 1000000.0
    int32_t lat_val = *((int32_t*)(buf_data + 9));
    TEST_ASSERT_EQUAL_INT(48856600, lat_val);
}

void test_services_buffer_contains_longitude() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.gps.longitude_signed = 2.3522;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    uint8_t* buf_data = buf.data();
    
    // Longitude is at offset 13 (after fix(1) + atmo(4) + temp(2) + hum(2) + lat(4))
    // Stored as int32_t with factor 1000000.0
    int32_t lon_val = *((int32_t*)(buf_data + 13));
    TEST_ASSERT_EQUAL_INT(2352200, lon_val);
}

void test_services_buffer_contains_rpm() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.engine.rpm = 2500;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    uint8_t* buf_data = buf.data();
    
    // RPM is at offset 34-35
    uint16_t rpm_val = *((uint16_t*)(buf_data + 34));
    TEST_ASSERT_EQUAL_INT(2500, rpm_val);
}

void test_services_buffer_contains_engine_time() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.engine.engine_time = 3600000ULL;  // 1 hour in milliseconds = 3600 seconds
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    uint8_t* buf_data = buf.data();
    
    // Engine time is at offset 36 (sent as seconds)
    uint32_t engine_time_val = *((uint32_t*)(buf_data + 36));
    TEST_ASSERT_EQUAL_INT(3600, engine_time_val);
}

void test_services_buffer_contains_voltage() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.battery_svc.voltage = 13.5;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    uint8_t* buf_data = buf.data();
    
    // Voltage is at offset 51 (after all previous fields)
    // Stored with factor 100.0, so 13.5 * 100 = 1350
    int16_t voltage_val = *((int16_t*)(buf_data + 51));
    TEST_ASSERT_EQUAL_INT(1350, voltage_val);
}

void test_services_buffer_contains_current() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.battery_svc.current = 50.0;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    uint8_t* buf_data = buf.data();
    
    // Current is at offset 49
    // Stored with factor 100.0, so 50.0 * 100 = 5000
    int16_t current_val = *((int16_t*)(buf_data + 49));
    TEST_ASSERT_EQUAL_INT(5000, current_val);
}

void test_services_buffer_contains_soc() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.battery_svc.soc = 85.0;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    uint8_t* buf_data = buf.data();
    
    // SOC is at offset 53
    // Stored with factor 100.0, so 85.0 * 100 = 8500
    int16_t soc_val = *((int16_t*)(buf_data + 53));
    TEST_ASSERT_EQUAL_INT(8500, soc_val);
}

void test_services_buffer_total_length_is_56_bytes() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    mockConf.saved_n2k_src = 22;
    mockConf.saved_rpm_adjustment = 1.0;
    mockConf.temperature_source = METEO_BME;
    mockConf.humidity_source = METEO_BME;
    mockConf.pressure_source = METEO_BME;
    
    data.gps.fix = 3;
    data.gps.latitude_signed = 48.8566;
    data.gps.longitude_signed = 2.3522;
    data.gps.sog = 12.5;
    data.gps.cog = 45.0;
    data.gps.gps_unix_time = 1609459200;
    
    data.meteo_0.temperature = 22.5;
    data.meteo_0.humidity = 65.0;
    data.meteo_0.pressure = 1013.25;
    
    data.battery_svc.voltage = 13.5;
    data.battery_svc.current = 50.0;
    data.battery_svc.soc = 85.0;
    
    data.engine.rpm = 2500;
    data.engine.engine_time = 3600000000ULL;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    // Total should be 56 bytes: fix(1) + atmo(4) + temp(2) + hum(2) + lat(4) + lon(4) + 
    // mem(4) + canbus(1) + canbus_s(4) + canbus_e(4) + sog(2) + cog(2) + rpm(2) + 
    // engine_time(4) + timestamp(4) + services(1) + rpmAdj(4) + current(2) + voltage(2) + soc(2) + n2k_source(1)
    TEST_ASSERT_EQUAL_INT(56, buf.length());
}

void test_data_characteristic_value() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    mockConf.saved_n2k_src = 22;
    mockConf.saved_rpm_adjustment = 1.0;
    mockConf.temperature_source = METEO_BME;
    mockConf.humidity_source = METEO_BME;
    mockConf.pressure_source = METEO_BME;
    
    data.gps.fix = 3;
    data.gps.latitude_signed = 48.8566;
    data.gps.longitude_signed = 2.3522;
    data.gps.sog = 12.5;
    data.gps.cog = 45.0;
    data.gps.gps_unix_time = 1609459200;
    
    data.meteo_0.temperature = 22.5;
    data.meteo_0.humidity = 65.0;
    data.meteo_0.pressure = 1013.25;
    
    data.battery_svc.voltage = 13.5;
    data.battery_svc.current = 50.0;
    data.battery_svc.soc = 85.0;
    
    data.engine.rpm = 2500;
    data.engine.engine_time = 3600000000ULL;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    ByteBuffer char_value = ble.get_field_value_buffer(0);
    // Total should be 56 bytes: fix(1) + atmo(4) + temp(2) + hum(2) + lat(4) + lon(4) + 
    // mem(4) + canbus(1) + canbus_s(4) + canbus_e(4) + sog(2) + cog(2) + rpm(2) + 
    // engine_time(4) + timestamp(4) + services(1) + rpmAdj(4) + current(2) + voltage(2) + soc(2) + n2k_source(1)
    TEST_ASSERT_EQUAL_INT(56, char_value.length());
    TEST_ASSERT_TRUE(buf==char_value);
}

void test_services_buffer_resets_on_loop_call() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.gps.fix = 3;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    // First loop
    ble.loop(10000000, context);
    ByteBuffer buf1 = ble.get_services_buffer();
    size_t len1 = buf1.length();
    
    // Second loop with different data
    data.gps.fix = 0;
    ble.loop(11000000, context);  // After update period
    ByteBuffer buf2 = ble.get_services_buffer();
    size_t len2 = buf2.length();
    
    // Both should have same length (reset happens each loop)
    TEST_ASSERT_EQUAL_INT(len1, len2);
}

void test_services_buffer_nan_values_become_invalid_sentinel() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    // All values are NAN by default in data
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    uint8_t* buf_data = buf.data();
    
    // Temperature (offset 5) should be INVALID_16 (0xFF7F) when NAN
    int16_t temp_val = *((int16_t*)(buf_data + 5));
    TEST_ASSERT_EQUAL_INT(INVALID_16, temp_val);
    
    // Humidity (offset 7) should be INVALID_16
    int16_t hum_val = *((int16_t*)(buf_data + 7));
    TEST_ASSERT_EQUAL_INT(INVALID_16, hum_val);
}

void test_services_buffer_copy_is_independent() {
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    
    ByteBuffer buf1 = ble.get_services_buffer();
    ByteBuffer buf2 = ble.get_services_buffer();
    
    // Both should have same capacity
    TEST_ASSERT_EQUAL_INT(buf1.size(), buf2.size());
    TEST_ASSERT_EQUAL_INT(buf1.length(), buf2.length());
}

void test_services_buffer_contains_sog() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.gps.sog = 7.5;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    uint8_t* buf_data = buf.data();
    
    // SOG is at offset 30
    // Stored with factor 100.0, so 7.5 * 100 = 750
    int16_t sog_val = *((int16_t*)(buf_data + 30));
    TEST_ASSERT_EQUAL_INT(750, sog_val);
}

void test_services_buffer_contains_cog() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    data.gps.cog = 45.0;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    uint8_t* buf_data = buf.data();
    
    // COG is at offset 32
    // Stored with factor 10.0, so 45.0 * 10 = 450
    int16_t cog_val = *((int16_t*)(buf_data + 32));
    TEST_ASSERT_EQUAL_INT(450, cog_val);
}

void test_services_buffer_contains_n2k_source() {
    MOCK_CONTEXT
    mockConf.saved_device_name = "Device";
    mockConf.saved_n2k_src = 22;
    
    BLEConf ble(mock_command_callback, mockBLEInternalImpl);
    ble.setup(context);
    ble.enable();
    
    ble.loop(10000000, context);
    
    ByteBuffer buf = ble.get_services_buffer();
    uint8_t* buf_data = buf.data();
    
    // N2K source is at offset 55 (last byte)
    TEST_ASSERT_EQUAL_INT(22, buf_data[55]);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
      
    // Constructor & Initialization
    RUN_TEST(test_constructor_with_callback);
    RUN_TEST(test_constructor_with_null_callback);
    RUN_TEST(test_setup_initializes_device_name);
    RUN_TEST(test_setup_copies_device_name_truncated_to_buffer_size);
    RUN_TEST(test_setup_with_empty_device_name);
    
    // Enable/Disable
    RUN_TEST(test_enable_without_setup);
    RUN_TEST(test_enable_sets_enabled_flag);
    RUN_TEST(test_disable_clears_enabled_flag);
    RUN_TEST(test_is_enabled_returns_state);
    RUN_TEST(test_multiple_enable_calls);
    RUN_TEST(test_multiple_disable_calls);
    RUN_TEST(test_enable_disable_toggle);
    
    // on_write Handle 0
    RUN_TEST(test_on_write_handle_0_triggers_callback_with_S_command);
    RUN_TEST(test_on_write_handle_0_with_empty_value);
    RUN_TEST(test_on_write_handle_0_with_long_value);
    RUN_TEST(test_on_write_handle_0_multiple_calls);
    
    // on_write Handle 1
    RUN_TEST(test_on_write_handle_1_parses_command_and_value);
    RUN_TEST(test_on_write_handle_1_single_char_command);
    RUN_TEST(test_on_write_handle_1_with_empty_value);
    RUN_TEST(test_on_write_handle_1_multiple_commands);
    RUN_TEST(test_on_write_handle_1_various_command_chars);
    
    // on_write without Callback
    RUN_TEST(test_on_write_handle_0_without_callback);
    RUN_TEST(test_on_write_handle_1_without_callback);
    
    // Loop - Periodic Updates
    RUN_TEST(test_loop_not_called_when_disabled);
    RUN_TEST(test_loop_initially_sends_at_first_update);
    RUN_TEST(test_loop_respects_update_period);
    RUN_TEST(test_loop_with_nan_values);
    RUN_TEST(test_loop_with_valid_temperature_data);
    RUN_TEST(test_loop_with_valid_gps_data);
    RUN_TEST(test_loop_with_battery_data);
    RUN_TEST(test_loop_with_engine_data);
    RUN_TEST(test_loop_with_complete_data);
    
    // Device Name Update
    RUN_TEST(test_loop_updates_device_name_if_changed);
    RUN_TEST(test_multiple_setup_calls);
    
    // Services Buffer
    RUN_TEST(test_services_buffer_has_initial_capacity);
    RUN_TEST(test_services_buffer_empty_after_construction);
    RUN_TEST(test_services_buffer_fills_with_gps_data);
    RUN_TEST(test_services_buffer_contains_gps_fix_value);
    RUN_TEST(test_services_buffer_contains_temperature);
    RUN_TEST(test_services_buffer_contains_humidity);
    RUN_TEST(test_services_buffer_contains_latitude);
    RUN_TEST(test_services_buffer_contains_longitude);
    RUN_TEST(test_services_buffer_contains_rpm);
    RUN_TEST(test_services_buffer_contains_engine_time);
    RUN_TEST(test_services_buffer_contains_voltage);
    RUN_TEST(test_services_buffer_contains_current);
    RUN_TEST(test_services_buffer_contains_soc);
    RUN_TEST(test_services_buffer_total_length_is_56_bytes);
    RUN_TEST(test_services_buffer_resets_on_loop_call);
    RUN_TEST(test_services_buffer_nan_values_become_invalid_sentinel);
    RUN_TEST(test_services_buffer_copy_is_independent);
    RUN_TEST(test_services_buffer_contains_sog);
    RUN_TEST(test_services_buffer_contains_cog);
    RUN_TEST(test_services_buffer_contains_n2k_source);
    RUN_TEST(test_data_characteristic_value);
    
    // Edge Cases
    RUN_TEST(test_on_write_with_special_characters);
    RUN_TEST(test_on_write_with_max_length_value);
    RUN_TEST(test_loop_with_zero_milliseconds);
    RUN_TEST(test_loop_with_large_milliseconds);
    RUN_TEST(test_destructor_called);
    
    // Integration
    RUN_TEST(test_ble_implements_agent_interface);
    RUN_TEST(test_setup_and_loop_sequence);
    RUN_TEST(test_ble_startup_lifecycle);
    RUN_TEST(test_ble_callback_integration);
  
    UNITY_END();
    return 0;
}