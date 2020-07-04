#include "PositionFilter.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

PositionFilter::PositionFilter() {
    _rolling_window = (RMC**)malloc(300 * sizeof(RMC*)); // enough space for 30 seconds
    _win_size = 0;
}

PositionFilter::~PositionFilter() {
    for (int i = 0; i<_win_size; i++) {
        if (_rolling_window[i]) free(_rolling_window[i]);
    }
    free(_rolling_window);
}

unsigned long get_time_ms(RMC* rmc) {
    tm _gps_time;
    _gps_time.tm_hour = rmc->h;
    _gps_time.tm_min = rmc->m;
    _gps_time.tm_sec = rmc->s;
    _gps_time.tm_year = rmc->y - 1900;
    _gps_time.tm_mon = rmc->M - 1;
    _gps_time.tm_mday = rmc->d;
    time_t _gps_time_t = mktime(&_gps_time);
    return _gps_time_t * 1000L + rmc->ms;
}

RMC* PositionFilter::sample(RMC* pos) {
    ulong t = get_time_ms(pos);
    
    int deleted = 0;
    if (_win_size && (t - get_time_ms(_rolling_window[0])) > 30000) {
        free(_rolling_window[deleted]);
        _rolling_window[deleted] = NULL;
        deleted++;
    }
    if (deleted) {
        memcpy(_rolling_window, _rolling_window + deleted * sizeof(RMC*), _win_size - deleted);
        _win_size -= deleted;
    }
    RMC* rmc = new RMC;
    memcpy(rmc, pos, sizeof(RMC));
    _rolling_window[_win_size] = rmc;
    _win_size++;



}