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
#include "Log.h"

#define N_GSA 1

#define GPGSV "$GPGSV,"
#define GPRMC "$GPRMC,"
#define GPGSA "$GPGSA,"

char tempBuffer[2048];

GSA* last_gsa = NULL;

char* get_next_token(const char* string, size_t &start, size_t len, char* buffer, char delim, char delimEnd = 0) {
    if (start>=len) return NULL;
    size_t s = start;
    for (size_t i = s; i<len; i++) {
        if (string[i]==delim) {
            buffer[i-s] = 0;
            start = i + 1;
            break;
        } else if (delimEnd && string[i]==delimEnd) {
            buffer[i-s] = 0;
            start = len; // cause the tokenizer to finish
        } else {
            buffer[i-s] = string[i];
            start++;
        }
    }
    return buffer;
} 

const char* get_next_token_checked(const char* string, size_t &start, size_t len, char* buffer, char delim, char delimEnd = 0) {
    const char* c = get_next_token(string, start, len, buffer, delim, delimEnd);
    if (c) {
        int l = strlen(c); 
        if (l>3) return NULL;
        else {
            for (int i = 0; i<l; i++) {
                if (c[i]<'0' || c[i]>'9') return NULL;
            }
        }
    }
    return c;
} 

NMEAUtils::NMEAUtils() {
    sat_ready = false;

    n_sat = 0;

    n_snapshot_sat = 0;
}

NMEAUtils::~NMEAUtils() {
}

int nmea_position_parse(char *s, float *pos)
{
    if (s==0 || s[0]==0) return -1;

    char *cursor;

    int degrees = 0;
    float minutes = 0;

    if (s == NULL || *s == '\0')
    {
        return -1;
    }

    /* decimal minutes */
    if (NULL == (cursor = strchr(s, '.')))
    {
        return -1;
    }

    /* minutes starts 2 digits before dot */
    cursor -= 2;
    minutes = atof(cursor);
    *cursor = '\0';

    /* integer degrees */
    cursor = s;
    degrees = atoi(cursor);

    *pos = degrees + (minutes / 60.0);

    return 0;
}

bool NMEAUtils::is_sentence(const char* sentence, const char* sentence_id) {
    static char id[4];
    for (int i = 3; i < 6; i++) {
        id[i - 3] = sentence[i];
    }
    id[3] = 0;

    return (strcmp(id, sentence_id) == 0);
}

int NMEAUtils::parseGSA(const char *s_gsa, GSA &gsa)
{
    static ulong last_gsa_time = 0;
    static int gsa_count = 0;

    int ret = -1;

    if (s_gsa && strncmp(s_gsa, GPGSA, strlen(GPGSA))==0) {

        ulong t = _millis();
        if ((t-last_gsa_time)>100) {
            memset(&gsa, 0, sizeof(GSA));
            gsa.nSat = 0;
            gsa_count = 1;
        } else {
            gsa_count++;
        }
        last_gsa_time = t;

        gsa.valid = 1;

        size_t start = strlen(GPGSA);
        size_t len = strlen(s_gsa);
        
        // read mode
        char* token = get_next_token(s_gsa, start, len, tempBuffer, ',');

        // read fix
        token = get_next_token(s_gsa, start, len, tempBuffer, ',');
        if (token && token[0]) gsa.fix = atoi(token);

        // count sats
        for (int i = 0; i < 12; i++)
        {
            token = get_next_token(s_gsa, start, len, tempBuffer, ',');
            if (token && token[0]) {
                gsa.sats[i] = atoi(token);
                gsa.nSat++;
            }
        }

        // read PDOP
        token = get_next_token(s_gsa, start, len, tempBuffer, ',');
        if (token && token[0]) gsa.pdop = atof(token); else gsa.pdop = 100.0;

        // read HDOP
        token = get_next_token(s_gsa, start, len, tempBuffer, ',');
        if (token && token[0]) gsa.hdop = atof(token); else gsa.hdop = 100.0;

        // read VDOP
        token = get_next_token(s_gsa, start, len, tempBuffer, ',');
        if (token && token[0]) gsa.vdop = atof(token); else gsa.vdop = 100.0;

        last_gsa = &gsa;

        ret = 0;
    }
    
    if (sat_ready && gsa_count==N_GSA) {
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
    sprintf(buffer, "RSA valid {%d} nSat {%d} fix {%d} HDOP {%.1f}", gsa.valid, gsa.nSat, gsa.fix, gsa.hdop);
}

int NMEAUtils::parseRMC(const char *s_rmc, RMC &rmc)
{
    if (s_rmc && strncmp(s_rmc, GPRMC, strlen(GPRMC)) == 0)
    {
        memset(&rmc, 0, sizeof(RMC));

        size_t start = strlen(GPRMC);
        size_t len = strlen(s_rmc);

        // read UTC time
        char* token = get_next_token(s_rmc, start, len, tempBuffer, ',');
        if (token && strlen(token)>=6) {
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
        rmc.cog = (token && token[0]) ? atof(token) : NAN;

        // read date
        token = get_next_token(s_rmc, start, len, tempBuffer, ',');
        if (token && token[0]) {
            rmc.y = atoi(token + 4 * sizeof(char)) + 2000;
            token[4] = 0;
            rmc.M = atoi(token + 2 * sizeof(char));
            token[2] = 0;
            rmc.d = atoi(token);
        }
        return 0;
    }
    return -1;
}

int sats_parse(const char* s_gsv, int &n_sat, sat* satellites, bool &sat_ready) {
    if (s_gsv && strncmp(s_gsv, GPGSV, strlen(GPGSV))==0) {
        size_t start = strlen(GPGSV);
        size_t len = strlen(s_gsv);

        // expected number of sentences
        int n = atoi(get_next_token(s_gsv, start, len, tempBuffer, ','));
        // sentence counter (out of n)
        int c = atoi(get_next_token(s_gsv, start, len, tempBuffer, ','));
        // expected number of satellites in this block of sentences
        /*int x = */atoi(get_next_token(s_gsv, start, len, tempBuffer, ','));

        while (start<len) 
        {
            const char* s_sat_id = get_next_token_checked(s_gsv, start, len, tempBuffer, ',');
            
            if (s_sat_id && s_sat_id[0]) {
                int sat_id = atoi(s_sat_id);

                const char* s = get_next_token_checked(s_gsv, start, len, tempBuffer, ',');
                if (s==NULL) return -1; // the sentence is corrupted
                int sat_el = atoi(s);

                s = get_next_token_checked(s_gsv, start, len, tempBuffer, ',');
                if (s==NULL) return -1; // the sentence is corrupted
                int sat_az = atoi(s);

                s = get_next_token_checked(s_gsv, start, len, tempBuffer, ',', '*');
                if (s==NULL) return -1; // the sentence is corrupted
                int sat_db = atoi(s);

                if (n_sat<12) {

                    satellites[n_sat].sat_id = sat_id;
                    satellites[n_sat].az = sat_az;
                    satellites[n_sat].elev = sat_el;
                    satellites[n_sat].db = sat_db;
                    
                    if (last_gsa)
                        satellites[n_sat].status = array_contains(satellites[n_sat].sat_id, last_gsa->sats, last_gsa->nSat)?2:0x0F;
                    else 
                        satellites[n_sat].status = 0x0F;
                }
                n_sat++;
                

            } else {
                // skip to the next sat
                get_next_token_checked(s_gsv, start, len, tempBuffer, ',');
                get_next_token_checked(s_gsv, start, len, tempBuffer, ',');
                get_next_token_checked(s_gsv, start, len, tempBuffer, ',', '*');
            }
        }
        
        sat_ready = (n==c);
        
        return 0;
    }
    return -1;
} 

void NMEAUtils::snap_sats() {
    n_snapshot_sat = n_sat;
    memcpy(&snapshots_satellites, &satellites, sizeof(snapshots_satellites));
}

sat* NMEAUtils::get_satellites() {
    return snapshots_satellites;
}

int NMEAUtils::parseGSV(const char* s_gsv) {
    unsigned long t = _millis();
    if ((t - last_gsv_time) > 200 /*ms*/) {
        n_sat = 0;
        sat_ready = false;
    }
    last_gsv_time = t;
    return sats_parse(s_gsv, n_sat, satellites, sat_ready);
}
