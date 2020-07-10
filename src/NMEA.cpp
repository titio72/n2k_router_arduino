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

#define N_GSA 2

NMEAUtils::NMEAUtils() {
    sat_ready = false;

    n_sat = 0;
    sat_size = 0;
    satellites = NULL;

    n_snapshot_sat = 0;
    snapshot_sat_size = 0;
    snapshots_satellites = NULL;
}

NMEAUtils::~NMEAUtils() {
    if (satellites) free(satellites);
    if (snapshots_satellites) free(snapshots_satellites);
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

GSA* last_gsa = NULL;

int NMEAUtils::parseGSA(const char *s_gsa, GSA &gsa)
{
    static ulong last_gsa_time = 0;
    static int gsa_count = 0;

    int ret = -1;
    char *tofree = strdup(s_gsa);
    char *tempstr = tofree;
    char *token = strsep(&tempstr, ",");
    if (strlen(token) && strcmp(token + sizeof(char) * 3, "GSA") == 0)
    {
        ulong t = millis();
        if ((t-last_gsa_time)>250) {
            memset(&gsa, 0, sizeof(GSA));
            gsa.nSat = 0;
            gsa_count = 1;
        } else {
            gsa_count++;
        }
        last_gsa_time = t;

        gsa.valid = 1;

        // read mode
        token = strsep(&tempstr, ",");

        // read fix
        token = strsep(&tempstr, ",");
        if (token && token[0]) gsa.fix = atoi(token);

        // count sats
        for (int i = 0; i < 12; i++)
        {
            token = strsep(&tempstr, ",");
            if (token && token[0]) {
                gsa.sats[i] = atoi(token);
                gsa.nSat++;
            }
        }

        // read PDOP
        token = strsep(&tempstr, ",");
        if (token && token[0]) gsa.pdop = atof(token); else gsa.pdop = 100.0;

        // read HDOP
        token = strsep(&tempstr, ",");
        if (token && token[0]) gsa.hdop = atof(token); else gsa.hdop = 100.0;

        // read VDOP
        token = strsep(&tempstr, ",");
        if (token && token[0]) gsa.vdop = atof(token); else gsa.vdop = 100.0;

        last_gsa = &gsa;

        ret = 0;
    }
    free(tofree);
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
    char *tofree = strdup(s_rmc);
    char *tempstr = tofree;
    char *token = strsep(&tempstr, ",");
    if (strlen(token) && strcmp(token + sizeof(char) * 3, "RMC") == 0)
    {
        memset(&rmc, 0, sizeof(RMC));

        // read UTC time
        token = strsep(&tempstr, ",");
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
        token = strsep(&tempstr, ",");

        // read latitude
        token = strsep(&tempstr, ",");
        rmc.lat = NAN;
        nmea_position_parse(token, &(rmc.lat));
        token = strsep(&tempstr, ",");
        if (!isnan(rmc.lat) && token && token[0])
        {
            if (token[0] == 'S')
                rmc.lat = -rmc.lat;
            rmc.valid = 1;
        }

        // read longitude
        token = strsep(&tempstr, ",");
        rmc.lon = NAN;
        nmea_position_parse(token, &(rmc.lon));
        token = strsep(&tempstr, ",");
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
        token = strsep(&tempstr, ",");
        rmc.sog = (token && token[0]) ? atof(token) : NAN;

        // read COG in degrees
        token = strsep(&tempstr, ",");
        rmc.cog = (token && token[0]) ? atof(token) : NAN;

        // read date
        token = strsep(&tempstr, ",");
        if (token && token[0]) {
            rmc.y = atoi(token + 4 * sizeof(char)) + 2000;
            token[4] = 0;
            rmc.M = atoi(token + 2 * sizeof(char));
            token[2] = 0;
            rmc.d = atoi(token);
        }
        free(tofree);
        return 0;
    }
    free(tofree);
    return -1;
}

/*
$GPGSV,3,1,10,
02,39,271,30,
04,21,072,,
05,28,308,25,
06,37,218,
*7D
$GPGSV,3,2,10,
07,73,148,,
09,57,059,,
13,03,258,,
16,13,045,
*7A
$GPGSV,3,3,10,
29,01,324,,
30,46,198,
*76

$GLGSV,3,1,10,68,37,085,,69,63,359,,70,17,301,24,77,01,020,*62
$GLGSV,3,2,10,78,43,052,,79,51,138,,80,09,182,,84,19,244,*66
$GLGSV,3,3,10,85,24,304,,86,04,352,35*60
*/


sat* sats_parse(const char* gsv, int &sat_size, int &n_sat, sat* satellites, bool &sat_ready) {
    char *tofree = strdup(gsv);
    char *tempstr = tofree;
    char *token = strsep(&tempstr, ",");
    if (strlen(token) && strcmp(token + sizeof(char) * 3, "GSV") == 0)
    {
        token = strsep(&tempstr, ","); int n = atoi(token);       
        token = strsep(&tempstr, ","); int c = atoi(token);

        // expected number of stats in this block of sentences
        token = strsep(&tempstr, ","); int x = atoi(token);

        if (sat_size==0) {
            n_sat = 0;
            satellites = (sat*)malloc(x*sizeof(sat));
            sat_size = x;
        } else if (n_sat==sat_size) {
            sat_size += x;
            satellites = (sat*)realloc(satellites, sat_size*sizeof(sat));
        }

        bool stop = false;
        while (!stop) {
            token = strsep(&tempstr, ","); 
            if (token) {
                int sat_id = atoi(token);
                if (sat_id>0) {
                    satellites[n_sat].sat_id = sat_id;

                    token = strsep(&tempstr, ",");
                    satellites[n_sat].elev = atoi(token);
                    
                    token = strsep(&tempstr, ",");
                    satellites[n_sat].az = atoi(token);
                    
                    token = strsep(&tempstr, ",");
                    if (token) {
                        char* checksum = strstr(token, "*");
                        if (checksum) {
                            checksum[0] = 0;
                            stop = true;
                        }
                    }
                    satellites[n_sat].db = atoi(token);
                    
                    if (last_gsa)
                        satellites[n_sat].status = array_contains(satellites[n_sat].sat_id, last_gsa->sats, last_gsa->nSat)?2:0x0F;
                    else 
                        satellites[n_sat].status = 0x0F;

                    n_sat++;
                } else {
                    strsep(&tempstr, ","); // skip elevation
                    strsep(&tempstr, ","); // skip azimuth
                    token = strsep(&tempstr, ","); // skip db
                    stop = strstr(token, "*");
                }
            } else {
                stop = true;
            }
        }
        sat_ready = (n==c);
    }
    free(tofree);
    return satellites;
} 

void NMEAUtils::snap_sats() {
    if (snapshot_sat_size<sat_size) {
        snapshot_sat_size = sat_size;
        snapshots_satellites = (sat*)realloc(snapshots_satellites, snapshot_sat_size*sizeof(sat));
    }
    n_snapshot_sat = n_sat;
    if (n_sat) {
        memcpy(snapshots_satellites, satellites, n_sat*sizeof(sat));
    }
}

sat* NMEAUtils::get_satellites() {
    return snapshots_satellites;
}

int NMEAUtils::parseGSV(const char* s_gsv) {
    unsigned long t = millis();
    if ((t - last_gsv_time) > 800 /*ms*/) {
        n_sat = 0;
        sat_ready = false;
    }
    last_gsv_time = t;
    satellites = sats_parse(s_gsv, sat_size, n_sat, satellites, sat_ready);
    return 0;
}