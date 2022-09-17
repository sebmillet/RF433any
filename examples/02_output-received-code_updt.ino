// 02_output-received-code.ino

// Example sketch that comes along with RF433any library
// Simply displays received codes

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

void setup() {
    pinMode(PIN_RFINPUT, INPUT);
    Serial.begin(115200);
    Serial.print("Waiting for signal\n");
}

Track track(PIN_RFINPUT);

// NOTE
//   One could also create a child class, it is cleaner, but I find it overkill
char* my_BitVector_to_str(const BitVector *bv) {
    if (!bv->get_nb_bits())
        return nullptr;

    byte nb_bytes = bv->get_nb_bytes();

    char *ret = (char*)malloc(nb_bytes * 3);
    char tmp[3];
    int j = 0;
    for (int i = nb_bytes - 1; i >= 0 ; --i) {
        snprintf(tmp, sizeof(tmp), "%02X", bv->get_nth_byte(i));
        ret[j] = tmp[0];
        ret[j + 1] = tmp[1];
        ret[j + 2] = (i > 0 ? ' ' : '\0');
        j += 3;
    }

    return ret;
}

void loop() {
    track.treset();

    while (!track.do_events())
        delay(1);

    Decoder *pdec0 = track.get_data(
        RF433ANY_FD_DECODED | RF433ANY_FD_DEDUP | RF433ANY_FD_NO_ERROR
    );
    for (Decoder *pdec = pdec0; pdec != nullptr; pdec = pdec->get_next()) {
        Serial.print("Received ");
        Serial.print(pdec->get_nb_bits());
        Serial.print(" bits (x");
        Serial.print(pdec->get_repeats() + 1);
        Serial.print("): ");
        char *buf = my_BitVector_to_str(pdec->get_pdata());
        Serial.println(buf);
        free(buf);
    }
    delete pdec0;
}

// vim: ts=4:sw=4:tw=80:et
