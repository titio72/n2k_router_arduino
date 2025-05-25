/*
 * File:   NMEA.cpp
 * Author: aboni
 */

#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <math.h>
#include "NMEA.h"
#include <Log.h>

#define N_GSA 1
//#define N_GSA 2 //used with M8N

#define GPGSV "GSV,"
#define GPRMC "RMC,"
#define GPGSA "GSA,"

char tempBuffer[2048];

GSA *last_gsa = NULL;

bool nmeaChecksumCompare(const char* s)
{
    int l = strlen(s);
    if (l<10 && (s[0] != '$' || s[l - 3] != '*'))
    {
        return false;
    }

    uint8_t calcChecksum = 0;
    for (int i = 1; s[i] != '*'; i++) // skip the $ at the beginning
    {
        calcChecksum ^= s[i];
    }

    uint8_t cs1 = calcChecksum / 16; cs1 += cs1 < 10 ? '0' : 'A' - 10;
    uint8_t cs2 = calcChecksum % 16; cs2 += cs2 < 10 ? '0' : 'A' - 10;
    return cs1 == s[l - 2] && cs2 == s[l - 1];

}

char *get_next_token(const char *string, size_t &start, size_t len, char *buffer, char delim, char delimEnd = 0)
{
    if (start >= len)
        return NULL;
    size_t s = start;
    for (size_t i = s; i < len; i++)
    {
        if (string[i] == delim)
        {
            buffer[i - s] = 0;
            start = i + 1;
            break;
        }
        else if (delimEnd && string[i] == delimEnd)
        {
            buffer[i - s] = 0;
            start = len; // cause the tokenizer to finish
        }
        else
        {
            buffer[i - s] = string[i];
            start++;
        }
    }
    return buffer;
}

const char *get_next_token_checked(const char *string, size_t &start, size_t len, char *buffer, char delim, char delimEnd = 0)
{
    const char *c = get_next_token(string, start, len, buffer, delim, delimEnd);
    if (c)
    {
        int l = strlen(c);
        if (l > 3)
            return NULL;
        else
        {
            for (int i = 0; i < l; i++)
            {
                if (c[i] < '0' || c[i] > '9')
                    return NULL;
            }
        }
    }
    return c;
}

NMEAUtils::NMEAUtils(): last_gsv_time(0), gsv_count(0), gsv_index(0), gsv_total(0), n_sat(0), n_snapshot_sat(0), sat_ready(false)
{}

NMEAUtils::~NMEAUtils()
{
}

NMEA_RESPONSE nmea_position_parse(char *s, double *pos)
{
    if (s == 0 || s[0] == 0)
        return NMEA_ERROR;

    char *cursor;

    int degrees = 0;
    float minutes = 0;

    if (s == NULL || *s == '\0')
    {
        return NMEA_ERROR;
    }

    /* decimal minutes */
    if (NULL == (cursor = strchr(s, '.')))
    {
        return NMEA_ERROR;
    }

    /* minutes starts 2 digits before dot */
    cursor -= 2;
    minutes = atof(cursor);
    *cursor = '\0';

    /* integer degrees */
    cursor = s;
    degrees = atoi(cursor);

    *pos = degrees + (minutes / 60.0);

    return NMEA_OK;
}

bool NMEAUtils::is_sentence(const char *sentence, const char *sentence_id)
{
    if (sentence == NULL || sentence_id == NULL || sentence[0] != '$')
        return false;

    // sentences are in the form "$XXYYY," where YYY is the sentence id - we skip the first 3 characters
    size_t l = strlen(sentence_id);
    for (int i = 0; i < l; i++)
    {
        if (sentence[i + 3]==0 || sentence_id[i] != sentence[i + 3])
            return false;
    }
    return true;
}

NMEA_RESPONSE NMEAUtils::parseGSA(const char *s_gsa, GSA &gsa)
{
    static ulong last_gsa_time = 0;
    static int gsa_count = 0;

    NMEA_RESPONSE ret = NMEA_ERROR;

    if (is_sentence(s_gsa, GPGSA))
    {

        ulong t = _millis();
        if ((t - last_gsa_time) > 100)
        {
            memset(&gsa, 0, sizeof(GSA));
            gsa.nSat = 0;
            gsa_count = 1;
        }
        else
        {
            gsa_count++;
        }
        last_gsa_time = t;

        gsa.valid = 1;

        size_t start = strlen(GPGSA) + 3;
        size_t len = strlen(s_gsa);

        // read mode
        char *token = get_next_token(s_gsa, start, len, tempBuffer, ',');

        // read fix
        token = get_next_token(s_gsa, start, len, tempBuffer, ',');
        if (token && token[0])
            gsa.fix = atoi(token);

        // count sats
        for (int i = 0; i < 12; i++)
        {
            token = get_next_token(s_gsa, start, len, tempBuffer, ',');
            if (token && token[0])
            {
                gsa.sats[gsa.nSat] = atoi(token);
                gsa.nSat++;
            }
        }

        // read PDOP
        token = get_next_token(s_gsa, start, len, tempBuffer, ',');
        if (token && token[0])
            gsa.pdop = atof(token);
        else
            gsa.pdop = 100.0;

        // read HDOP
        token = get_next_token(s_gsa, start, len, tempBuffer, ',');
        if (token && token[0])
            gsa.hdop = atof(token);
        else
            gsa.hdop = 100.0;

        // read VDOP
        token = get_next_token(s_gsa, start, len, tempBuffer, ',');
        if (token && token[0])
            gsa.vdop = atof(token);
        else
            gsa.vdop = 100.0;

        last_gsa = &gsa;

        ret = NMEA_OK;
    }

    if (sat_ready && gsa_count == N_GSA)
    {
        snap_sats();
        n_sat = 0;
        sat_ready = false;
    }
    return ret;
}

void NMEAUtils::dumpRMC(RMC &rmc, char *buffer)
{
    sprintf(buffer, "RMC valid {%d} lat {%.4f} lon {%.4f} cog {%.1f} sog {%.2f} time {%d-%d-%dT%d:%d:%dZ}",
            rmc.valid, rmc.lat, rmc.lon, rmc.cog, rmc.sog, rmc.y, rmc.M, rmc.d, rmc.h, rmc.m, rmc.s);
}

void NMEAUtils::dumpGSA(GSA &gsa, char *buffer)
{
    sprintf(buffer, "RSA valid {%d} nSat {%d} fix {%d} HDOP {%.1f} sats{", gsa.valid, gsa.nSat, gsa.fix, gsa.hdop);
    buffer = buffer + strlen(buffer);
    for (int i = 0; i < gsa.nSat; i++)
    {
        if (i)
            sprintf(buffer, ",%d", gsa.sats[i]);
        else
            sprintf(buffer, "%d", gsa.sats[i]);
        buffer = buffer + strlen(buffer);
    }
    buffer = buffer + strlen(buffer);
    sprintf(buffer, "}");
}

time_t get_time(int y, int M, int d, int h, int m, int s)
{
    struct tm t;
    t.tm_year = y - 1900;
    t.tm_mon = M - 1;
    t.tm_mday = d;
    t.tm_hour = h;
    t.tm_min = m;
    t.tm_sec = s;
    t.tm_isdst = 0;
    return mktime(&t);
}

NMEA_RESPONSE NMEAUtils::parseRMC(const char *s_rmc, RMC &rmc)
{
    if (is_sentence(s_rmc, GPRMC))
    {
        memset(&rmc, 0, sizeof(RMC));

        size_t start = strlen(GPRMC) + 3;
        size_t len = strlen(s_rmc);

        // read UTC time
        char *token = get_next_token(s_rmc, start, len, tempBuffer, ',');
        if (token && strlen(token) >= 6)
        {
            if (strlen(token) > 6)
            {
                token[6] = 0;
                rmc.ms = atoi(token + 7 * sizeof(char));
            }
            rmc.s = atoi(token + 4 * sizeof(char));
            token[4] = 0;
            rmc.m = atoi(token + 2 * sizeof(char));
            token[2] = 0;
            rmc.h = atoi(token);
        }
        //Log::tracex("GPS", "RMC", "time {%d:%d:%d.%d}\n", rmc.h, rmc.m, rmc.s, rmc.ms);
        // read validity
        token = get_next_token(s_rmc, start, len, tempBuffer, ',');

        // read latitude
        token = get_next_token(s_rmc, start, len, tempBuffer, ',');
        rmc.lat = NAN;
        nmea_position_parse(token, &(rmc.lat));
        token = get_next_token(s_rmc, start, len, tempBuffer, ',');
        if (!isnan(rmc.lat) && token && token[0])
        {
            if (token[0] == 'S')
                rmc.lat = -rmc.lat;
            rmc.valid = 1;
        }

        // read longitude
        token = get_next_token(s_rmc, start, len, tempBuffer, ',');
        rmc.lon = NAN;
        nmea_position_parse(token, &(rmc.lon));
        token = get_next_token(s_rmc, start, len, tempBuffer, ',');
        if (!isnan(rmc.lon) && token && token[0])
        {
            if (token[0] == 'W')
                rmc.lon = -rmc.lon;
        }
        else
        {
            rmc.valid = 0;
        }

        // read SOG in knots
        token = get_next_token(s_rmc, start, len, tempBuffer, ',');
        rmc.sog = (token && token[0]) ? atof(token) : NAN;

        // read COG in degrees
        token = get_next_token(s_rmc, start, len, tempBuffer, ',');
        //Log::tracex("GPS", "RMC", "COG {%s}\n", token);
        rmc.cog = (token && token[0]) ? atof(token) : NAN;

        // read date
        token = get_next_token(s_rmc, start, len, tempBuffer, ',');
        if (token && token[0])
        {
            rmc.y = atoi(token + 4 * sizeof(char)) + 2000;
            token[4] = 0;
            rmc.M = atoi(token + 2 * sizeof(char));
            token[2] = 0;
            rmc.d = atoi(token);
        }

        if (rmc.y > 0)
        {
            rmc.unix_time = get_time(rmc.y, rmc.M, rmc.d, rmc.h, rmc.m, rmc.s);
        }
        return NMEA_OK;
    }
    return NMEA_ERROR;
}

enum SAT_PARSE
{
    PARSE_ERROR,
    PARSE_OK,
    PARSE_EMPTY
};

SAT_PARSE sat_parse(const char* s_gsv, size_t &start, size_t len, sat& satellite)
{
    const char *s_sat_id = get_next_token_checked(s_gsv, start, len, tempBuffer, ',');

    if (s_sat_id && s_sat_id[0])
    {

        int sat_id = atoi(s_sat_id);

        const char *s = get_next_token_checked(s_gsv, start, len, tempBuffer, ',');
        if (s == NULL)
            return PARSE_ERROR; // the sentence is corrupted
        int sat_el = atoi(s);

        s = get_next_token_checked(s_gsv, start, len, tempBuffer, ',');
        if (s == NULL)
            return PARSE_ERROR; // the sentence is corrupted
        int sat_az = atoi(s);

        int sat_db = -1;
        s = get_next_token_checked(s_gsv, start, len, tempBuffer, ',', '*');
        if (s == NULL)
            return PARSE_ERROR; // the sentence is corrupted
        else if (s[0])
            sat_db = atoi(s);

        satellite.sat_id = sat_id;
        satellite.az = sat_az;
        satellite.elev = sat_el;
        satellite.db = sat_db;
        return PARSE_OK;
    }
    else
    {
        // skip to the next sat
        get_next_token_checked(s_gsv, start, len, tempBuffer, ',');
        get_next_token_checked(s_gsv, start, len, tempBuffer, ',');
        get_next_token_checked(s_gsv, start, len, tempBuffer, ',', '*');
        return PARSE_EMPTY;
    }
}

/*
$GPGSV,2,1,07,07,79,048,42,02,51,062,43,26,36,256,42,27,27,138,42*71
$GPGSV,2,2,07,09,23,313,42,04,19,159,41,15,12,041,42*41
*/

void NMEAUtils::reset_gsv()
{
    sat_ready = false;
    n_sat = 0;
    gsv_count = 0;
    gsv_index = 0;
    gsv_total = 0;
}

NMEA_RESPONSE parse_gsv_sats(const char *s_gsv, size_t &start, size_t len, sat *satellites, int &n_sat)
{
    SAT_PARSE res = PARSE_OK;
    for (int i = 0; i<4 && res!=PARSE_ERROR; i++) // 4 is the max N of stas per sentence
    {
        res = sat_parse(s_gsv, start, len, satellites[n_sat]);
        switch (res)
        {
            case PARSE_OK:
            {
                n_sat++;
            }
            break;
            case PARSE_EMPTY:
            {

            }
            break;
            default: return NMEA_ERROR;
        }
    }
    return NMEA_OK;
}

NMEA_RESPONSE NMEAUtils::parseGSV(const char *s_gsv, GSV &gsv)
{
    //Log::trace("%s\n", s_gsv);
    if (is_sentence(s_gsv, GPGSV))
    {
        size_t start = strlen(GPGSV) + 3;
        size_t len = strlen(s_gsv);

        // expected number of sentences
        int n = atoi(get_next_token(s_gsv, start, len, tempBuffer, ','));
        // sentence counter (out of n)
        int c = atoi(get_next_token(s_gsv, start, len, tempBuffer, ','));
        // expected number of satellites in this block of sentences
        int x = atoi(get_next_token(s_gsv, start, len, tempBuffer, ','));

        //Log::trace("GSV: %d of %d, %d satellites\n", c, n, x);
        if (c == 1)
        {
            // first sentence of the series - resetting the state
            reset_gsv();
            n_sat = 0;
            gsv_count = n;
            gsv_index = 1;
            gsv_total = x;
        }
        else if (n == gsv_count && c == (gsv_index + 1))
        {
            // received the next sentence in the series
            gsv_index++;
            if (gsv_index > n)
            {
                // the index exeeds the expected number of sentences - definitely an error
                reset_gsv();
                return NMEA_ERROR;
            }
        }
        else
        {
            // the sentence is out of order or the number of sentences is wrong
            reset_gsv();
            return NMEA_ERROR;
        }

        if (parse_gsv_sats(s_gsv, start, len, satellites, n_sat)==NMEA_ERROR)
        {
            // there was an error parsing the sattelites
            return NMEA_ERROR;
        }

        if (c == n)
        {
            //Log::trace("GSV: %d satellites\n", n_sat);
            sat_ready = true;
            gsv.nSat = n_sat;
            memcpy(satellites, gsv.satellites, sizeof(satellites));
        }
        return NMEA_OK;
    }
    //Log::trace("GSV: invalid sentence {%s}\n", s_gsv); 
    return NMEA_ERROR;
}

void NMEAUtils::snap_sats()
{
    n_snapshot_sat = n_sat;
    memcpy(&snapshots_satellites, &satellites, sizeof(snapshots_satellites));
}

sat *NMEAUtils::get_satellites()
{
    return snapshots_satellites;
}