// RF433Debug.h

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

#ifndef _RF433DEBUG_H
#define _RF433DEBUG_H

#define dbg(a) \
    { static const char tmp[] PROGMEM = {a}; \
      constexpr short unsigned l = strlen(a); \
      dbgfunc(__FILE__, __LINE__, l, tmp); \
    }

#define dbgf(a, ...) \
    { static const char tmp[] PROGMEM = {a}; \
      constexpr short unsigned l = strlen(a); \
      dbgffunc(__FILE__, __LINE__, l, tmp, __VA_ARGS__); \
    }

void dbgfunc(const char* file, long int line, short unsigned msg_len,
        const char *msg);
void dbgffunc(const char* file, long int line, short unsigned format_len,
        const char *format, ...)
        __attribute__((format(printf, 4, 5)));

#endif // _RF433DEBUG_H

// vim: ts=4:sw=4:tw=80:et
