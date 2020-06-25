#include <Arduino.h>
#include "Utils.h"

#define TRACE
#define MAX_TRACE_SIZE 256

void debug_println(const char *s) {
  #ifdef TRACE
  Serial.println(s);
  #endif
}

void debug_print(const char *fmt, ...) {
  #ifdef TRACE
  static char buffer[MAX_TRACE_SIZE];    
  va_list argptr;
  va_start(argptr, fmt);
  vsnprintf(buffer, MAX_TRACE_SIZE, fmt, argptr);
  va_end(argptr);
  Serial.println(buffer);
  #endif
}

