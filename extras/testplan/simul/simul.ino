// simul.ino

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

#include "RF433Serial.h"
#include <Arduino.h>

extern uint16_t sim_timings_count;
extern unsigned int counter;
extern unsigned int sim_int_count;
extern char buffer[RF433SERIAL_LINE_BUF_LEN];
extern RF433SerialLine sl;
extern duration_t sim_timings[SIM_TIMINGS_LEN];

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
}

Track track(PIN_RFINPUT);

void read_simulated_timings_from_usb() {
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

        dbgf("==READ LINE [%s]", buffer);

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

        sim_timings[sim_timings_count++] = compact(l);
        sim_timings[sim_timings_count++] = compact(h);
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

const char *encoding_names[] = {
    "R",          // T
    "R", // N
    "R",      // M
    "<"   // Anything else
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

    dbg("\n-----CODE START-----");
    dbg("// [WRITE THE DEVICE NAME HERE]\n"
            "rf.register_Receiver(");
    dbgf("\t%s, // mod", enc_name);
    dbgf("\t%u, // initseq", tsext.initseq);
    dbgf("\t%u, // lo_prefix", tsext.first_low);
    dbgf("\t%u, // hi_prefix", tsext.first_high);
    dbgf("\t%u, // first_lo_ign", tsext.first_low_ignored);
    dbgf("\t%u, // lo_short", tsext.low_short);
    dbgf("\t%u, // lo_long", tsext.low_long);
    dbgf("\t%u, // hi_short (0 => take lo_short)", tsext.high_short);
    dbgf("\t%u, // hi_long  (0 => take lo_long)", tsext.high_long);
    dbgf("\t%u, // lo_last", tsext.last_low);
    dbgf("\t%u, // sep", tsext.sep);
    dbgf("\t%u  // nb_bits", nb_bits);
    dbg(");");
    dbg("-----CODE END-----\n");
}

void loop() {
    if (sim_int_count >= sim_timings_count)
        read_simulated_timings_from_usb();

    ++counter;

    track.treset();
    while (track.get_trk() != TRK_DATA && sim_int_count <= sim_timings_count) {
        Track::ih_handle_interrupt();
        track.do_events();
    }
    track.force_stop_recv();

    Decoder *pdec = track.get_data(0);
    if (pdec) {
        int nb_bits = pdec->get_nb_bits();
        output_timings(pdec, nb_bits);
        delete pdec;
    }
}

// vim: ts=4:sw=4:tw=80:et
