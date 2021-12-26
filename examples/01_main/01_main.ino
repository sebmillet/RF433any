// 01_main.ino

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
// Schematic: Radio Frequencies RECEIVER plugged on D2.
// You can change it by updating PIN_RFINPUT below.
// Since the library RF433any uses interruptions, the pin MUST BE either 2 or 3.
//

#include "RF433any.h"
#include <Arduino.h>

#define PIN_RFINPUT  2

    // Comment the below macro if you wish to output everything.
    // As most codes are repeated, this'll likely result in the output of the
    // same thing multiple times.
#define OUTPUT_FIRST_DECODED_ONLY

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
    "RFMOD_TRIBIT",          // T
    "RFMOD_TRIBIT_INVERTED", // N
    "RFMOD_MANCHESTER",      // M
    "<unmanaged encoding>"   // Anything else
};

const char *id_letter_to_encoding_name(char c) {
    if (c == 'T')
        return encoding_names[0];
    else if (c == 'N')
        return encoding_names[1];
    else if (c == 'M')
        return encoding_names[2];

    return encoding_names[3];
}

void output_timings(Decoder *pdec, byte nb_bits) {
    TimingsExt tsext;
    if (!pdec)
        return;
    pdec->get_tsext(&tsext);

    const char *enc_name = id_letter_to_encoding_name(pdec->get_id_letter());

    serial_printf("\n-----CODE START-----\n");
    serial_printf("// [WRITE THE DEVICE NAME HERE]\n"
            "rf.register_Receiver(\n");
    serial_printf("\t%s, // mod\n", enc_name);
    serial_printf("\t%u, // initseq\n", tsext.initseq);
    serial_printf("\t%u, // lo_prefix\n", tsext.first_low);
    serial_printf("\t%u, // hi_prefix\n", tsext.first_high);
    serial_printf("\t%u, // first_lo_ign\n", tsext.first_low_ignored);
    serial_printf("\t%u, // lo_short\n", tsext.low_short);
    serial_printf("\t%u, // lo_long\n", tsext.low_long);
    serial_printf("\t%u, // hi_short (0 => take lo_short)\n", tsext.high_short);
    serial_printf("\t%u, // hi_long  (0 => take lo_long)\n", tsext.high_long);
    serial_printf("\t%u, // lo_last\n", tsext.last_low);
    serial_printf("\t%u, // sep\n", tsext.sep);
    serial_printf("\t%u  // nb_bits\n", nb_bits);
    serial_printf(");\n");
    serial_printf("-----CODE END-----\n\n");
}

void loop() {
    track.treset();

    while (!track.do_events())
        delay(1);

    Decoder *pdec0 = track.get_data(RF433ANY_FD_ALL);
    Decoder *pdec = pdec0;
    while (pdec) {
        int nb_bits = pdec->get_nb_bits();
        BitVector *pdata = pdec->take_away_data();

        if (pdata) {
            Serial.print("Data: ");
            char *buf = pdata->to_str();
            if (buf) {
                Serial.print(buf);
                free(buf);
            }
            delete pdata;
        }
        Serial.print("\n");
        output_timings(pdec, nb_bits);

#ifdef OUTPUT_FIRST_DECODED_ONLY
        pdec = nullptr;
        delay(1000);
#else
        pdec = pdec->get_next();
#endif

    }
    delete pdec0;
}

// vim: ts=4:sw=4:tw=80:et
