#ifndef _STW_CALIBRATION_H_
#define _STW_CALIBRATION_H_

#include <stdint.h>
#include <stddef.h>
#include "Constants.h"

/** One calibration point: paddle frequency and speed through water, both x100. */
struct StwPoint
{
    uint16_t hz100;
    uint16_t kn100;
};

/**
 * User-editable Hz -> knots calibration table for the STW paddle wheel.
 * Up to STW_TABLE_MAX_POINTS points, sorted by frequency, with strictly
 * increasing speed. The implicit point (0, 0) anchors the first segment; the
 * last segment's slope is used above the highest point. An empty table uses
 * the default factor (STW_DEFAULT_HZ_PER_KN).
 * Pure logic: no logging, no hardware dependencies.
 */
class StwCalibration
{
public:
    static constexpr size_t MAX_BLOB = 2 + 4 * STW_TABLE_MAX_POINTS;

    uint8_t size() const { return count; }
    const StwPoint &point(uint8_t i) const { return points[i]; }

    /** Insert or replace (same hz100) a point. False (table unchanged) if invalid, non monotonic or full. */
    bool upsert(double hz, double kn);
    /** Remove the point at this frequency. False if there is none. */
    bool remove(double hz);
    void clear() { count = 0; }

    /** Speed in knots for a paddle frequency in Hz. NaN in -> NaN out, negative -> 0. */
    double speed(double frequency_hz) const;

    /** Load from blob (version, count, then little-endian hz100/kn100 pairs). False and cleared on failure. */
    bool load(const uint8_t *blob, size_t len);
    /** Serialize; returns bytes written, 0 if cap is too small. */
    size_t store(uint8_t *blob, size_t cap) const;

private:
    StwPoint points[STW_TABLE_MAX_POINTS] = {};
    uint8_t count = 0;
};

/** Strict "hz,kn" parser: finite non-negative numbers, no trailing junk. */
bool parse_stw_point(const char *s, double &hz, double &kn);
/** Strict single non-negative finite number parser. */
bool parse_stw_hz(const char *s, double &hz);

#endif
