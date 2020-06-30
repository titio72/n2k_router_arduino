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
    static int getDaysSince1970(int y, int m, int d);
    static int parseRMC(const char *s_rmc, RMC &rmc);
    static int parseGSA(const char *s_rmc, GSA &gsa);
    static void dumpGSA(GSA &gsa, char *buffer);
    static void dumpRMC(RMC &rmc, char *buffer);
    static bool is_sentence(const char* sentence, const char* sentence_id);
};

#endif