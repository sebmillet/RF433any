// output-signal-timings.ino

// Example sketch that comes along with RF433any library

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
// Schematic: see RF433any library
//

#include "RF433any.h"
#include <Arduino.h>

#define PIN_RFINPUT  2

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
    Serial.print("Waiting for signal\n");
}

Track track(PIN_RFINPUT);

void output_timings(Decoder *pdec) {
    TimingsExt tsext;
    if (!pdec)
        return;
    pdec->get_tsext(&tsext);
    serial_printf("  I=%u, LS=%u, LL=%u, HS=%u, HL=%u, S=%u, U=%u, V=%u, "
            "Y=%u, Z=%u\n", tsext.initseq, tsext.low_short, tsext.low_long,
            tsext.high_short, tsext.high_long, tsext.sep, tsext.first_low,
            tsext.first_high, tsext.first_low_ignored, tsext.last_low);
}

void loop() {
    track.treset();

    while (!track.do_events())
        delay(1);

    Decoder *pdec0 = track.get_data(RF433ANY_FD_DEDUP);
    Decoder *pdec = pdec0;
    while(pdec) {
        BitVector *pdata = pdec->take_away_data();

        serial_printf("Decoded: %s, err: %d, code: %c, "
                "rep: %d, bits: %2d",
                (pdec->data_got_decoded() ? "yes" : "no "),
                pdec->get_nb_errors(), pdec->get_id_letter(),
                pdec->get_repeats() + 1, pdec->get_nb_bits());

        if (pdec->data_got_decoded()) {
            Serial.print(", data: ");
            if (pdata) {
                char *buf = pdata->to_str();
                if (buf) {
                    Serial.print(buf);
                    free(buf);
                }
                delete pdata;
            }
//            output_timings(pdec);
        }
        Serial.print("\n");
        pdec = pdec->get_next();
    }
    delete pdec0;
}

// vim: ts=4:sw=4:tw=80:et
