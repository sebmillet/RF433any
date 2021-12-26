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
        char *buf = pdec->get_pdata()->to_str();
        Serial.println(buf);
        free(buf);
    }
    delete pdec0;
}

// vim: ts=4:sw=4:tw=80:et
