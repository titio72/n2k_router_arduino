# STW paddle wheel calibration table - design

Status: awaiting approval. Size: small change (short design).

## Intent

The paddle wheel is not linear: the fixed `Hz * adjustment / 4.8` conversion is wrong at some speeds. The skipper
wants to define a Hz -> knots curve (calibrated e.g. against GPS runs) from the companion app. The firmware
linearises between the points. A similar function exists in the wind project (`n2k_wind`: correction table).

## Goals

- A calibration table of up to 5 points (Hz, kn), editable over BLE, persisted across reboots.
- Linear interpolation between points.
- The user decides whether the table is used (on/off switch).
- Table off: `speed = Hz * adjustment / 4.8` (today's behaviour, unchanged).
- Table on: the `s` adjustment is ignored. If the table is empty, `speed = Hz / 4.8`.
- The table is readable by the app through the existing BLE data stream.

## Non-goals / anti-goals

- No auto-calibration or curve fitting from GPS, no UI work (app side is separate).
- No change to the alpha filter: it still smooths the frequency before conversion.
- No change to the `Conf` EEPROM layout or `CONF_VERSION` (existing devices must keep their settings).
- A table must never be able to make speed decrease while frequency increases.

## Approaches considered

- **A. Table appended to the existing BLE data blob; row-by-row edit commands** (chosen). 5 x 4 bytes + count = 21
  bytes, blob goes from 93 to 114 of 128 bytes. No new characteristic, no `n2k_tools_libs` bump (1.0.3 stays).
  Each command payload is ~11 chars, fits a default 20-byte BLE write.
- B. Separate read-only characteristic like the wind project: needs the library bump to 1.1.0 and a new UUID; extra
  room is not needed for 5 points.
- C. One command replacing the whole table: ~60 char payload needs MTU negotiation, and a failed write leaves the
  device state ambiguous.

## Design

**Data model** (new `src/StwCalibration.h/.cpp`, pure logic, no hardware): up to 5 points `{uint16 hz100, uint16 kn100}`
(0.01 resolution), kept sorted by Hz. Operations: `upsert(hz, kn)`, `remove(hz)`, `clear()`, `speed_from_frequency(hz)`.

**Interpolation**: implicit anchor (0 Hz, 0 kn). Below the first point: line from the origin. Between points: linear.
Above the last point: extend the last segment's slope (with one point, the line through the origin and that point).

**Validation** (an invalid edit is rejected, the table is unchanged, the reason is logged): Hz in (0, 300], kn in
(0, 50]; a point with the same Hz (x100) replaces the existing one; adding a 6th point is rejected; after the edit
kn must be strictly increasing with Hz across all points. Consequence for the UI: edits that would transiently break
monotonicity (e.g. shifting several points) must be sent in a valid order or after a clear.

**Ownership and threading**: the table lives in `Configuration` (like the BLE passkey), so `Context` and
`CommandHandler::on_command` signatures do not change. Only the loop task touches it; commands arrive through the
existing `DeferredEvents` queue (payload limit 64 chars is plenty).

**Persistence**: NVS blob, namespace `n2krouter`, key `stw_cal` (version byte + count + points), with a persistence
interface and an in-memory implementation for native/test builds (same pattern as engine hours / passkey). A corrupt
or missing blob reads as an empty table. An edit is saved first and swapped in only if the write succeeded.

**Commands** (my choice of letters, all currently unused; values in display units, dot as decimal separator):

| Cmd | Payload | Meaning |
|-----|---------|---------|
| `U` | `hz,kn` e.g. `9.6,2.0` | add a point or replace the one at that Hz |
| `D` | `hz` | delete the point at that Hz |
| `Z` | (none) | clear the table |

**On/off switch**: new `N2KServices` flag `use_stw_table`, bit 11 (`MAX_CONF` 11 -> 12), set through the existing `S`
command. Default off. Stored services values from older firmware have bit 11 = 0, so nothing to migrate. An older app
sending an 11-char services string simply switches the table off.

**BLE data blob**: `BUFFER_LAYOUT_VERSION` 11 -> 12. After `_uere` (offset 93): `uint8` point count, then 5 fixed
slots of (`uint16` Hz x100, `uint16` kn x100), unused slots zero. Total 114 bytes. The on/off state is already in the
services field.

**Speed computation** (`SpeedThroughWater::loop`): if `use_stw_table` -> `table.speed_from_frequency(freq)` (empty
table = `freq / 4.8`); else `freq * adjustment / 4.8`.

## Tests

New `test/test_stw_calibration` (interpolation, below/above range, single point, empty table, validation incl.
monotonicity/full table/replace, persistence round trip and corrupt blob); additions to `test_commands` (`U`/`D`/`Z`,
bad payloads), `test_bleconf` (layout v12), `test_n2k_router`/agents as needed for the on/off switch behaviour in
`SpeedThroughWater`. CLAUDE.md updated (command table, layout, flag).

## Left to the builder

Exact text parsing helper for `hz,kn` (strict: reject trailing junk, NaN, negatives), log wording, mock class shape
(follow `MockConfiguration` conventions), whether the blob helper is a free function or a method.

## Revision 2026-11-17: table is knots -> knots

The table no longer stores Hz -> kn. Points are `raw_kn -> kn`, where raw speed = Hz / 4.8 (`STW_DEFAULT_HZ_PER_KN`),
so a calibration is independent of the paddle's nominal factor and reads as "when the log says X, the true speed is Y".
Both columns are in (0, 50] kn. Interpolation, monotonicity, replace/full-table rules and the on/off switch are
unchanged; an empty table is now the identity (calibrated = raw, same result as before: Hz / 4.8). Commands `U raw,kn`,
`D raw`; BLE blob layout 13 (same size and offsets, slot meaning changed); NVS blob version 2 (version 1 reads as empty).
