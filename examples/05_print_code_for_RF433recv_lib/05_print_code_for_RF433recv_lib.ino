// 05_print_code_for_RF433recv_lib.ino

// Example sketch that comes along with RF433any library
// Displays the signal shape in a way that is ready to use with the library
// RF433recv.

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

char serial_printf_buffer[100];
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
    Serial.print(F("Waiting for signal\n"));
}

Track track(PIN_RFINPUT);

const char *encoding_names[] = {
    "RFMOD_TRIBIT",
    "RFMOD_TRIBIT_INVERTED",
    "RFMOD_MANCHESTER",
    "<unmanaged encoding>"
};

void output_timings(Decoder *pdec, byte nb_bits) {
    TimingsExt tsext;
    if (!pdec)
        return;
    pdec->get_tsext(&tsext);

    if (!tsext.initseq)
        return;

//    serial_printf("         I=%u, LS=%u, LL=%u, HS=%u, HL=%u, S=%u, U=%u, "
//            "V=%u, Y=%u, Z=%u\n", tsext.initseq, tsext.low_short,
//            tsext.low_long, tsext.high_short, tsext.high_long, tsext.sep,
//            tsext.first_low, tsext.first_high, tsext.first_low_ignored,
//            tsext.last_low);

    const char *enc;
    if (pdec->get_id_letter() == 'T')
        enc = encoding_names[0];
    else if (pdec->get_id_letter() == 'N')
        enc = encoding_names[1];
    else if (pdec->get_id_letter() == 'M')
        enc = encoding_names[2];
    else
        enc = encoding_names[3];

    serial_printf("\n// -----CODE START-----\n");
    serial_printf("    // [WRITE THE DEVICE NAME HERE]\n"
            "rf.register_Receiver(\n");
    serial_printf("%s, // mod\n", enc);
    serial_printf("%5u, // initseq\n", tsext.initseq);
    serial_printf("%5u, // lo_prefix\n", tsext.first_low);
    serial_printf("%5u, // hi_prefix\n", tsext.first_high);
    serial_printf("%5u, // first_lo_ign\n", tsext.first_low_ignored);
    serial_printf("%5u, // lo_short\n", tsext.low_short);
    serial_printf("%5u, // lo_long\n", tsext.low_long);
    serial_printf("%5u, // hi_short (0 => take lo_short)\n", tsext.high_short);
    serial_printf("%5u, // hi_long  (0 => take lo_long)\n", tsext.high_long);
    serial_printf("%5u, // lo_last\n", tsext.last_low);
    serial_printf("%5u, // sep\n", tsext.sep);
    serial_printf("%5u  // nb_bits\n", nb_bits);
    serial_printf(");\n");
    serial_printf("// -----CODE END-----\n\n");
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
        output_timings(pdec, nb_bits);
        pdec = pdec->get_next();
    }
    delete pdec0;
}

// vim: ts=4:sw=4:tw=80:et
