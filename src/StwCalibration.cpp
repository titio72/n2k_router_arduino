#include "StwCalibration.h"
#include <math.h>
#include <stdlib.h>

#define STW_BLOB_VERSION 1

static bool to_x100(double v, double max, uint16_t &out)
{
    if (!isfinite(v) || v <= 0.0 || v > max)
        return false;
    double r = round(v * STW_TABLE_SCALE);
    if (r < 1.0 || r > max * STW_TABLE_SCALE)
        return false;
    out = (uint16_t)r;
    return true;
}

/** points sorted by hz100 (strictly), all values in range, kn100 strictly increasing. */
static bool valid_points(const StwPoint *p, uint8_t n)
{
    if (n > STW_TABLE_MAX_POINTS)
        return false;
    for (uint8_t i = 0; i < n; i++)
    {
        if (p[i].hz100 == 0 || p[i].kn100 == 0 ||
            p[i].hz100 > (uint16_t)(STW_TABLE_MAX_HZ * STW_TABLE_SCALE) ||
            p[i].kn100 > (uint16_t)(STW_TABLE_MAX_KN * STW_TABLE_SCALE))
            return false;
        if (i > 0 && (p[i].hz100 <= p[i - 1].hz100 || p[i].kn100 <= p[i - 1].kn100))
            return false;
    }
    return true;
}

bool StwCalibration::upsert(double hz, double kn)
{
    uint16_t h, k;
    if (!to_x100(hz, STW_TABLE_MAX_HZ, h) || !to_x100(kn, STW_TABLE_MAX_KN, k))
        return false;

    // a full table only accepts a replacement of an existing frequency
    if (count >= STW_TABLE_MAX_POINTS)
    {
        bool exists = false;
        for (uint8_t i = 0; i < count; i++)
            exists = exists || points[i].hz100 == h;
        if (!exists)
            return false;
    }

    StwPoint cand[STW_TABLE_MAX_POINTS + 1]; // room for one insertion
    uint8_t n = 0;
    bool placed = false;
    for (uint8_t i = 0; i < count; i++)
    {
        if (!placed && points[i].hz100 >= h)
        {
            cand[n++] = {h, k};
            placed = true;
            if (points[i].hz100 == h)
                continue; // replace
        }
        cand[n++] = points[i];
    }
    if (!placed)
        cand[n++] = {h, k};
    if (n > STW_TABLE_MAX_POINTS || !valid_points(cand, n))
        return false;
    for (uint8_t i = 0; i < n; i++)
        points[i] = cand[i];
    count = n;
    return true;
}

bool StwCalibration::remove(double hz)
{
    if (!isfinite(hz))
        return false;
    double r = round(hz * STW_TABLE_SCALE);
    for (uint8_t i = 0; i < count; i++)
    {
        if ((double)points[i].hz100 == r)
        {
            for (uint8_t j = i + 1; j < count; j++)
                points[j - 1] = points[j];
            count--;
            return true;
        }
    }
    return false;
}

double StwCalibration::speed(double f) const
{
    if (isnan(f))
        return f;
    if (f <= 0.0)
        return 0.0;
    if (count == 0)
        return f / STW_DEFAULT_HZ_PER_KN;

    double x0 = 0.0, y0 = 0.0;
    double x1 = 0.0, y1 = 0.0;
    for (uint8_t i = 0; i < count; i++)
    {
        x1 = points[i].hz100 / STW_TABLE_SCALE;
        y1 = points[i].kn100 / STW_TABLE_SCALE;
        if (f == x1)
            return y1;
        if (f < x1)
            return y0 + (f - x0) * (y1 - y0) / (x1 - x0);
        x0 = x1;
        y0 = y1;
    }
    // above the last point: extend the last segment (from origin when there is only one point)
    if (count >= 2)
    {
        x0 = points[count - 2].hz100 / STW_TABLE_SCALE;
        y0 = points[count - 2].kn100 / STW_TABLE_SCALE;
    }
    else
    {
        x0 = 0.0;
        y0 = 0.0;
    }
    return y1 + (f - x1) * (y1 - y0) / (x1 - x0);
}

bool StwCalibration::load(const uint8_t *blob, size_t len)
{
    clear();
    if (blob == nullptr || len < 2 || blob[0] != STW_BLOB_VERSION)
        return false;
    uint8_t n = blob[1];
    if (n > STW_TABLE_MAX_POINTS || len < (size_t)(2 + 4 * n))
        return false;
    StwPoint p[STW_TABLE_MAX_POINTS];
    for (uint8_t i = 0; i < n; i++)
    {
        const uint8_t *b = blob + 2 + 4 * i;
        p[i].hz100 = (uint16_t)(b[0] | (b[1] << 8));
        p[i].kn100 = (uint16_t)(b[2] | (b[3] << 8));
    }
    if (!valid_points(p, n))
        return false;
    for (uint8_t i = 0; i < n; i++)
        points[i] = p[i];
    count = n;
    return true;
}

size_t StwCalibration::store(uint8_t *blob, size_t cap) const
{
    size_t need = 2 + 4 * (size_t)count;
    if (blob == nullptr || cap < need)
        return 0;
    blob[0] = STW_BLOB_VERSION;
    blob[1] = count;
    for (uint8_t i = 0; i < count; i++)
    {
        uint8_t *b = blob + 2 + 4 * i;
        b[0] = points[i].hz100 & 0xFF;
        b[1] = points[i].hz100 >> 8;
        b[2] = points[i].kn100 & 0xFF;
        b[3] = points[i].kn100 >> 8;
    }
    return need;
}

static bool parse_number(const char *s, double &v, const char *&end)
{
    if (s == nullptr || *s == '\0')
        return false;
    char *e = nullptr;
    double d = strtod(s, &e);
    if (e == s || !isfinite(d) || d < 0.0)
        return false;
    v = d;
    end = e;
    return true;
}

bool parse_stw_hz(const char *s, double &hz)
{
    double v;
    const char *end;
    if (!parse_number(s, v, end) || *end != '\0')
        return false;
    hz = v;
    return true;
}

bool parse_stw_point(const char *s, double &hz, double &kn)
{
    double h, k;
    const char *end;
    if (!parse_number(s, h, end) || *end != ',')
        return false;
    if (!parse_number(end + 1, k, end) || *end != '\0')
        return false;
    hz = h;
    kn = k;
    return true;
}
