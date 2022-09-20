// RF433Debug.cpp

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

#include "RF433Debug.h"
#include <Arduino.h>
#include <stdarg.h>

static char buffer[120];
static char progmem_reading_buffer[91];

const char *newline = "\n";

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

static fatal(const char* file, long int line, short unsigned len) {
    Serial.print(newline);
    Serial.print(file);
    Serial.print(newline);
    Serial.print(line);
    Serial.print(newline);
    Serial.print(len);
    Serial.print(newline);
    Serial.flush();
    while (1)
        ;
}

void dbgfunc(const char* file, long int line, short unsigned progmem_str_len,
        const char* progmem_str) {
    if (progmem_str_len >= sizeof(progmem_reading_buffer)) {
        fatal(file, line, progmem_str_len);
    }
    strcpy_P(progmem_reading_buffer, progmem_str);
    Serial.print(progmem_reading_buffer);
    Serial.print(newline);
      // Not useful normally (sometimes, makes analysis easier in bug fix)
//    Serial.flush();
}

void dbgffunc(const char* file, long int line, short unsigned progmem_fmt_len,
        const char* progmem_fmt, ...) {
    if (progmem_fmt_len >= sizeof(progmem_reading_buffer)) {
        fatal(file, line, progmem_fmt_len);
    }
    strcpy_P(progmem_reading_buffer, progmem_fmt);
    va_list args;

    // FIXME
#pragma GCC diagnostic ignored "-Wvarargs"
    va_start(args, progmem_reading_buffer);

    vsnprintf(buffer, sizeof(buffer), progmem_reading_buffer, args);
    va_end(args);
    Serial.print(buffer);
    Serial.print(newline);
      // Not useful normally (sometimes, makes analysis easier in bug fix)
//    Serial.flush();
}

// vim: ts=4:sw=4:tw=80:et
