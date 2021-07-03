// 02_output-signal-timings.ino

// Example sketch that comes along with RF433any library
// Displays all signals details (incl. non-coding sequences) along with their
// timings

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

const char welcome[] PROGMEM =
    "Waiting for signal\n"
    "  Durations are in microseconds\n"
    "  Data is output in hexdecimal\n"
    "  When high durations are equal to low, they are output as zero\n"
    "  Codes: (I)nconsistent, (S)ync, (U)nknown, (T)ribit, "
        "tribit i(N)verted, (M)anchester\n"
    "  I:  Initseq = 'high' radio signal sent once\n"
    "  LS: Low Short duration\n"
    "  LL: Low Long duration\n"
    "  HS: High Short duration\n"
    "  HL: High Long duration\n"
    "  S: Separator (typically separating repeats)\n"
    "  U: Low signal prefix (zero most frequently)\n"
    "  V: High signal prefix (zero most frequently)\n"
    "  Y: First low non-coding signal "
        "(non-zero only if encoding is tribit inverted)\n"
    "  Z: Last low signal\n";

void setup() {
    pinMode(PIN_RFINPUT, INPUT);
    Serial.begin(115200);
    char *buf = (char *)malloc(sizeof(welcome));
    strcpy_P(buf, welcome);
    Serial.print(buf);
    free(buf);
}

Track track(PIN_RFINPUT);

void output_timings(Decoder *pdec) {
    TimingsExt tsext;
    if (!pdec)
        return;
    pdec->get_tsext(&tsext);
    serial_printf("         I=%u, LS=%u, LL=%u, HS=%u, HL=%u, S=%u, U=%u, "
            "V=%u, Y=%u, Z=%u\n", tsext.initseq, tsext.low_short,
            tsext.low_long, tsext.high_short, tsext.high_long, tsext.sep,
            tsext.first_low, tsext.first_high, tsext.first_low_ignored,
            tsext.last_low);
}

void loop() {
    track.treset();

    while (!track.do_events())
        delay(1);

    Decoder *pdec0 = track.get_data(RF433ANY_FD_ALL);
    Decoder *pdec = pdec0;
    while (pdec) {
        int nb_bits = pdec->get_nb_bits();
        bool got_data = pdec->data_got_decoded();
        BitVector *pdata = pdec->take_away_data();

        serial_printf("Decoded: %s, err: %d, code: %c, "
                "rep: %d, bits: %2d", (got_data ? "yes" : "no "),
                pdec->get_nb_errors(), pdec->get_id_letter(),
                pdec->get_repeats() + 1, nb_bits);

        if (pdata) {
            Serial.print(", data: ");
            char *buf = pdata->to_str();
            if (buf) {
                Serial.print(buf);
                free(buf);
            }
            delete pdata;
        }
        Serial.print("\n");
        output_timings(pdec);
        pdec = pdec->get_next();
    }
    delete pdec0;
}

// vim: ts=4:sw=4:tw=80:et
