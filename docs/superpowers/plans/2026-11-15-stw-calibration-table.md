# STW Calibration Table Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the user define a Hz -> knots calibration table (max 5 points) for the STW paddle wheel, edited over BLE, linearly interpolated, switchable on/off.

**Architecture:** A pure-logic `StwCalibration` class (sorted points, validation, interpolation) is owned by `Configuration` (like the BLE passkey) and persisted as an NVS blob through a new persistence interface. `CommandHandler` gets three commands (`U`, `D`, `Z`), `N2KServices` gets a `use_stw_table` flag, the BLE data blob grows to layout v12 carrying the table, and `SpeedThroughWater` converts frequency through `Configuration::get_stw_speed()`.

**Tech Stack:** C++ (Arduino/ESP32, PlatformIO), Unity native tests (`pio test -e test`), NVS `Preferences`.

**Spec:** `docs/superpowers/specs/2026-11-15-stw-calibration-table-design.md`

## Global Constraints

- Table: max **5** points `{uint16 hz100, uint16 kn100}` (0.01 resolution), sorted by Hz; Hz in (0, 300.00], kn in (0, 50.00].
- Implicit anchor (0 Hz, 0 kn); above the last point extend the last segment's slope.
- kn must be **strictly increasing** with Hz after every edit; invalid edit -> rejected, table unchanged, logged.
- Table off: `speed = Hz * adjustment / 4.8`. Table on: `s` ignored; table on + empty: `speed = Hz / 4.8`.
- Do **not** touch the `Conf` struct, `CONF_VERSION` (0x06), `Context`, or the `CommandHandler::on_command` signature.
- Persistence: NVS namespace `n2krouter`, key `stw_cal`; written before swapped in; corrupt/missing blob = empty table.
- Commands (display units, `.` decimal): `U` `hz,kn`; `D` `hz`; `Z` (no payload). Letters are currently unused.
- Services flag `use_stw_table` = bit 11, `MAX_CONF` 11 -> 12, default off.
- BLE data blob: `BUFFER_LAYOUT_VERSION` 11 -> 12; after `_uere` (offset 93): `uint8` count, then 5 fixed slots of (`uint16` hz100, `uint16` kn100), unused slots zero; total 114 bytes (buffer is 128).
- Only the loop task touches the table (commands arrive via `DeferredEvents`); no logging from other tasks.
- No `n2k_tools_libs` bump (stays 1.0.3).

## Review Focus

- `U` with garbage (`"abc"`, `"1,2,3"`, `"9.6"`, `"9.6,"`, `"nan,1"`, trailing junk `"9.6,2.0x"`): rejected, table untouched.
- Editing the middle point so it crosses its neighbour (e.g. points (4,1),(8,2),(12,3); `U 8,3.5`): rejected, not silently re-sorted into a non-monotonic table.
- 6th point on a full table: rejected, but replacing an existing Hz on a full table works.
- `D` for a Hz that is not in the table, and `Z` on an empty table: no crash, no error state.
- Interpolation at exactly a point, at 0 Hz, at negative/NaN Hz (NaN must not propagate as a bogus finite speed), far above the range.
- Persistence write failure: in-memory table keeps its old value.
- Old app sending an 11-char services string: table flag ends up off, nothing else breaks.

---

## File Structure

- Create `src/StwCalibration.h`, `src/StwCalibration.cpp` — table data, validation, interpolation, command-text parsing helpers. No hardware, no Log dependency beyond `Log::tracex` in the command layer (keep this class silent).
- Modify `src/Conf.h`, `src/Conf.cpp` — services flag; `StwCalibrationPersistence` interface + NVS/memory/dummy implementations; table ownership and accessors in `Configuration`; `get_stw_speed`.
- Modify `src/Constants.h` — limits and the 4.8 constant.
- Modify `src/CommandHandler.hpp` — `U`, `D`, `Z`.
- Modify `src/BLEConf.h`, `src/BLEConf.cpp` — offsets, layout v12.
- Modify `src/SpeedThroughWater.cpp` — use `get_stw_speed`.
- Create `test/test_stw_calibration/test_stw_calibration.cpp` (own `main`); modify `test/test_commands`, `test/test_bleconf`, `test/test_configuration` (services size/strings).
- Modify `CLAUDE.md` — command table, flag, layout, test filter.

---

### Task 1: StwCalibration table (pure logic)

**Files:**
- Create: `src/StwCalibration.h`, `src/StwCalibration.cpp`
- Modify: `src/Constants.h` (add `STW_TABLE_MAX_POINTS 5`, `STW_TABLE_SCALE 100.0`, `STW_TABLE_MAX_HZ 300.0`, `STW_TABLE_MAX_KN 50.0`, `STW_DEFAULT_HZ_PER_KN 4.8`)
- Test: `test/test_stw_calibration/test_stw_calibration.cpp`

**Interfaces:**
- Consumes: nothing.
- Produces:
  ```cpp
  struct StwPoint { uint16_t hz100; uint16_t kn100; };
  class StwCalibration {
  public:
    uint8_t size() const;
    const StwPoint &point(uint8_t i) const;           // i < size()
    bool upsert(double hz, double kn);                // validation per Global Constraints; false = unchanged
    bool remove(double hz);                           // false if no point at that hz100
    void clear();
    double speed(double frequency_hz) const;          // interpolation; empty table = f/4.8; NaN in -> NaN out; negative f -> 0
    bool load(const uint8_t *blob, size_t len);       // validates; false + cleared on failure
    size_t store(uint8_t *blob, size_t cap) const;    // returns bytes written (1 version + 1 count + 4*n), 0 if cap too small
    static constexpr size_t MAX_BLOB = 2 + 4 * STW_TABLE_MAX_POINTS;
  };
  bool parse_stw_point(const char *s, double &hz, double &kn);  // strict "hz,kn": finite, no trailing junk, no spaces required
  bool parse_stw_hz(const char *s, double &hz);                 // strict single number
  ```
  Hz/kn are converted to x100 with rounding (`round(v*100)`); two Hz are "the same point" when hz100 matches.

- [ ] **Step 1: Write failing tests** in `test/test_stw_calibration/test_stw_calibration.cpp` (own `setUp/tearDown/main`, `RUN_TEST` per test). Names and key assertions:
  - `test_empty_table_uses_default_factor`: `speed(9.6)` == 2.0 (within 1e-9); `speed(0)` == 0.
  - `test_single_point_is_line_through_origin`: after `upsert(4.8, 1.0)`, `speed(2.4)` == 0.5, `speed(9.6)` == 2.0 (extrapolated).
  - `test_interpolates_between_points`: points (4.8,1.0),(10.0,2.5) -> `speed(7.4)` == 1.75; at exact points returns exactly 1.0 and 2.5.
  - `test_extends_last_slope_above_range`: same points -> `speed(15.2)` == 2.5 + (5.2 * 1.5/5.2) = 4.0.
  - `test_below_first_point_interpolates_from_origin`: points (4.8,1.0): `speed(2.4)` == 0.5.
  - `test_nan_and_negative_frequency`: `isnan(speed(NAN))`; `speed(-1.0)` == 0.
  - `test_upsert_keeps_sorted`: insert Hz 10, 4.8, 7 in that order -> `point(0..2).hz100` = 480,700,1000.
  - `test_upsert_same_hz_replaces`: `upsert(4.8,1.0)` then `upsert(4.8,1.1)` -> size 1, kn100 110.
  - `test_upsert_rejects_non_monotonic`: with (4,1),(8,2),(12,3): `upsert(8,3.5)` false; `upsert(6,0.9)` false; `upsert(10,1.5)` false; table unchanged (size 3, point(1).kn100 == 200).
  - `test_upsert_rejects_out_of_range`: hz 0, -1, 300.01, NaN; kn 0, -1, 50.01, NaN -> all false.
  - `test_upsert_full_table`: 5 points ok; 6th distinct Hz false; replacing an existing Hz (valid monotonic) true and size stays 5.
  - `test_remove`: removing existing -> true, size shrinks, order kept; missing hz -> false; empty table -> false.
  - `test_clear`.
  - `test_store_load_roundtrip`: 3 points -> `store` then `load` into a fresh table yields identical points; `store` with cap < needed returns 0.
  - `test_load_rejects_corrupt`: wrong version byte, count > 5, truncated length, non-monotonic payload, hz100 == 0 -> `load` false and table empty.
  - `test_parse_point_strict`: accepts `"9.6,2.0"`, `"9.6,2"`, `"0.5,0.1"`; rejects `""`, `"abc"`, `"9.6"`, `"9.6,"`, `",2"`, `"1,2,3"`, `"9.6,2.0x"`, `"nan,1"`, `"inf,1"`, `"9.6,-2"`.
  - `test_parse_hz_strict`: accepts `"9.6"`, `"10"`; rejects `""`, `"x"`, `"9.6x"`, `"nan"`.
- [ ] **Step 2: Run to verify failure**: `pio test -e test --filter test_stw_calibration` -> FAIL (compile error: header missing).
- [ ] **Step 3: Implement** `StwCalibration` and the two parse functions per the Interfaces block. Approach: fixed array of 5 `StwPoint`, `upsert` builds the candidate sorted array in a copy, validates strict monotonicity of kn over the sorted points (the implicit (0,0) anchor is below the first point automatically since kn > 0), then commits. `speed` walks segments, using `(0,0)` as the start of the first segment and the last segment's slope for extrapolation. Blob format: byte 0 = version 1, byte 1 = count, then `uint16 hz100, uint16 kn100` little-endian per point.
- [ ] **Step 4: Run** `pio test -e test --filter test_stw_calibration` -> all PASS.
- [ ] **Step 5: Commit** `git add src/StwCalibration.* src/Constants.h test/test_stw_calibration && git commit -m "feat: STW calibration table (interpolation, validation, blob format)"`

---

### Task 2: Configuration owns the table (persistence, flag, speed conversion)

**Files:**
- Modify: `src/Conf.h`, `src/Conf.cpp`
- Test: `test/test_configuration/` (new file `test_stw_table_conf.cpp` included from the suite's main, following how `test_n2k_services.cpp` is wired — read `test/test_configuration/main.cpp` first), plus update `test_n2k_services.cpp` size/strings.

**Interfaces:**
- Consumes: `StwCalibration` (Task 1).
- Produces:
  ```cpp
  class StwCalibrationPersistence {            // Conf.h, next to BlePasskeyPersistence
  public:
    virtual bool init_persistence() = 0;
    virtual bool save_stw_calibration(const uint8_t *blob, size_t len) = 0;
    virtual size_t load_stw_calibration(uint8_t *blob, size_t cap) = 0;   // 0 = nothing stored
  };
  // N2KServices: SVC_ACCESSOR_H(use_stw_table)  -> is_use_stw_table()/set_use_stw_table(bool)
  // Configuration:
  Configuration(ConfigurationPersistence* = nullptr, BlePasskeyPersistence* = nullptr, StwCalibrationPersistence* = nullptr);
  virtual const StwCalibration &get_stw_table() const;
  virtual bool save_stw_table_point(double hz, double kn);   // upsert + persist; false if invalid or write fails (table unchanged)
  virtual bool delete_stw_table_point(double hz);            // same contract
  virtual bool clear_stw_table();                            // same contract
  virtual double get_stw_speed(double frequency_hz) const;   // table on -> table.speed(f); off -> f * get_stw_paddle_adjustment() / STW_DEFAULT_HZ_PER_KN
  ```
  `MockConfiguration` gains `save_stw_table_point_calls`, `delete_stw_table_point_calls`, `clear_stw_table_calls` (counted in overrides, reset in `reset_call_counts()`), and passes a dummy in-memory table persistence.

- [ ] **Step 1: Write failing tests**:
  - services: `SVC_ID 11` defaults off (`DEFAULT_USE_STW_TABLE 0` in Constants.h); `size()` == 12; `to_string` all-on == `"111111111111"`; `from_string("11111111111")` (11 chars) leaves `is_use_stw_table()` false; update the existing `"11111111111"`/`"00000000000"`/`"11001010101"` expectations in `test_n2k_services.cpp` to 12 chars (append `0` where the input stays 11 chars of input, or extend the inputs — keep each test's intent).
  - `test_get_stw_speed_table_off_uses_adjustment`: `save_stw_paddle_adjustment(1.1)`; `get_stw_speed(4.8)` == 1.1.
  - `test_get_stw_speed_table_on_ignores_adjustment`: adjustment 1.1, table (4.8,1.0),(9.6,2.2), flag on -> `get_stw_speed(4.8)` == 1.0, `get_stw_speed(7.2)` == 1.6.
  - `test_get_stw_speed_table_on_but_empty`: flag on, adjustment 1.5 -> `get_stw_speed(9.6)` == 2.0.
  - `test_table_persisted_and_loaded`: a recording `StwCalibrationPersistence` fake; `save_stw_table_point` x2 then a new `Configuration` with the same fake, `init()` -> same 2 points.
  - `test_table_write_failure_leaves_table`: fake returns false from save -> `save_stw_table_point` false and `get_stw_table().size()` unchanged; same for delete and clear.
  - `test_invalid_point_not_persisted`: invalid point -> false and fake `saves` counter unchanged.
  - `test_corrupt_blob_loads_empty`.
- [ ] **Step 2: Run** `pio test -e test --filter test_configuration` -> FAIL.
- [ ] **Step 3: Implement** in `Conf.h/.cpp`: flag (`SVC_STW_TABLE_ID 11`, `MAX_CONF 12`, default constant, accessor, constructor default mask); persistence interface + `StwCalibrationPersistenceNVS` (`Preferences`, `putBytes/getBytes`, key `stw_cal`, mirrors `BlePasskeyPersistenceNVS`) under `#ifndef NATIVE`, in-memory version under `#else`, and a dummy for `MockConfiguration`; `Configuration` gets the pointer (default = the NVS/memory instance), loads the blob in `init()` (a persistence-init failure logs and leaves the table empty, like the passkey path). Edit methods work on a copy, persist the copy's blob, and only then assign.
- [ ] **Step 4: Run** `pio test -e test --filter test_configuration` -> PASS; then `pio test -e test` -> nothing else regressed.
- [ ] **Step 5: Commit** `git add src test && git commit -m "feat: STW table in Configuration with NVS persistence and use_stw_table flag"`

---

### Task 3: BLE commands U / D / Z

**Files:**
- Modify: `src/CommandHandler.hpp`
- Test: `test/test_commands/test_commands.cpp`

**Interfaces:**
- Consumes: `parse_stw_point`, `parse_stw_hz`, `Configuration::save_stw_table_point / delete_stw_table_point / clear_stw_table`.
- Produces: `COMMAND_STW_TABLE_UPSERT = 'U'`, `COMMAND_STW_TABLE_DELETE = 'D'`, `COMMAND_STW_TABLE_CLEAR = 'Z'`.

- [ ] **Step 1: Write failing tests** (register in the suite's `main` like the others):
  - `test_command_U_adds_point`: `on_command('U', "9.6,2.0", ...)` -> `conf.save_stw_table_point_calls == 1`, table size 1, `point(0).hz100 == 960`, `kn100 == 200`.
  - `test_command_U_replaces_point`.
  - `test_command_U_invalid_payloads_ignored`: loop over `""`, `"abc"`, `"9.6"`, `"9.6,"`, `"1,2,3"`, `"9.6,2.0x"`, `"nan,1"`, `"9.6,-2"`, `"0,1"`, `"400,1"` -> table stays empty (the call counter may be 0 for unparseable input; for parseable-but-invalid values only assert the table is unchanged).
  - `test_command_U_non_monotonic_rejected`: (4,1),(8,2),(12,3) then `U 8,3.5` -> point(1).kn100 still 200.
  - `test_command_D_removes_point`, `test_command_D_unknown_hz_is_harmless`, `test_command_D_invalid_payload`.
  - `test_command_Z_clears`, `test_command_Z_on_empty_is_harmless`.
  - `test_command_S_toggles_table_flag`: `on_command('S', "000000000001", ...)` -> `is_use_stw_table()` true.
- [ ] **Step 2: Run** `pio test -e test --filter test_commands` -> FAIL.
- [ ] **Step 3: Implement** the three cases in `CommandHandler::on_command`, in the style of the existing ones (`Log::tracex(CMD_LOG_TAG, ...)`, then act; log a line when rejected).
- [ ] **Step 4: Run** `pio test -e test --filter test_commands` -> PASS.
- [ ] **Step 5: Commit** `git add src/CommandHandler.hpp test/test_commands && git commit -m "feat: BLE commands U/D/Z to edit the STW calibration table"`

---

### Task 4: BLE data blob layout v12

**Files:**
- Modify: `src/BLEConf.h` (add `BUFFER_OFFSET_STW_TABLE_COUNT 93`, `BUFFER_OFFSET_STW_TABLE 94`), `src/BLEConf.cpp` (`BUFFER_LAYOUT_VERSION 12`, append after `_uere`; update the trailing comment)
- Test: `test/test_bleconf/test_bleconf.cpp`

**Interfaces:**
- Consumes: `Configuration::get_stw_table()`.
- Produces: blob bytes 93 = count, 94..113 = 5 x (`uint16 hz100`, `uint16 kn100`), unused slots zero. Version byte (offset 0) = 12.

- [ ] **Step 1: Write failing tests**: update the two `93` length assertions to `114` (and their comments); add `test_services_buffer_version_is_12`; `test_services_buffer_contains_stw_table`: table (4.8,1.0),(9.6,2.2) -> count byte 2, slot0 = (480,100), slot1 = (960,220), slots 2..4 all zero; `test_services_buffer_stw_table_empty`: count 0, 20 zero bytes. Also an assertion that `buf.size()` (capacity) is still 128.
- [ ] **Step 2: Run** `pio test -e test --filter test_bleconf` -> FAIL.
- [ ] **Step 3: Implement** in `fill_buffer`: after `<< _uere`, write the count and always 5 slots, zero-filling missing ones.
- [ ] **Step 4: Run** `pio test -e test --filter test_bleconf` -> PASS.
- [ ] **Step 5: Commit** `git add src/BLEConf.* test/test_bleconf && git commit -m "feat: STW calibration table in BLE data blob (layout v12)"`

---

### Task 5: Use the table in SpeedThroughWater, docs, full build

**Files:**
- Modify: `src/SpeedThroughWater.cpp:61` (`data.speed = conf.get_stw_speed(frequency);`), drop the now-unused `// 4.8Hz = 1Kn` inline math (keep the header comment about the ST810 spec)
- Modify: `CLAUDE.md` (BLE command table: `U`, `D`, `Z`; services flag; data-blob layout v12 note; `test_stw_calibration` filter; Configuration section mentions the NVS table)

**Interfaces:**
- Consumes: `Configuration::get_stw_speed(double)`.
- Produces: nothing further.

- [ ] **Step 1: Edit** `SpeedThroughWater.cpp` to call `conf.get_stw_speed(frequency)`; no new test here (the conversion is covered by Task 2's `get_stw_speed` tests; `SpeedThroughWater` itself is hardware-bound and has no native tests).
- [ ] **Step 2: Update CLAUDE.md** as listed above.
- [ ] **Step 3: Run the full native suite**: `pio test -e test` -> all suites PASS.
- [ ] **Step 4: Build every release env**: `pio run -e esp32-s3-3-0`, `pio run -e esp32-s3-1-0`, `pio run -e esp32-s3-gps-uart-3-0` -> SUCCESS (checks the NVS persistence path compiles, which native tests don't).
- [ ] **Step 5: Commit** `git add src/SpeedThroughWater.cpp CLAUDE.md && git commit -m "feat: apply STW calibration table to paddle wheel speed; document"`

---

## Self-review notes

- Spec coverage: data model/interpolation/validation (T1), ownership/persistence/flag/speed rule (T2), commands (T3), blob v12 (T4), speed computation + docs (T5). Tests section of the spec: `test_n2k_router`/agents additions are covered by `get_stw_speed` tests in T2 instead (SpeedThroughWater has no native harness).
- Type names used across tasks: `StwCalibration`, `StwPoint`, `StwCalibrationPersistence`, `get_stw_table`, `save_stw_table_point`, `delete_stw_table_point`, `clear_stw_table`, `get_stw_speed`, `parse_stw_point`, `parse_stw_hz` — consistent.
