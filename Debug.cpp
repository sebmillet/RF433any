// Debug.cpp

/*
  Provides some useful functions to output debug from Arduino on the serial
  line.
  Used and tested with an Arduino nano.
*/

/*
  Copyright 2021 Sébastien Millet

  `rf433any' is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  `rf433any' is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program. If not, see
  <https://www.gnu.org/licenses>.
*/

#include "Debug.h"
#include <Arduino.h>
#include <stdarg.h>

// What items to include in the debug lines:

static char buffer[150];
static char progmem_reading_buffer[100];

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

void dbgfunc(const char* file, long int line, const char* progmem_str) {
    strcpy_P(progmem_reading_buffer, progmem_str);
    Serial.print(progmem_reading_buffer);
    Serial.print("\n");
}

void dbgffunc(const char* file, long int line, const char* progmem_fmt, ...) {
    strcpy_P(progmem_reading_buffer, progmem_fmt);
    va_list args;

    // FIXME
#pragma GCC diagnostic ignored "-Wvarargs"
    va_start(args, progmem_reading_buffer);

    vsnprintf(buffer, sizeof(buffer), progmem_reading_buffer, args);
    va_end(args);
    Serial.print(buffer);
    Serial.print("\n");
}

// vim: ts=4:sw=4:tw=80:et
