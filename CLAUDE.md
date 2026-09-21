# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

An NMEA2000 (N2K) gateway for ESP32. Reads marine sensors (GPS, barometer, humidity, engine tachometer, battery monitor via VE.Direct) and publishes data to the N2K CAN bus network. Also exposes sensor data over BLE for configuration and monitoring.

## Build & Flash Commands

```bash
# Build for the current ESP32-S3 board (PCB v3.0, I2C GPS)
pio run -e esp32-s3-3-0

# Flash and monitor
pio run -e esp32-s3-3-0 --target upload
pio device monitor -e esp32-s3-3-0

# Run all unit tests (native/desktop)
pio test -e test

# Run a single test suite
pio test -e test --filter test_tachometer
pio test -e test --filter test_configuration
pio test -e test --filter test_bmv712
pio test -e test --filter test_agents
pio test -e test --filter test_bleconf
pio test -e test --filter test_commands
pio test -e test --filter test_meteo
pio test -e test --filter test_ve_direct
pio test -e test --filter test_deferred
pio test -e test --filter test_reset_button
pio test -e test --filter test_temperature
pio test -e test --filter test_n2k_router
```

Available build environments (see `platformio.ini`):

| Env | Board / GPS |
|-----|-------------|
| `esp32-s3-3-0` | PCB v3.0, I2C GPS (release) |
| `esp32-s3-1-0` | PCB v1.0, I2C GPS, OLED display (release) |
| `esp32-s3-gps-uart-2-0` | PCB v2.0, UART GPS, reset button (debug) |
| `esp32-s3-gps-uart-3-0` | PCB v3.0, UART GPS (release) |
| `test` | native/desktop unit tests |

### Crash Debugging

`pio device monitor` decodes ESP32 panic backtraces automatically (`monitor_filters = esp32_exception_decoder` in `platformio.ini`).

## Architecture

### Agent Pattern

Every sensor/service module is an **Agent**: a class implementing the four methods declared by the `AB_AGENT` macro in `src/Agents.hpp`:

```cpp
void setup(Context &ctx);
void enable(Context &ctx);
void disable(Context &ctx);
bool is_enabled();
void loop(unsigned long time_micros, Context &ctx);
```

`main.cpp` drives every agent via the `handle_agent_loop()` template, which handles enable/disable and retry logic: up to `MAX_RETRY=3` quick attempts, then (when the caller passes a `retry_at` timestamp, as `main.cpp` does) one more attempt every `AGENT_RETRY_COOLDOWN_USEC` (30 s). Agents are always passed a `Context` reference. Hardware that is compiled out (`GPS_TYPE=0`, `DO_VE_DIRECT=0`, `DO_TACHOMETER=0`, no `RESET_PIN`) is replaced by a `Dummy` agent (`src/Dummy.h`).

Whether an agent is enabled at runtime is controlled by the `N2KServices` bitfield stored in `Configuration` (e.g. `conf.get_services().is_use_tacho()`). This is separate from the compile-time `DO_TACHOMETER` flag: the compile-time flag controls whether the agent is compiled in; the runtime flag controls whether `handle_agent_loop()` calls `enable()` on it each cycle.

### Context (src/Context.h)

The central dependency-injection struct passed to all agents:

```cpp
struct Context {
    N2KSender& n2k;      // send NMEA2000 messages to CAN bus
    Configuration& conf; // read persistent configuration
    Data& data_cache;    // shared sensor data cache
};
```

Tests use the `MOCK_CONTEXT` macro (defined in `Context.h`) to instantiate `NullN2KSender`, `MockConfiguration`, and a `Data` object.

### EngineHours (src/Conf.h)

`EngineHours` is a separate class from `Configuration` and is **not** part of `Context`. It stores cumulative engine time in milliseconds, persisted in NVS (`Preferences`, namespace `n2krouter`, key `engine_ms`) rather than in the `Conf` EEPROM buffer, so the `Conf` layout can change without moving it; a stored value above 100 000 h is treated as corrupted and read as 0 (desktop/test builds use an in-memory default). The instance is passed directly to `Tachometer` at construction in `main.cpp`. Tests use `MockEngineHours`, which counts `save_engine_hours()` calls and is defined in `Conf.h` behind `#ifdef PIO_UNIT_TESTING`.

### Data Cache (src/Data.h)

`Data` is the shared in-memory cache written by sensor agents and read by `EnvMessenger`. Key fields:

| Field | Written by | Content |
|-------|-----------|---------|
| `gps` | GPSX | GPSData (fix, lat/lon, COG/SOG, satellites, time) |
| `meteo_0` | MeteoBME | pressure, humidity, temperature from BME280 |
| `meteo_1` | MeteoDHT | temperature/humidity from DHT sensor |
| `battery_svc` | BMV712 | service battery voltage/current/SOC/TTG |
| `battery_eng` | BMV712 | engine/starter battery |
| `engine` | Tachometer | RPM, engine time (ms) |
| `water_data` | SpeedThroughWater, WaterTemperature | STW speed, water temperature |

`Data::get_pressure()`, `get_humidity()`, `get_temperature()`, and `get_temperature_el()` use `Configuration::get_*_source()` (returning `MeteoSource` enum: `METEO_BME=0`, `METEO_DHT=1`, `METEO_NONE=2`) to select which meteo sub-struct to read.

### Threading (src/DeferredEvents.h)

Several tasks run at once: the Arduino loop task (all agents), the N2K task (core 0, NMEA2000 stack), the NimBLE host task (BLE callbacks) and `esp_timer` tasks (pulse sampling). **`Configuration`, `EngineHours` and the `Data` cache are only read/written by the loop task**, and all EEPROM/NVS writes happen there. Callbacks on other tasks must not touch them (or log): they only post to `DeferredEvents` (`post_command`, `post_source_claim`, `post_n2k_activity`) and `_loop()` applies them via `process_deferred_*`. BLE commands therefore take effect within one loop iteration (~5 ms), not inside the BLE callback. Note `Log::tracex` (n2k_tools_libs) uses one shared buffer and is not thread-safe, so avoid logging from non-loop tasks in this repo's code.

### Data Flow

1. Sensor agents (GPS, MeteoBME, MeteoDHT, BMV712, Tachometer, etc.) read hardware and write to `ctx.data_cache`.
2. `EnvMessenger` reads from `ctx.data_cache` every 2 seconds and calls `ctx.n2k.send*()` to push environmental PGNs to the N2K bus.
3. GPS and Tachometer send their own N2K messages directly in their `loop()`.

### N2K Sender Hierarchy

- `N2KSender` (pure virtual interface) — defines all `sendXxx()` methods for each PGN
- `N2KSenderAbstract` — implements all PGN encoding, calls `send_it()` for dispatch
- `N2K_router` — concrete implementation; uses a FreeRTOS queue and a task pinned to core 0 for thread-safe CAN transmission
- `NullN2KSender` — test stub that counts calls without touching hardware

### Configuration (src/Conf.h)

Persisted to EEPROM as a versioned `Conf` struct (`CONF_VERSION = 0x06`). Feature flags are stored as bitfields in `N2KServices` (serialized as `uint16_t`). All `save_*()` methods write through to EEPROM immediately.

`Configuration` and `EngineHours` accept an optional `*Persistence` pointer, defaulting to the real EEPROM implementation. For tests, `MockConfiguration` / `MockEngineHours` are defined inside `#ifdef PIO_UNIT_TESTING` blocks in `Conf.h`.

### Compile-time Feature Flags

Hardware features are selected via preprocessor defines set per environment in `platformio.ini`:

| Flag | Values | Effect |
|------|--------|--------|
| `GPS_TYPE` | 0/1/2 | Dummy / I2C u-blox / UART u-blox |
| `DO_TACHOMETER` | 0/1 | Engine RPM via alternator signal |
| `DO_VE_DIRECT` | 0/1 | Victron BMV712 battery monitor |
| `DO_DISPLAY` | 0/1 | SSD1306 OLED |
| `DO_LOGGER` | 0/1 (default 1) | Serial logging compiled in; the `use_logger` service flag switches it on/off at runtime (also applied at boot: a stored "off" silences the device until re-enabled with the `S` command) |
| `RESET_PIN` | GPIO / -1 (default) | Push button to GND, hold `RESET_HOLD_USEC` (2 s) to reboot (`ResetButton`) |
| `NATIVE` | defined | Desktop build (suppresses Arduino/ESP32 headers) |

All pin assignments (`CAN_TX_PIN`, `ENGINE_RPM_PIN`, `DHT_PIN`, etc.) are also defines, never hardcoded in source.

### BLE Configuration Interface

`BLEConf` exposes the device over BLE. The companion app writes single-character commands dispatched to `CommandHandler::on_command()`:

| Cmd | Meaning |
|-----|---------|
| `S` | Set enabled services bitmask |
| `N` | Set device name |
| `B` | Set battery capacity (Ah) |
| `H` | Set engine hours (seconds) |
| `T` | Tachometer calibration (set actual RPM) |
| `t` | Tachometer adjustment factor (×100) |
| `s` | STW paddle adjustment factor (×100) |
| `a` | STW paddle alpha filter (×100) |
| `w` | Sea temp adjustment factor (×100) |
| `x` | Sea temp alpha filter (×100) |
| `h` | Heartbeat (no-op, resets BLE inactivity timer) |

Pairing: each device has a 6-digit passkey baked into the firmware at flash time by `tools/ble_passkey.py` (a random one on `pio run --target upload`, or `BLE_PASSKEY=123456 pio run ...` to reuse a code). It is printed before and after the upload and appended to `ble_passkeys.log` (git-ignored); `Configuration::get_ble_passkey()` returns it (0 = no passkey, BLE writes open, e.g. tests). Writing to the `conf`/`command` characteristics requires an encrypted, MITM-authenticated (passkey) link; reads stay open. Heartbeats go to a separate `heartbeat` characteristic (`BLE_HEARTBEAT_UUID`) that is writable without pairing and only resets the inactivity timer (`h` on `command` still works for paired clients). Any number of clients can pair (`CONFIG_BT_NIMBLE_MAX_BONDS=10`).

BLE auto-suspends after `BLE_INACTIVITY_TIMEOUT` (30 s) with no activity.

## Testing Conventions

- Framework: Unity (`#include <unity.h>`)
- Test suites live in `test/test_<name>/`; each has its own `main.cpp` calling `UNITY_BEGIN()` / `UNITY_END()`
- Use `MOCK_CONTEXT` to get a fully wired `Context` without hardware
- Mock objects track call counts (e.g. `MockEngineHours::save_engine_hours_calls`) for assertion
- The `test` environment defines `NATIVE`, `DISABLE_LEDS`, and `PIO_UNIT_TESTING`

## Key External Dependencies (from n2k_tools_libs)

The shared library at `https://github.com/titio72/n2k_tools_libs.git` provides headers used throughout:

- `Log.h` — `Log::tracex(tag, label, format, ...)` for structured serial logging
- `Utils.h` — `check_elapsed(now, &t0, period)` for non-blocking timers
- `N2K.h` — thin wrapper around `ttlappalainen/NMEA2000`
- `BTInterface.h` / `ByteBuffer` — BLE abstraction
- `SpeedSensor.h` / `SpeedSensorInterrupt.h` — pulse counting for tachometer
- `Ports.h` / `ArduinoPort.hpp` — serial port abstraction used by VE.Direct
