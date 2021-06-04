// Serial.h

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

#ifndef _SERIAL_H
#define _SERIAL_H

#include <Arduino.h>

#define SERIAL_LINE_BUF_LEN 19

class SerialLine {
    private:
        char buf[SERIAL_LINE_BUF_LEN]; // 16-character strings (then CR+LF then
                                       // NULL-terminating).
        size_t head;
        bool got_a_line;
        void reset();

    public:
        SerialLine();

        void do_events();
        bool is_line_available();
        bool get_line(char *s, size_t len);
        void get_line_blocking(char *s, size_t len);
        void split_s_into_func_args(char *s, char **func, char **args) const;
};

#endif // _SERIAL_H

// vim: ts=4:sw=4:tw=80:et
