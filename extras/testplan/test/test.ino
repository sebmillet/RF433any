// test.ino

// Perform RF433any library test plan

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

#define PIN_RFINPUT  2

#include "RF433any.h"
#include "Serial.h"
#include <Arduino.h>

extern uint16_t sim_timings_count;
extern unsigned int counter;
extern unsigned int sim_int_count;
extern char buffer[SERIAL_LINE_BUF_LEN];
extern SerialLine sl;
extern uint16_t sim_timings[SIM_TIMINGS_LEN];
extern unsigned int sim_int_count_svg;

bool filter_mask_set;
uint16_t filter_mask;

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

void setup() {
    pinMode(PIN_RFINPUT, INPUT);
    Serial.begin(115200);
}

Track track(PIN_RFINPUT);

void read_simulated_timings_from_usb() {
    filter_mask_set = false;
    sim_timings_count = 0;
    sim_int_count = 0;
    counter = 0;
    buffer[0] = '\0';
    for (   ;
            strcmp(buffer, ".");
            sl.get_line_blocking(buffer, sizeof(buffer))
        ) {

        if (!strlen(buffer))
            continue;

        char *p = buffer;
        while (*p != ',' && *p != '\0')
            ++p;
        if (*p != ',') {
            dbg("FATAL: each line must have a ',' character!");
            assert(false);
        }

        *p = '\0';
        unsigned int l = atoi(buffer);
        unsigned int h = atoi(p + 1);

        if (sim_timings_count >=
                sizeof(sim_timings) / sizeof(*sim_timings) - 1) {
            dbg("FATAL: timings buffer full!");
            assert(false);
        }

#if RF433ANY_TESTPLAN == 5
        if (!filter_mask_set) {
            filter_mask_set = true;
            filter_mask = l;
        } else {
            sim_timings[sim_timings_count++] = l;
            sim_timings[sim_timings_count++] = h;
        }
#else
        sim_timings[sim_timings_count++] = l;
        sim_timings[sim_timings_count++] = h;
#endif
    }
}

#if RF433ANY_TESTPLAN == 5
void output_decoder(Decoder *pdec) {
    while (pdec) {
        dbgf("Decoded: %s, err: %d, code: %c, "
                "rep: %d, bits: %2d",
                (pdec->data_got_decoded() ? "yes" : "no "),
                pdec->get_nb_errors(), pdec->get_id_letter(),
                pdec->get_repeats() + 1, pdec->get_nb_bits());

        if (pdec->data_got_decoded()) {
            Serial.print("  Data: ");
            if (pdec->get_pdata()) {
                char *buf = pdec->get_pdata()->to_str();
                if (buf) {
                    Serial.print(buf);
                    free(buf);
                }
            }
            Serial.print("\n");
        }
        pdec = pdec->get_next();
    }
}
#endif

void loop() {
    if (sim_int_count >= sim_timings_count)
        read_simulated_timings_from_usb();

    if (!counter) {
        delay(100);
        dbg("----- BEGIN TEST -----");
#ifdef RF433ANY_DBG_TRACK
        dbg("[");
#endif
    }

    ++counter;

    track.treset();
    sim_int_count_svg = sim_int_count;
    while (track.get_trk() != TRK_DATA && sim_int_count <= sim_timings_count) {
        for (int i = 0; i < 2; ++i) {
            Track::ih_handle_interrupt();
        }
        track.do_events();
    }
    track.force_stop_recv();

#ifdef RF433ANY_DBG_TIMINGS
    track.dbg_timings();
#endif

#ifdef RF433ANY_DBG_TRACK
    if (sim_int_count >= sim_timings_count) {
        dbg("]");
    }
#endif

    Decoder *pdec = track.get_data(filter_mask);
    if (pdec) {
#ifdef RF433ANY_DBG_DECODER
        pdec->dbg_decoder(2);
#endif
#if RF433ANY_TESTPLAN == 5
        output_decoder(pdec);
#endif
        delete pdec;
    }

    if (sim_int_count >= sim_timings_count) {
        dbg("----- END TEST -----");
    }
}

// vim: ts=4:sw=4:tw=80:et
