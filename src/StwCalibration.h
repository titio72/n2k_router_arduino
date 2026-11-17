#ifndef _STW_CALIBRATION_H_
#define _STW_CALIBRATION_H_

#include <stdint.h>
#include <stddef.h>
#include "Constants.h"

/** One calibration point: raw speed (paddle Hz / STW_DEFAULT_HZ_PER_KN) and calibrated speed, both in knots x100. */
struct StwPoint
{
    uint16_t raw100;
    uint16_t kn100;
};

/**
 * User-editable knots -> knots calibration table for the STW paddle wheel:
 * maps the raw speed (paddle Hz / STW_DEFAULT_HZ_PER_KN) to the true speed.
 * Up to STW_TABLE_MAX_POINTS points, sorted by raw speed, with strictly
 * increasing calibrated speed. The implicit point (0, 0) anchors the first
 * segment; the last segment's slope is used above the highest point. An empty
 * table is the identity (calibrated = raw).
 * Pure logic: no logging, no hardware dependencies.
 */
class StwCalibration
{
public:
    static constexpr size_t MAX_BLOB = 2 + 4 * STW_TABLE_MAX_POINTS;

    uint8_t size() const { return count; }
    const StwPoint &point(uint8_t i) const { return points[i]; }

    /** Insert or replace (same raw100) a point. False (table unchanged) if invalid, non monotonic or full. */
    bool upsert(double raw_kn, double kn);
    /** Remove the point at this raw speed. False if there is none. */
    bool remove(double raw_kn);
    void clear() { count = 0; }

    /** Calibrated speed in knots for a raw speed in knots. NaN in -> NaN out, negative -> 0. */
    double speed(double raw_kn) const;

    /** Load from blob (version, count, then little-endian raw100/kn100 pairs). False and cleared on failure. */
    bool load(const uint8_t *blob, size_t len);
    /** Serialize; returns bytes written, 0 if cap is too small. */
    size_t store(uint8_t *blob, size_t cap) const;

private:
    StwPoint points[STW_TABLE_MAX_POINTS] = {};
    uint8_t count = 0;
};

/** Strict "raw_kn,kn" parser: finite non-negative numbers, no trailing junk. */
bool parse_stw_point(const char *s, double &raw_kn, double &kn);
/** Strict single non-negative finite number parser (a raw speed in knots). */
bool parse_stw_raw(const char *s, double &raw_kn);

#endif
