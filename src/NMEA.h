/* 
 * File:   NMEA.h
 * Author: aboni
 */

#ifndef NMEA_H_
#define NMEA_H_

#include "Utils.h"

class NMEAUtils
{
public:
    NMEAUtils();
    ~NMEAUtils();
    static int getDaysSince1970(int y, int m, int d);
    static int parseRMC(const char *s_rmc, RMC &rmc);
    int parseGSA(const char *s_rmc, GSA &gsa);
    int parseGSV(const char* s_ggv);
    static void dumpGSA(GSA &gsa, char *buffer);
    static void dumpRMC(RMC &rmc, char *buffer);
    static bool is_sentence(const char* sentence, const char* sentence_id);

    sat* get_satellites();
    uint get_n_satellites() { return n_snapshot_sat; }

private:
    void snap_sats();

    sat* snapshots_satellites;
    int n_snapshot_sat;
    int snapshot_sat_size;

    sat* satellites;
    int n_sat;
    int sat_size;
    bool sat_ready;
    unsigned long last_gsv_time;
};

#endif