// 04_callback.ino

// Example sketch that comes along with RF433any library
// Shows how to trigger different actions depending on code received, by
// registering callback function.

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

Track track(PIN_RFINPUT);

byte dummy;

void on_call(void *data) {
    byte n = (byte *)data - &dummy;
    Serial.print("Received code number ");
    Serial.print(n);
    Serial.print("\n");
}

void setup() {
    pinMode(PIN_RFINPUT, INPUT);
    Serial.begin(115200);
    track.register_callback(RF433ANY_ID_TRIBIT,
            new BitVector(32, 4, 0xb9, 0x35, 0x6d, 0x00),
            (void *)(&dummy + 1), on_call, 2000);
    track.register_callback(RF433ANY_ID_TRIBIT,
            new BitVector(32, 4, 0xb5, 0x35, 0x6d, 0x00),
            (void *)(&dummy + 2), on_call, 2000);
}

void loop() {
    track.treset();
    while (!track.do_events())
        delay(1);
}

// vim: ts=4:sw=4:tw=80:et
