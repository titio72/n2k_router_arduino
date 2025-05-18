/* 
 * File:   NMEA.h
 * Author: aboni
 */

#ifndef NMEA_H_
#define NMEA_H_

#include "Utils.h"
#include "Data.h"

enum NMEA_RESPONSE
{
    NMEA_OK = 0,
    NMEA_ERROR = -1,
    NMEA_INVALID = -2
};

class NMEAUtils
{
public:
    NMEAUtils();
    ~NMEAUtils();
    static int getDaysSince1970(int y, int m, int d);
    static NMEA_RESPONSE parseRMC(const char *s_rmc, RMC &rmc);
    NMEA_RESPONSE parseGSA(const char *s_rmc, GSA &gsa);
    NMEA_RESPONSE parseGSV(const char* s_ggv, GSV &gsv);
    static void dumpGSA(GSA &gsa, char *buffer);
    static void dumpRMC(RMC &rmc, char *buffer);
    static bool is_sentence(const char* sentence, const char* sentence_id);

    sat* get_satellites();
    uint get_n_satellites() { return n_snapshot_sat; }

    void snap_sats();

private:

    sat snapshots_satellites[32];
    int n_snapshot_sat;

    sat satellites[32];
    int n_sat;

    bool sat_ready;
    unsigned long last_gsv_time;
    unsigned char gsv_count;
    unsigned char gsv_index;
    unsigned char gsv_total;

    void reset_gsv();
};
#endif