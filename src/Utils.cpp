#include "constants.h"
#include "Utils.h"
#include "errno.h"
#include <time.h>
#include <math.h>
#include <string.h>

ulong _millis(void)
{
  #ifndef ESP32_ARCH
  long            ms; // Milliseconds
  time_t          s;  // Seconds
  struct timespec spec;

  clock_gettime(CLOCK_REALTIME, &spec);

  s  = spec.tv_sec;
  ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
  if (ms > 999) {
      s++;
      ms = 0;
  }

  return s * 1000 + ms;
  #else
  return millis();
  #endif
}

int msleep(long msec)
{
  #ifndef ESP32_ARCH
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
    #else
    delay(msec);
    return 0;
    #endif
}

char * replace(char const * const original, char const * const pattern, char const * const replacement, bool first) {
  size_t const replen = strlen(replacement);
  size_t const patlen = strlen(pattern);
  size_t const orilen = strlen(original);

  size_t patcnt = 0;
  const char * oriptr;
  const char * patloc;

  // find how many times the pattern occurs in the original string
  if (first) {
    patcnt += strstr(original, pattern)?1:0;
  } else {
    for (oriptr = original; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen) {
      patcnt++;
    }
  }

  // allocate memory for the new string
  size_t const retlen = orilen + patcnt * (replen - patlen);
  char * const returned = (char *) malloc( sizeof(char) * (retlen + 1) );

  if (returned != NULL)
  {
    // copy the original string, 
    // replacing all the instances of the pattern
    char * retptr = returned;
    for (oriptr = original; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen)
    {
      size_t const skplen = patloc - oriptr;
      // copy the section until the occurence of the pattern
      strncpy(retptr, oriptr, skplen);
      retptr += skplen;
      // copy the replacement 
      strncpy(retptr, replacement, replen);
      retptr += replen;
    }
    // copy the rest of the string.
    strcpy(retptr, oriptr);

    return returned;
  } else {
    return NULL;
  }
}

// To store number of days in all months from January to Dec.
const int monthDays[12] = {31, 28, 31, 30, 31, 30,
                           31, 31, 30, 31, 30, 31};

int countLeapYears(int year, int month)
{
    // Check if the current year needs to be considered
    // for the count of leap years or not
    if (month <= 2)
        year--;

    // An year is a leap year if it is a multiple of 4,
    // multiple of 400 and not a multiple of 100.
    return year / 4 - year / 100 + year / 400;
}

int getDaysSince1970(int y, int m, int d) {
    // COUNT TOTAL NUMBER OF DAYS BEFORE FIRST DATE 'dt1'

    // initialize count using years and day
    long int n1 = 1970 * 365 + 1;

    // Since every leap year is of 366 days,
    // Add a day for every leap year
    n1 += countLeapYears(1970, 1);

    // SIMILARLY, COUNT TOTAL NUMBER OF DAYS BEFORE 'dt2'

    long int n2 = y * 365 + d;
    for (int i = 0; i < m - 1; i++)
        n2 += monthDays[i];
    n2 += countLeapYears(y, m);

    // return difference between two counts
    return (n2 - n1);
}

bool array_contains(int test, int* int_set, int sz) {
    for (int i=0; i<sz; i++) {
        if (test==int_set[i]) return true;
    }
    return false;
}