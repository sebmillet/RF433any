// 04_react_on_code.ino

// Example sketch that comes along with RF433any library
// Shows how to trigger different actions depending on code received.

/*
  Copyright 2021 Sébastien Millet

  `RF433any' is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  `RF433any' is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program. If not, see
  <https://www.gnu.org/licenses>.
*/

//
// Schematic: Radio Frequencies RECEIVER plugged on D2
//

#include "RF433any.h"
#include <Arduino.h>

#define PIN_RFINPUT  2

#define ASSERT_OUTPUT_TO_SERIAL

#define assert(cond) { \
    if (!(cond)) { \
        assert_failed(__LINE__); \
    } \
}

static void assert_failed(int line) {
#ifdef ASSERT_OUTPUT_TO_SERIAL
    Serial.print("\ntest.ino:");
    Serial.print(line);
    Serial.println(": assertion failed, aborted.");
#endif
    while (1)
        ;
}

char serial_printf_buffer[150];
void serial_printf(const char* msg, ...)
     __attribute__((format(printf, 1, 2)));

    // NOTE
    //   Assume Serial has been initialized (Serial.begin(...))
void serial_printf(const char* msg, ...) {
    va_list args;

    va_start(args, msg);

    vsnprintf(serial_printf_buffer, sizeof(serial_printf_buffer), msg, args);
    va_end(args);
    Serial.print(serial_printf_buffer);
}

void setup() {
    pinMode(PIN_RFINPUT, INPUT);
    Serial.begin(115200);
}

Track track(PIN_RFINPUT);

void loop() {
    const BitVector *code1 = new BitVector(32, 4, 0xb9, 0x35, 0x6d, 0x00);

    track.treset();

    while (!track.do_events())
        delay(1);
    track.wait_free_433();

    Decoder *pdec0 = track.get_data(
        RF433ANY_FD_DECODED | RF433ANY_FD_DEDUP | RF433ANY_FD_NO_ERROR
    );
    Decoder *pdec = pdec0;
    while (pdec) {
        const BitVector *pdata = pdec->get_pdata();
        assert(pdata); // Must be the case (RF433ANY_FD_DECODED in the call to
                       // get_data() above).
        char *buf = pdata->to_str();
        assert(buf);
        serial_printf("Received %c(%d): %s\n", pdec->get_id_letter(),
                pdata->get_nb_bits(), buf);
        free(buf);

        if (!pdata->cmp(code1)) {
            serial_printf("GOT IT!\n");
        }

        pdec = pdec->get_next();
    }
    delete pdec0;

    delete code1;
}

// vim: ts=4:sw=4:tw=80:et
