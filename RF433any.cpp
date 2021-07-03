// RF433any.cpp

// See README.md about the purpose of this library

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

/*
  Schematic

  1. Arduino board. Tested with NANO and UNO.
  2. Radio Frequence 433Mhz RECEIVER like MX-RM-5V.

  RF433 RECEIVER data pin must be plugged on a board' digital PIN that can
  trigger interrupts, that is, D2 or D3.
  This RECEIVER PIN is defined at the time a 'Track' object is created. This
  library does not set it at compile time.
  See file schema.fzz (Fritzing format) or schema.png, for a circuit example
  with receiver plugged on D2.
*/

/*

**About the classes Band, Rail and Track**

1. About none of these - the signal as we see it.

The Radio-Frequence signal is supposed to be OOK (On-Off Keying), and
auto-synchronized.

The signal is a succession of low signal and high signal, low when no RF signal
received, high when a RF signal is received.
The coding relies on durations being either 'short' or 'long', and sometimes
much longer (to initialize, and to separate signal pieces).

The durations can be one of:
    - short
    - long, typically, twice as long as short
    - separator, much longer than the long one (at least 3 or 4 times longer)
    - initialization, at least as long as the separator, often much longer. It
      serves to make receiver ready to receive coded signal to come.

A signal structure is as follows:
    1. Initialization (very long high signal)
    2. Succession of low and high signals being 'short' or 'long'
    3. Separator (high signal)
    4. Possibly, repetition of steps 2 and 3

The succession of 'short' and 'long' is then decoded into original data, either
based on tri-bit scheme (inverted or not), or, Manchester.

Note that there can be complexities:

- After the long initialization high signal, addition of 'intermediate' prefix
  to the signal (longer than 'long', but shorter than 'separator'). Seen on a
  NICE FLO/R telecommand (/R means Rolling Code), while not seen on NICE FLO
  (fix code). The author guesses this prefix serves to let the receiver know the
  signal to come is FLO/R instead of FLO.

- After the long initialization high signal, succession of {low=short,
  high=short} followed by a separator. This serves as a synchronization
  sequence.

- While most protocols use same lengths for low and high signals, on NICE FLO/R
  this rule is not met, that is: the 'short' and 'long' durations of the low
  signal are different from 'short' and 'long' durations of the high signal.

2. About Rail

The Rail manages the succession of durations for one, and only one, of signal
realms (low or high).

That is, if you note dow the signal as usually (by line, one low followed by one
high):
      LOW, HIGH
      150,  200
      145,  400
      290,  195
        ...

Then the values below LOW (150, 145, 290, ...) are one Rail, and the values
below HIGH (200, 400, 195, ...) are another Rail.

3. About Bands

A band aims to categorize a duration, short or long. Therefore, a Rail is made
of 2 bands, one for the short duration, one for the long duration.

4. About Tracks

Rails live their own live but at some point, they must work in conjunction
(start and stop together, and provide final decoded values). This is the purpose
of a Track, that is made of 2 Rails.

In the end, a Track provides a convenient interface to the caller.

5. Overall schema

track ->  r_low  ->  b_short = manage short duration on LOW signal
      |          `-> b_long  = manage long duration on LOW signal
      |
      `-> r_high ->  b_short = manage short duration on HIGH signal
                 `-> b_long  = manage long duration on HIGH signal

*/

#include "RF433any.h"
#include <Arduino.h>

#define ASSERT_OUTPUT_TO_SERIAL

#define assert(cond) { \
    if (!(cond)) { \
        rf433any_assert_failed(__LINE__); \
    } \
}

static void rf433any_assert_failed(int line) {
#ifdef ASSERT_OUTPUT_TO_SERIAL
    Serial.print("\nRF433any.cpp:");
    Serial.print(line);
    Serial.println(": assertion failed, aborted.");
#endif
    while (1)
        ;
}


// * **** *********************************************************************
// * Band *********************************************************************
// * **** *********************************************************************

inline void Band::breset() {
    inf = 0;
    sup = 0;
    mid = 0;
}

inline bool Band::init(uint16_t d) {
#ifdef RF433ANY_DBG_TRACE
    dbgf("B> init: %u", d);
#endif

    if (d >= BAND_MIN_D && d <= BAND_MAX_D) {
        mid = d;
        uint16_t d_divided_by_4 = d >> 2;
        inf = d - d_divided_by_4;
        sup = d + d_divided_by_4;
        got_it = true;
    } else {
        got_it = false;
    }

    return got_it;
}

inline bool Band::init_sep(uint16_t d) {
#ifdef RF433ANY_DBG_TRACE
    dbgf("BSEP> init: %u", d);
#endif

    sup = RF433ANY_MAX_SEP_DURATION;
    inf = d >> 1;
    inf += (inf >> 2);
    mid = d;

    got_it = true;
    return got_it;
}

inline bool Band::test_value_init_if_needed(uint16_t d) {
    if (!mid) {
        init(d);
    } else {
        got_it = (d >= inf && d <= sup);
#ifdef RF433ANY_DBG_TRACE
        dbgf("B> cmp %u to [%u, %u]", d, inf, sup);
#endif
    }
#ifdef RF433ANY_DBG_TRACE
    dbgf("B> res: %d", got_it);
#endif
    return got_it;
}

inline bool Band::test_value(uint16_t d) {
    if (!mid) {
        got_it = false;
#ifdef RF433ANY_DBG_TRACE
        dbgf("BSEP> cmp %u to uninitialized d", d);
#endif
    } else {
        got_it = (d >= inf && d <= sup);
#ifdef RF433ANY_DBG_TRACE
        dbgf("BSEP> cmp %u to [%u, %u]", d, inf, sup);
#endif
    }
#ifdef RF433ANY_DBG_TRACE
    dbgf("BSEP> res: %d", got_it);
#endif
    return got_it;
}


// * **** *********************************************************************
// * Rail *********************************************************************
// * **** *********************************************************************

Rail::Rail(byte arg_mood):mood(arg_mood) {
    rreset();
}

inline void Rail::rreset() {
    rreset_soft();

    b_short.breset();
    b_long.breset();
    b_sep.breset();
}

inline void Rail::rreset_soft() {
    status = RAIL_OPEN;
    index = 0;
    rec = 0;
}

inline bool Rail::rail_eat(uint16_t d) {
#ifdef RF433ANY_DBG_TRACE
    dbgf("R> index = %d, d = %u", index, d);
#endif

    if (status != RAIL_OPEN)
        return false;

    byte count_got_it = 0;
    if (b_short.test_value_init_if_needed(d))
        ++count_got_it;
    if (b_long.test_value_init_if_needed(d))
        ++count_got_it;

    byte band_count = get_band_count();

#ifdef RF433ANY_DBG_TRACE
    dbgf("R> b_short.got_it = %d, b_long.got_it = %d, "
                  "band_count = %d", b_short.got_it, b_long.got_it,
                  band_count);
    for (int i = 0; i < 2; ++i) {
        dbgf("R>  [%i]: inf = %u, mid = %u, sup = %u", i,
                      (i == 0 ? b_short.inf : b_long.inf),
                      (i == 0 ? b_short.mid : b_long.mid),
                      (i == 0 ? b_short.sup : b_long.sup));
    }
#endif

    if (band_count == 1 && !count_got_it) {
        Band *pband;
            // IMPORTANT
            //   We are using below 'unsigned long' although they are
            //   initialized using uint16_t values.
            //   We need 'unsigned long' because later in the code, we check
            //   whether 'big' is not more than 4 times 'short' (if it is, then
            //   the coding shape is too distorted and we give up).
            //   We do this check by calculating 'small << 2', therefore it
            //   could be that this operation ends up above 16-bit max unsigned
            //   integer value.
        unsigned long small;
        unsigned long big;
        if (d < b_short.inf) {
            pband = &b_short;
            small = d;
            big = b_short.mid;
        } else if (d > b_short.sup) {
            pband = &b_long;
            small = b_short.mid;
            big = d;
        } else {
                // Should not happen.
                // If value is within band range, then why the hell didn't the
                // range grab it?
            assert(false);
        }

#ifdef RF433ANY_DBG_TRACE
        dbg("R> P0");
#endif

        if ((small << 2) >= big) {
            if (pband->init(d)) {

#ifdef RF433ANY_DBG_TRACE
                dbg("R> P1");
#endif

                    // As we now know who's who (b_short is b_short and b_long
                    // is b_long, yes), we can adjust boundaries accordingly.

                b_short.inf = (b_short.mid >> 1) - (b_short.mid >> 3);
                if (mood == RAIL_MOOD_LAXIST) {
                    b_short.sup = (b_short.mid + b_long.mid) >> 1;
                    b_long.inf = b_short.sup + 1;
                }
                b_long.sup = b_long.mid + (b_long.mid >> 1) + (b_long.mid >> 3);

                count_got_it = 1;
                band_count = 2;

                    // Test if intervals overlap?
                    // That is, test if b_short.sup >= b_long.inf?
                    // Not done for now...
                ;

                if (pband == &b_short) {
                        // The first N signals received ('N' equals 'index')
                        // happened to be LONG ones => to be recorded as as many
                        // ONEs.
                    rec = ((recorded_t)1 << index) - 1;
                }
            }
        }
    }

    if (!band_count) {
        status = RAIL_ERROR;
        return false;
    }

    if (!count_got_it || (band_count == 2 && count_got_it == 2)) {
        if (!b_sep.mid) {
                // BAND_MAX_D is 30000, and multiplying .mid by 2 will produce a
                // maximum value of 60000, that's OK for an unsigned 16-bit int.
            if (d >= (b_short.mid << 1) && d >= (b_long.mid << 1)) {
#ifdef RF433ANY_DBG_TRACE
                dbg("R> init b_sep");
#endif
                    // We can end up with an overlap between b_sep and b_long.
                    // Not an issue.
                b_sep.init_sep(d);
            } else {
#ifdef RF433ANY_DBG_TRACE
                dbg("R> no init of b_sep (d too small)");
#endif
            }
        }
        status = (b_sep.test_value(d) ? RAIL_STP_RCVD : RAIL_ERROR);

#ifdef RF433ANY_DBG_TRACE
        dbgf("R> rail terminated, status = %d", status);
#endif

    } else {

        if (band_count == 2) {
            if (b_short.got_it == b_long.got_it) {
                assert(false);
            }
            last_bit_recorded = (b_short.got_it ? 0 : 1);
            rec = (rec << 1) | last_bit_recorded;
        } else {
            last_bit_recorded = 0;
        }
        if (++index == (sizeof(rec) << 3)) {
            status = RAIL_FULL;
        }

    }

    return (status == RAIL_OPEN);
}

#ifdef RF433ANY_DBG_TRACK
const char* status_names[] = {
    "open",
    "full",
    "stop received",
    "closed",
    "error"
};
void Rail::rail_debug() const {
    dbgf("      \"bits\":%i,\"v\":0x" FMTRECORDEDT
         ",\"railstatus\":\"%s\",\"n\":%d,", index, rec, status_names[status],
         (b_short.mid == b_long.mid ? 1 : 2));
    for (byte i = 0; i < 3; ++i) {
        dbgf("      \"%s\":{\"inf\":%u,\"mid\":%u,\"sup\":%u}%s",
                 (i == 0 ? "b_short" : (i == 1 ? "b_long" : "b_sep")),
                 (i == 0 ? b_short.inf : (i == 1 ? b_long.inf : b_sep.inf)),
                 (i == 0 ? b_short.mid : (i == 1 ? b_long.mid : b_sep.mid)),
                 (i == 0 ? b_short.sup : (i == 1 ? b_long.sup : b_sep.sup)),
                 (i == 2 ? "" : ",")
             );
    }
}
#endif

byte Rail::get_band_count() const {
    return b_short.mid == b_long.mid ? (b_short.mid ? 1 : 0) : 2;
}


// * **** *********************************************************************
// * Misc *********************************************************************
// * **** *********************************************************************

#ifdef RF433ANY_DBG_RAWCODE
const char *sts_names[] = {
    "CONT",
    "SSEP",
    "LSEP",
    "2SEP",
    "ERR"
};

void RawCode::debug_rawcode() const {
    dbgf("> nb_sections = %d, initseq = %u",
            nb_sections, initseq);
    for (byte i = 0; i < nb_sections; ++i) {
        const Section *psec = &sections[i];
        dbgf("  %02d  %s", i, sts_names[psec->sts]);
        dbgf("      sep = %u", psec->ts.sep);
        dbgf("      low:  [%d] n = %2d, v = 0x" FMTRECORDEDT "",
                      psec->low_bands, psec->low_bits, psec->low_rec);
        dbgf("      high: [%d] n = %2d, v = 0x" FMTRECORDEDT "",
                      psec->high_bands, psec->high_bits, psec->high_rec);
    }
}
#endif


// * ********* ****************************************************************
// * BitVector ****************************************************************
// * ********* ****************************************************************

BitVector::BitVector():
        array(nullptr),
        allocated(0),
        nb_bits(0) {

}

void BitVector::prepare_BitVector_construction(short arg_nb_bits,
        short arg_nb_bytes, short n) {
    assert(arg_nb_bits > 0);
    assert((arg_nb_bits + 7) >> 3 == arg_nb_bytes);
    assert(arg_nb_bytes == n);
    array = (uint8_t*)malloc(arg_nb_bytes);
    allocated = arg_nb_bytes;
    nb_bits = arg_nb_bits;
}

BitVector::BitVector(short arg_nb_bits, short arg_nb_bytes, byte b0,
        byte b1) {
    prepare_BitVector_construction(arg_nb_bits, arg_nb_bytes, 2);
    array[1] = b0;
    array[0] = b1;
}

BitVector::BitVector(short arg_nb_bits, short arg_nb_bytes, byte b0, byte b1,
        byte b2) {
    prepare_BitVector_construction(arg_nb_bits, arg_nb_bytes, 3);
    array[2] = b0;
    array[1] = b1;
    array[0] = b2;
}

BitVector::BitVector(short arg_nb_bits, short arg_nb_bytes, byte b0, byte b1,
        byte b2, byte b3) {
    prepare_BitVector_construction(arg_nb_bits, arg_nb_bytes, 4);
    array[3] = b0;
    array[2] = b1;
    array[1] = b2;
    array[0] = b3;
}

BitVector::BitVector(short arg_nb_bits, short arg_nb_bytes, byte b0, byte b1,
        byte b2, byte b3, byte b4) {
    prepare_BitVector_construction(arg_nb_bits, arg_nb_bytes, 5);
    array[4] = b0;
    array[3] = b1;
    array[2] = b2;
    array[1] = b3;
    array[0] = b4;
}

BitVector::BitVector(short arg_nb_bits, short arg_nb_bytes, byte b0, byte b1,
        byte b2, byte b3, byte b4, byte b5) {
    prepare_BitVector_construction(arg_nb_bits, arg_nb_bytes, 6);
    array[5] = b0;
    array[4] = b1;
    array[3] = b2;
    array[2] = b3;
    array[1] = b4;
    array[0] = b5;
}

BitVector::~BitVector() {
    if (array)
        free(array);
}

void BitVector::add_bit(byte v) {
    if (!allocated)
        array = (uint8_t*)malloc(1);
    if (nb_bits >= (allocated << 3)) {
        byte old_allocated = allocated;

            // FIXME
        ++allocated;    // Could be another formula ('<<= 1', ...)

        array = (uint8_t*)realloc(array, allocated);
        for (byte i = old_allocated; i < allocated; ++i)
            array[i] = 0;
    }

    ++nb_bits;
    for (short i = allocated - 1; i >= 0; --i) {

        byte b;
        if (i > 0) {
            b = !!(array[i - 1] & 0x80);
        } else {
                // Defensive programming:
                //   Normally v is 0 or 1, but I normalize it, just in case.
            b = !!v;
        }

        array[i]= (array[i] << 1) | b;

    }
}

int BitVector::get_nb_bits() const {
    return nb_bits;
}

byte BitVector::get_nb_bytes() const {
    return (nb_bits + 7) >> 3;
}

    // Bit numbering starts at 0
byte BitVector::get_nth_bit(byte n) const {
    assert(n >= 0 && n < nb_bits);
    byte index = (n >> 3);
    byte bitread = (1 << (n & 0x07));
    return !!(array[index] & bitread);
}

    // Bit numbering starts at 0
byte BitVector::get_nth_byte(byte n) const {
    assert(n >= 0 && n < get_nb_bytes());
    return array[n];
}

    // *IMPORTANT*
    //   If no data got received, returns nullptr. So, you must test the
    //   returned value.
    //
    // *IMPORTANT (2)*
    //   The return value is malloc'd so caller must think of freeing it.
    //   For example:
    //     char *s = data_to_str_with_malloc(data);
    //     ...
    //     if (s)       // DON'T FORGET (s can be null)
    //         free(s); // DON'T FORGET! (if non-null, s must be freed)
char* BitVector::to_str() const {
    if (!get_nb_bits())
        return nullptr;

    byte nb_bytes = get_nb_bytes();

    char *ret = (char*)malloc(nb_bytes * 3);
    char tmp[3];
    int j = 0;
    for (int i = nb_bytes - 1; i >= 0 ; --i) {
        snprintf(tmp, sizeof(tmp), "%02x", get_nth_byte(i));
        ret[j] = tmp[0];
        ret[j + 1] = tmp[1];
        ret[j + 2] = (i > 0 ? ' ' : '\0');
        j += 3;
    }
    assert(j <= nb_bytes * 3);

    return ret;
}

short BitVector::cmp(const BitVector *p) const {
    assert(p);
    short cmp_nb_bits = (get_nb_bits() > p->get_nb_bits());
    if (!cmp_nb_bits)
        cmp_nb_bits = -(get_nb_bits() < p->get_nb_bits());

    if (cmp_nb_bits)
        return cmp_nb_bits;

    for (int i = get_nb_bits() - 1; i >= 0; --i) {
        byte v1 = get_nth_bit(i);
        byte v2 = p->get_nth_bit(i);
        if (v1 > v2)
            return 1;
        if (v1 < v2)
            return -1;
    }

    return 0;
}


// * ******* ******************************************************************
// * Decoder ******************************************************************
// * ******* ******************************************************************

#ifdef RF433ANY_DBG_DECODER
const char *dec_id_names[] = {
    "INC",
    "SYN",
    "TRI",
    "TRN",
    "MAN",
    "UNK"
};
#endif

Decoder::Decoder(byte arg_convention):
        next(nullptr),
        pdata(new BitVector()),
        convention(arg_convention),
        nb_errors(0) {
    tsext.initseq = 0;
    tsext.first_low = 0;
    tsext.first_high = 0;
    tsext.first_low_ignored = 0;
    tsext.last_low = 0;
}

Decoder::~Decoder() {
    if (pdata)
        delete pdata;
    if (next)
        delete next;
}

Decoder* Decoder::build_decoder(byte id, byte convention) {
    switch (id) {
        case RF433ANY_ID_RAW_SYNC:
            return new DecoderRawSync(0);
        case RF433ANY_ID_TRIBIT:
            return new DecoderTriBit(convention);
        case RF433ANY_ID_TRIBIT_INV:
            return new DecoderTriBitInv(convention);
        case RF433ANY_ID_MANCHESTER:
            return new DecoderManchester(convention);
        case RF433ANY_ID_RAW_UNKNOWN_CODING:
            return new DecoderRawUnknownCoding();
        default:
            assert(false);
    }
    return nullptr; // Never executed
}

void Decoder::attach(Decoder *pdec) {
    assert(!next);
    next = pdec;
}

void Decoder::detach() {
    next = nullptr;
}

void Decoder::add_data_bit(byte valbit) {
    pdata->add_bit(valbit);
}

byte Decoder::get_nb_errors() const { return nb_errors; }

int Decoder::get_nb_bits() const { return pdata ? pdata->get_nb_bits() : 0; }

void Decoder::set_ts(const uint16_t& arg_initseq, const Timings& ts) {
    tsext.initseq = arg_initseq;
    tsext.low_short = ts.low_short;
    tsext.low_long = ts.low_long;
    tsext.high_short = ts.high_short;
    tsext.high_long = ts.high_long;
    tsext.sep = ts.sep;
}

void Decoder::get_tsext(TimingsExt *p_tsext) const {
    *p_tsext = tsext;
    p_tsext->first_low_ignored = first_lo_ignored();
}

void Decoder::take_into_account_first_low_high(const Section *psec,
        bool is_cont_of_prev_sec) {
    if (is_cont_of_prev_sec)
        return;
    tsext.first_low = psec->first_low;
    tsext.first_high = psec->first_high;
    tsext.last_low = psec->last_low;

    Signal e[2];
    for (short i = 0; i < 2; ++i) {
        uint16_t d = (i == 0 ? tsext.first_low : tsext.first_high);
        uint16_t short_d = (i == 0 ? psec->ts.low_short : psec->ts.high_short);
        uint16_t long_d = (i == 0 ? psec->ts.low_long : psec->ts.high_long);
        Band b_short;
        Band b_long;
        b_short.init(short_d);
        b_long.init(long_d);

//        b_short.sup = (b_short.mid + b_long.mid) >> 1;
//        b_long.inf = b_short.sup + 1;

        bool is_short = b_short.test_value(d);
        bool is_long = b_long.test_value(d);

        if (is_short && !is_long) {
            e[i] = Signal::SHORT;
        } else if (!is_short && is_long) {
            e[i] = Signal::LONG;
        } else if (is_short && is_long && short_d == long_d) {
            e[i] = Signal::SHORT;
        } else {
            e[i] = Signal::OTHER;
        }
    }

    if (e[0] != Signal::OTHER && e[1] != Signal::OTHER) {
        add_signal_step(e[0], e[1]);
        tsext.first_low = 0;
        tsext.first_high = 0;
    }
}

void Decoder::decode_section(const Section *psec, bool is_cont_of_prev_sec) {
    take_into_account_first_low_high(psec, is_cont_of_prev_sec);

    byte pos_low = psec->low_bits;
    byte pos_high = psec->high_bits;

    while (pos_low >= 1 || pos_high >= 1) {
        Signal sd_low = Signal::OTHER;
        Signal sd_high = Signal::OTHER;
        if (pos_low >= 1) {
            --pos_low;
            sd_low = ((((recorded_t)1 << pos_low) & psec->low_rec) ?
                        Signal::LONG : Signal::SHORT);
        }
        if (pos_high >= 1) {
            --pos_high;
            sd_high =
                ((((recorded_t)1 << pos_high) & psec->high_rec) ?
                Signal::LONG : Signal::SHORT);
        }
        add_signal_step(sd_low, sd_high);
    }
}

uint16_t Decoder::first_lo_ignored() const {
    return 0;
}

const BitVector* Decoder::get_pdata() const {
    return pdata;
}

BitVector* Decoder::take_away_data() {
    if (pdata) {
        BitVector *ret = pdata;
        pdata = nullptr;
        return ret;
    } else
        return nullptr;
}

#ifdef RF433ANY_DBG_DECODER
void Decoder::dbg_data(byte seq) const {
    char *buf = pdata->to_str();
    if (buf) {
        dbgf("[%d] Received %d bits%s: %s", seq, get_nb_bits(),
                (get_nb_errors() ? "(!)" : ""), buf);
        free(buf);
    } else {
        dbgf("[%d] No data received, type = %s", seq, dec_id_names[get_id()]);
    }
}

void Decoder::dbg_meta(byte disp_level) const {
    if (disp_level <= 1)
        return;
    if (!tsext.first_low && !tsext.first_high) {
        if (!tsext.high_short && !tsext.high_long) {
            dbgf("    T=%s, E=%u, I=%u, S=%u, L=%u, P=%u, Y=%u, Z=%u",
                    dec_id_names[get_id()], nb_errors, tsext.initseq,
                    tsext.low_short, tsext.low_long, tsext.sep,
                    first_lo_ignored(), tsext.last_low);
        } else {
            dbgf("    T=%s, E=%u, I=%u, S(lo)=%u, L(lo)=%u, "
                    "S(hi)=%u, L(hi)=%u, P=%u, Y=%u, Z=%u",
                    dec_id_names[get_id()], nb_errors, tsext.initseq,
                    tsext.low_short, tsext.low_long, tsext.high_short,
                    tsext.high_long, tsext.sep, first_lo_ignored(),
                    tsext.last_low);
        }
    } else {
        if (!tsext.high_short && !tsext.high_long) {
            dbgf("    T=%s, E=%u, I=%u, S=%u, L=%u, P=%u, U=%u, "
                        "V=%u, Y=%u, Z=%u",
                    dec_id_names[get_id()], nb_errors, tsext.initseq,
                    tsext.low_short, tsext.low_long, tsext.sep, tsext.first_low,
                    tsext.first_high, first_lo_ignored(), tsext.last_low);
        } else {
            dbgf("    T=%s, E=%u, I=%u, S(lo)=%u, L(lo)=%u, "
                    "S(hi)=%u, L(hi)=%u, P=%u, U=%u, V=%u, Y=%u, Z=%u",
                    dec_id_names[get_id()], nb_errors, tsext.initseq,
                    tsext.low_short, tsext.low_long, tsext.high_short,
                    tsext.high_long, tsext.sep, tsext.first_low,
                    tsext.first_high, first_lo_ignored(), tsext.last_low);
        }
    }
}

void Decoder::dbg_next(byte disp_level, byte seq) const {
    if (next)
        next->dbg_decoder(disp_level, seq + 1);
}

#endif


// * ********************** ***************************************************
// * DecoderRawInconsistent ***************************************************
// * ********************** ***************************************************

#ifdef RF433ANY_DBG_DECODER
void DecoderRawInconsistent::dbg_decoder(byte disp_level, byte seq) const {
    dbgf("[%d] Inconsistent signal", seq);
    dbg_meta(disp_level);
    dbg_next(disp_level, seq);
}
#endif


// * ************** ***********************************************************
// * DecoderRawSync ***********************************************************
// * ************** ***********************************************************

void DecoderRawSync::add_signal_step(Signal lo, Signal hi) {
    if (!sync_shape_set) {
        sync_shape = lo;
        sync_shape_set = true;
    }

    if (lo != sync_shape) {
        ++nb_errors;
    } else if (hi == Signal::OTHER) {
    } else if (lo != hi) {
        ++nb_errors;
    } else {
        ++nb_low_high;
    }
}

void DecoderRawSync::add_sync(byte n) {
    nb_low_high += n;
}

int DecoderRawSync::get_nb_bits() const { return nb_low_high; }

#ifdef RF433ANY_DBG_DECODER
void DecoderRawSync::dbg_decoder(byte disp_level, byte seq) const {
    dbgf("[%d] Sync %d", seq, nb_low_high);
    dbg_meta(disp_level);
    dbg_next(disp_level, seq);
}
#endif


// * *********************** **************************************************
// * DecoderRawUnknownCoding **************************************************
// * *********************** **************************************************

void DecoderRawUnknownCoding::add_signal_step(Signal lo, Signal hi) {
    if (hi == Signal::OTHER) {
        unused_final_low = lo;
        terminates_with_sep = true;
        return;
    }

    for (short i = 0; i < 2; ++i) {
        Signal x = (i ? hi : lo);
        add_data_bit(x == Signal::SHORT ? 0 : 1);
    }
}

#ifdef RF433ANY_DBG_DECODER
void DecoderRawUnknownCoding::dbg_decoder(byte disp_level, byte seq) const {
    dbgf("[%d] Unknown encoding: %d signal bits", seq, pdata->get_nb_bits());

    if (disp_level <= 1)
        return;

    int n = pdata->get_nb_bits();
    assert(!(n & 1));

    int sz = ((int)n * 3) / 2 + 4;
    char *buf = new char[sz];
    int p = 0;
    for (int i = n - 1; i >= 1; i -= 2) {
        byte vlo = pdata->get_nth_bit(i);
        byte vhi = pdata->get_nth_bit(i - 1);
        buf[p] = (vlo ? 'L' : 'S');
        buf[p + 1] = (vhi ? 'L' : 'S');
        buf[p + 2] = ':';
        p += 3;
    }
    assert(p + 2 < sz);
    if (terminates_with_sep) {
        if (unused_final_low == Signal::SHORT)
            buf[p] = 'S';
        else
            buf[p] = 'L';
        buf[p + 1] = 'P';
        buf[p + 2] = '\0';
    } else {
        if (!p)
            buf[p] = '\0';
        else
            buf[p - 1] = '\0';
    }
    Serial.print("    Signal: ");
    Serial.print(buf);
    Serial.print("\n");
    delete buf;

    dbg_meta(disp_level);
    dbg_next(disp_level, seq);
}
#endif


// * ************* ************************************************************
// * DecoderTriBit ************************************************************
// * ************* ************************************************************

void DecoderTriBit::add_signal_step(Signal lo, Signal hi) {
    if (hi == Signal::OTHER)
        return;

    byte valbit;
    if (lo == Signal::SHORT && hi == Signal::LONG)
        valbit = convention;
    else if (lo == Signal::LONG && hi == Signal::SHORT)
        valbit = !convention;
    else {
        ++nb_errors;
        return;
    }

    add_data_bit(valbit);
}

#ifdef RF433ANY_DBG_DECODER
void DecoderTriBit::dbg_decoder(byte disp_level, byte seq) const {
    dbg_data(seq);
    dbg_meta(disp_level);
    dbg_next(disp_level, seq);
}
#endif


// * **************** *********************************************************
// * DecoderTriBitInv *********************************************************
// * **************** *********************************************************

void DecoderTriBitInv::add_signal_step(Signal lo, Signal hi) {
    if (first_call_to_add_sgn_lo_hi) {
        first_call_to_add_sgn_lo_hi = false;
        unused_initial_low = lo;
        last_hi = hi;
        return;
    }

    bool add_it = true;
    byte valbit;
    if (lo == Signal::SHORT && last_hi == Signal::LONG)
        valbit = !convention;
    else if (lo == Signal::LONG && last_hi == Signal::SHORT)
        valbit = convention;
    else {
        ++nb_errors;
        add_it = false;
    }

    if (add_it)
        add_data_bit(valbit);

    last_hi = hi;
}

uint16_t DecoderTriBitInv::first_lo_ignored() const {
    switch (unused_initial_low) {
        case Signal::OTHER:
            return 0;
        case Signal::SHORT:
            return tsext.low_short;
        case Signal::LONG:
            return tsext.low_long;
        default:
            assert(false);
    };
    return 0; // Never executed
}

#ifdef RF433ANY_DBG_DECODER
void DecoderTriBitInv::dbg_decoder(byte disp_level, byte seq) const {
    dbg_data(seq);
    dbg_meta(disp_level);
    dbg_next(disp_level, seq);
}
#endif


// * ***************** ********************************************************
// * DecoderManchester ********************************************************
// * ***************** ********************************************************

DecoderManchester::DecoderManchester(byte arg_convention)
        :Decoder(arg_convention),
        buf_pos(0),
        leading_lo_hi_has_been_passed(false) {
    for (byte i = 0; i < sizeof(buf) / sizeof(*buf); ++i) {
        buf[i] = 0;
    }
}

inline void DecoderManchester::add_buf(byte r) {
    assert(buf_pos < sizeof(buf)  /sizeof(*buf));
    buf[buf_pos++] = r;
}

void DecoderManchester::consume_buf() {
    if (buf_pos >= 2) {
        if (leading_lo_hi_has_been_passed) {
            if (buf[0] == 0 && buf[1] == 1) {
                add_data_bit(convention);
            } else if (buf[0] == 1 && buf[1] == 0) {
                add_data_bit(!convention);
            } else {
                ++nb_errors;
            }
        } else {
            if (buf[0] != 0 || buf[1] != 1) {
                ++nb_errors;
            }
            leading_lo_hi_has_been_passed = true;
        }
            // Not always necessary, but harmless if done while not necessary
        buf[0] = buf[2];
        buf_pos -= 2;
    }
}

void DecoderManchester::add_signal_step(Signal lo, Signal hi) {
    if (lo == Signal::OTHER) {
        ++nb_errors;
        return;
    }

    for (byte i = 0; i < 2; ++i) {
        Signal sgn = (i == 0 ? lo : hi);
        add_buf(i);
        if (sgn == Signal::LONG)
            add_buf(i);
        consume_buf();
    }
}

#ifdef RF433ANY_DBG_DECODER
void DecoderManchester::dbg_decoder(byte disp_level, byte seq) const {
    dbg_data(seq);
    dbg_meta(disp_level);
    dbg_next(disp_level, seq);
}
#endif


// * ***** ********************************************************************
// * Track ********************************************************************
// * ***** ********************************************************************

#ifdef RF433ANY_DBG_SIMULATE
SerialLine sl;
char buffer[SERIAL_LINE_BUF_LEN];

uint16_t sim_timings[SIM_TIMINGS_LEN];

uint16_t sim_timings_count = 0;

unsigned int sim_int_count = 0;
unsigned int sim_int_count_svg;
unsigned int counter;
#endif

#ifdef RF433ANY_DBG_TIMINGS
uint16_t Track::ih_dbg_timings[40];
uint16_t Track::ih_dbg_exec[40];
unsigned int Track::ih_dbg_pos = 0;
#endif
volatile IH_timing_t Track::IH_timings[IH_SIZE];
volatile unsigned char Track::IH_write_head = 0;
volatile unsigned char Track::IH_read_head = 0;
byte Track::IH_max_pending_timings = 0;
bool Track::IH_interrupt_handler_is_attached = false;
volatile short Track::IH_wait_free_count_ok;
volatile uint16_t Track::IH_wait_free_last16;

    // Set when Track object is created
byte Track::pin_number = 99;

Track::Track(int arg_pin_number, byte mood):
        r_low(mood),
        r_high(mood) {
    pin_number = arg_pin_number;
    treset();
}

void Track::treset() {
    trk = TRK_WAIT;
    rawcode.nb_sections = 0;
}

void Track::ih_handle_interrupt() {
    static unsigned long last_t = 0;
    const unsigned long t = micros();

#ifdef RF433ANY_DBG_SIMULATE
    unsigned long d;
    byte r = sim_int_count % 2;
    if (sim_int_count >= sim_timings_count) {
        d = 100;
        sim_int_count = sim_timings_count + 1;
    } else {
        d = sim_timings[sim_int_count++];
    }
    (void)last_t;
    (void)t;
#else
    unsigned long d = t - last_t;
    last_t = t;
    byte r = (digitalRead(pin_number) == HIGH ? 1 : 0);
#endif

    if (d > RF433ANY_MAX_DURATION)
        d = RF433ANY_MAX_DURATION;

    unsigned char next_IH_write_head = (IH_write_head + 1) & IH_MASK;
        // No ideal solution here: we reached the buffer size, so either we
        // write nothing, or, we loose the oldest entry that was the next one to
        // read.
        // Solution here: we loose oldest entry in buffer and do the write.
    if (next_IH_write_head == IH_read_head) {
        IH_read_head = (IH_read_head + 1) & IH_MASK;
    }
    IH_write_head = next_IH_write_head;
    IH_timings[IH_write_head].r = r;
    IH_timings[IH_write_head].d = d;
}

void Track::force_stop_recv() {
#ifdef RF433ANY_DBG_TRACE
    dbg("T> running force_stop_recv()");
#endif
    if (get_trk() == TRK_RECV) {
        track_eat(0, 0);
        track_eat(1, 0);
        do_events();
    }
}

void Track::reset_border_mgmt() {
    count = 0;
    first_low = 0;
    first_high = 0;
    last_low = 0;
}

inline void Track::track_eat(byte r, uint16_t d) {

#ifdef RF433ANY_DBG_TRACE
    dbgf("T> trk = %d, r = %d, d = %u", trk, r, d);
#endif

    if (trk == TRK_WAIT) {
        if (r == 1 && d >= TRACK_MIN_INITSEQ_DURATION) {
            r_low.rreset();
            r_high.rreset();
            prev_r = r;
            rawcode.initseq = d;
            rawcode.max_code_d = d - (d >> 2);
            reset_border_mgmt();
            trk = TRK_RECV;
        }
        return;
    } else if (trk != TRK_RECV) {
        return;
    }

    bool enforce_b_to_false = false;
        // [COMMENT002]
        // We missed an interrupt apparently (two calls with same r), so we
        // had better discard the actual signal.
    if (r == prev_r) {
        enforce_b_to_false = true;
    }
    prev_r = r;

    ++count;
#ifdef RF433ANY_DBG_TRACE
    dbgf("T> count = %d", count);
#endif

    if (count == 1) {
        if ((d < BAND_MIN_D || d >= rawcode.max_code_d)
            && count < TRACK_MIN_BITS && !rawcode.nb_sections) {
#ifdef RF433ANY_DBG_TRACE
            dbg("T> case 1");
#endif
            treset();
                // WARNING
                //   Re-entrant call... not ideal.
            track_eat(r, d);
        } else {
#ifdef RF433ANY_DBG_TRACE
            dbg("T> case 2");
#endif
            first_low = d;
        }
        return;
    } else if (count == 2) {
        if ((d < BAND_MIN_D || d >= rawcode.max_code_d)
            && count < TRACK_MIN_BITS && !rawcode.nb_sections) {
#ifdef RF433ANY_DBG_TRACE
            dbg("T> case 3");
#endif
            treset();
                // WARNING
                //   Re-entrant call... not ideal.
            track_eat(r, d);
        } else {
#ifdef RF433ANY_DBG_TRACE
            dbg("T> case 4");
#endif
            first_high = d;
        }
        return;
    }
#ifdef RF433ANY_DBG_TRACE
    dbg("T> case 5");
#endif

    Rail *prail = (r == 0 ? &r_low : &r_high);
    if (prail->status != RAIL_OPEN)
        return;

    if (r == 0)
        last_low = d;

    bool b;
    if ((d < BAND_MIN_D || d >= rawcode.max_code_d)
        && count < TRACK_MIN_BITS) {
        enforce_b_to_false = true;
    } else if (abs(r_low.index - r_high.index) >= 2) {
        enforce_b_to_false = true;
    } else if (!enforce_b_to_false) {
        b = prail->rail_eat(d);
    }

    if (enforce_b_to_false) {
        r = 1;
        b = false;
    }

    if (r == 1 && (!b || r_low.status != RAIL_OPEN)) {

#ifdef RF433ANY_DBG_TRACE
        dbgf("T> b = %d", b);
#endif

        if (r_low.status == RAIL_OPEN)
            r_low.status = RAIL_CLOSED;
        if (r_high.status == RAIL_OPEN)
            r_high.status = RAIL_CLOSED;

        section_term_status_t sts;
        if (r_low.status == RAIL_FULL && r_high.status == RAIL_FULL) {
            sts = STS_CONTINUED;
        } else if (r_high.status == RAIL_STP_RCVD) {
            if (r_low.status == RAIL_CLOSED || r_low.status == RAIL_FULL
                    || r_low.status == RAIL_ERROR) {
                sts = (r_low.last_bit_recorded ? STS_LONG_SEP : STS_SHORT_SEP);
            } else if (r_low.status == RAIL_STP_RCVD) {
                sts = STS_SEP_SEP;
            } else {
                sts = STS_ERROR;
            }
        } else {
            sts = STS_ERROR;
        }

/*
Tests implemented below reproduce the following decision table

Notations:
  "pr=cont":  the previous track terminated as STS_CONTINUED
  "pr!=cont": the previous track didn't terminate as STS_CONTINUED
  "nbsec": nb_sections
  "cur":   how did current track end? ->
    "sep":   it ended with a separator
    "err":   it ended with an error
    "full":  it didn't end but record is full
   CUR?:  shall we record the current track?
   NEXT?: what to do next? (reset track, start new section)

   FIXME?
     When a section (that is not the first) ends in error, the current section
     is discarded _but_ previous sections are kept and shown to the caller
     (enter 'DATA' state).
     This is questionnable because, why keeping previous section?
     Also it leads to different results depending on recorded_t size, that is an
     internal, intermediate artefact, the nature of which shall not change
     result as seen by caller.
     I leave this behavior though, as a tradeoff between strictness and lax.
     In a possible, future improvement, I might change this behavior, and
     discard previous sections would an error be encountered.

  +---------------+-------- +----------+-------++-------+--------------+
  |nb_bits        | nbsec   | prev     | cur   ||  CUR? | NEXT?        |
  +---------------+-------- +----------+-------++-------+--------------+
  |bits<min_bits  | !nbsec  | n/a      | sep   ||  DISC | RESET        |
  |               |         |          | err   ||  DISC | RESET        |
  |               |         |          | full  ||  n/a  | n/a          |
  |               | nbsec>0 | pr=cont  | sep   ||  REC  | NEWSEC       |
  |               |         |          | err   ||  DISC | DATA         |
  |               |         |          | full  ||  n/a  | n/a          |
  |               | nbsec>0 | pr!=cont | sep   ||  DISC | DATA         |
  |               |         |          | err   ||  DISC | DATA         |
  |               |         |          | full  ||  n/a  | n/a          |
  |bits>=min_bits | !nbsec  | n/a      | sep   ||  REC  | NEWSEC       |
  |               |         |          | err   ||  DISC | RESET        |
  |               |         |          | ful   ||  REC  | NEWSEC(CONT) |
  |               | nbsec>0 | pr=cont  | sep   ||  REC  | NEWSEC       |
  |               |         |          | err   ||  DISC | DATA         |
  |               |         |          | ful   ||  REC  | NEWSEC(CONT) |
  |               | nbsec>0 | pr!=cont | sep   ||  REC  | NEWSEC       |
  |               |         |          | err   ||  DISC | DATA         |
  |               |         |          | ful   ||  REC  | NEWSEC(CONT) |
  +---------------+-------- +----------+-------++-------+--------------+
*/

        bool record_current_section;

#ifdef RF433ANY_DBG_TRACK
        bool do_track_debug = false;
        (void)do_track_debug;
#endif

        if (r_low.index < TRACK_MIN_BITS || r_high.index < TRACK_MIN_BITS) {
            record_current_section =
                (sts != STS_ERROR
                 && rawcode.nb_sections
                 && rawcode.sections[rawcode.nb_sections - 1].sts
                    == STS_CONTINUED);

#ifdef RF433ANY_DBG_TRACK
            do_track_debug = record_current_section;
#endif

        } else {
            record_current_section = (sts != STS_ERROR);

#ifdef RF433ANY_DBG_TRACK
            do_track_debug = true;
#endif

        }

#ifdef RF433ANY_DBG_TRACE
        dbgf("T> reccursec=%i, sts=%i", record_current_section, sts);
#endif
#if defined(RF433ANY_DBG_SIMULATE) && defined(RF433ANY_DBG_TRACK)
        if (do_track_debug) {
            dbgf("%s  {", counter >= 2 ? ",\n" : "");
            dbgf("    \"N\":%d,\"start\":%u,\"end\":%u,",
                sim_timings_count, sim_int_count_svg, sim_int_count - 1);
            track_debug();
            dbg("  }");
        }
#endif

        if (record_current_section) {
#ifdef RF433ANY_DBG_TRACE
            dbg("T> recording current section");
#endif
            Section *psec = &rawcode.sections[rawcode.nb_sections++];
            psec->sts = sts;

            psec->ts.sep = (sts == STS_SHORT_SEP
                           || sts == STS_LONG_SEP
                           || sts == STS_SEP_SEP ? d : 0);
            if (r_low.b_short.test_value(r_high.b_short.mid)
                    && !r_low.b_short.test_value(r_high.b_long.mid)
                    && !r_low.b_long.test_value(r_high.b_short.mid)
                    && r_low.b_long.test_value(r_high.b_long.mid)) {
                psec->ts.low_short = (r_low.b_short.mid + r_high.b_short.mid)
                                    >> 1;
                psec->ts.low_long = (r_low.b_long.mid + r_high.b_long.mid)
                                    >> 1;
                psec->ts.high_short = 0;
                psec->ts.high_long = 0;
            } else {
                psec->ts.low_short = r_low.b_short.mid;
                psec->ts.low_long = r_low.b_long.mid;
                psec->ts.high_short = r_high.b_short.mid;
                psec->ts.high_long = r_high.b_long.mid;
            }

            psec->low_rec = r_low.rec;
            psec->low_bits = r_low.index;
            psec->low_bands = r_low.get_band_count();
            psec->high_rec = r_high.rec;
            psec->high_bits = r_high.index;
            psec->high_bands = r_high.get_band_count();

            psec->first_low = first_low;
            psec->first_high = first_high;
            psec->last_low = last_low;

            trk = ((rawcode.nb_sections == RF433ANY_MAX_SECTIONS)
                    ? TRK_DATA : TRK_RECV);

#ifdef RF433ANY_DBG_TRACE
            dbgf("T> rawcode.nb_sections = %d", rawcode.nb_sections);
#endif

            if (trk == TRK_RECV) {
#ifdef RF433ANY_DBG_TRACE
                dbg("T> keep receiving (soft reset)");
#endif
                r_low.rreset_soft();
                r_high.rreset_soft();
                if (sts != STS_CONTINUED) {
                    reset_border_mgmt();
                }
            } else {
#ifdef RF433ANY_DBG_TRACE
                dbg("T> stop receiving (data)");
#endif
            }
        } else {
            if (rawcode.nb_sections) {
                trk = TRK_DATA;
            } else {
                treset();
                    // WARNING
                    //   Re-entrant call... not ideal.
                track_eat(r, d);
            }
        }

    }
}

    // Returns true if a timing got processed, false otherwise.
    // Do nothing (and returns false) if Track is in the status TRK_DATA.
    // NOTE
    //   When Track is in the TRK_DATA state, no erase can happen
    //   (track_eat() will exit immediately).
    //   Therefore the safeguard of explicitly doing nothing if in the status
    //   TRK_DATA is redundant => it is defensive programming.
bool Track::process_interrupt_timing() {
    if (get_trk() == TRK_DATA)
        return false;

    unsigned char IH_pending_timings =
        (IH_write_head - IH_read_head) & IH_MASK;
    if (IH_pending_timings > IH_max_pending_timings)
        IH_max_pending_timings = IH_pending_timings;

    bool ret;

    cli();
    if (IH_read_head != IH_write_head) {
        IH_timing_t timing = IH_timings[IH_read_head];
        IH_read_head = (IH_read_head + 1) & IH_MASK;

        sei();
#ifdef RF433ANY_DBG_TIMINGS
        unsigned long t0 = micros();
#endif
        track_eat(timing.r, timing.d);
#ifdef RF433ANY_DBG_TIMINGS
        unsigned long d = micros() - t0;
        if (d > RF433ANY_MAX_DURATION)
            d = RF433ANY_MAX_DURATION;
        ih_dbg_exec[ih_dbg_pos] = d;
        if (get_trk() == TRK_WAIT)
            ih_dbg_pos = 0;
        else {
            if (ih_dbg_pos < sizeof(ih_dbg_timings) / sizeof(*ih_dbg_timings))
                ih_dbg_timings[ih_dbg_pos++] = timing.d;
        }
#endif

        ret = true;

    } else {

        sei();
        ret = false;
    }

    return ret;
}

void Track::activate_recording() {
#ifndef RF433ANY_DBG_SIMULATE
    if (!IH_interrupt_handler_is_attached) {
        attachInterrupt(digitalPinToInterrupt(pin_number), &ih_handle_interrupt,
                CHANGE);
        IH_interrupt_handler_is_attached = true;
    }
#endif
}

void Track::deactivate_recording() {
#ifndef RF433ANY_DBG_SIMULATE
    if (IH_interrupt_handler_is_attached) {
        detachInterrupt(digitalPinToInterrupt(pin_number));
        IH_interrupt_handler_is_attached = false;
    }
#endif
}

bool Track::do_events() {
    activate_recording();
    while (process_interrupt_timing())
        ;
    if (get_trk() == TRK_DATA) {
        deactivate_recording();
#ifdef RF433ANY_DBG_RAWCODE
        dbgf("IH_max_pending_timings = %d", ih_get_max_pending_timings());
        rawcode.debug_rawcode();
#endif
        return true;
    }
    return false;
}

void Track::ih_handle_interrupt_wait_free() {
    static unsigned long last_t = 0;

    const unsigned long t = micros();
    unsigned long d = t - last_t;
    last_t = t;

    if (d > RF433ANY_MAX_DURATION)
        d = RF433ANY_MAX_DURATION;

    short new_bit = (d >= 200 && d <= 25000);
    short old_bit = !!(IH_wait_free_last16 & 0x8000);
    IH_wait_free_last16 <<= 1;
    IH_wait_free_last16 |= new_bit;

    IH_wait_free_count_ok += new_bit;
    IH_wait_free_count_ok -= old_bit;
}

void Track::wait_free_433() {
    if (IH_interrupt_handler_is_attached)
        return;

    IH_wait_free_last16 = (uint16_t)0xffff;
    IH_wait_free_count_ok = 16;

    attachInterrupt(digitalPinToInterrupt(pin_number),
            &ih_handle_interrupt_wait_free, CHANGE);

        // 75% of the last 16 durations must be in the interval [200, 25000]
        // (that is, 12 out of 16).
    while (IH_wait_free_count_ok >= 12)
        ;

    detachInterrupt(digitalPinToInterrupt(pin_number));
}

Decoder* Track::get_data_core(byte convention) {
    Decoder *pdec_head = nullptr;
    Decoder *pdec_tail = nullptr;
    Decoder *pdec = nullptr;

    for (byte i = 0; i < rawcode.nb_sections; ++i) {

        const Section *psec = &rawcode.sections[i];

        if (abs(psec->low_bits - psec->high_bits) >= 2) {
                // Defensive programming (should never happen).
            if (!pdec) {
                pdec = new DecoderRawInconsistent();
            }

        } else if (psec->low_bands == 1 && psec->high_bands == 1) {
            byte n = (psec->low_bits < psec->high_bits ?
                      psec->low_bits : psec->high_bits);
            if (pdec) {
                pdec->add_sync(n);
            } else {
                pdec = new DecoderRawSync(n);
                pdec->take_into_account_first_low_high(psec, false);
            }

        } else if (psec->low_bands == 1 || psec->high_bands == 1) {
            if (!pdec) {
                pdec = new DecoderRawInconsistent();
            }

        } else {
            byte enum_decoders = RF433ANY_ID_START;
            bool is_continuation_of_prev_section = pdec;
            do {
                if (!pdec)
                    pdec = Decoder::build_decoder(enum_decoders, convention);

                pdec->decode_section(psec, is_continuation_of_prev_section);

                if (!is_continuation_of_prev_section && pdec->get_nb_errors()) {
                    delete pdec;
                    pdec = nullptr;
                }
            } while (!pdec && ++enum_decoders <= RF433ANY_ID_END);

        }
            // The last enumerated decoder is DecoderRawUnknownCoding, that
            // never produces any error and MUST be chosen in the end (if no
            // other worked).
        assert(pdec);

        pdec->set_ts((pdec_head ? 0 : rawcode.initseq), psec->ts);

        if (psec->sts != STS_CONTINUED || i == rawcode.nb_sections - 1) {
            if (!pdec_head) {
                assert(!pdec_tail);
                pdec_head = pdec;
                pdec_tail = pdec;
            } else {
                assert(pdec_tail);
                pdec_tail->attach(pdec);
                pdec_tail = pdec;
            }
            pdec = nullptr;
        }
    }

    return pdec_head;
}

Decoder* Track::get_data(uint16_t filter, byte convention) {
    Decoder *pdec0 = get_data_core(convention);
    Decoder *prev_pdec = pdec0;
    Decoder *pdec = pdec0;

    while (pdec) {
        pdec->reset_repeats();

        bool keep = true;

        if (filter & RF433ANY_FD_DECODED) {
                // Defensive programming
                //   Normally if data_got_decoded() is true then pdata is non
                //   null and pdata->get_nb_bits() is non-zero.
            if (!pdec->data_got_decoded()
                    || !pdec->get_pdata()
                    || !pdec->get_pdata()->get_nb_bits())
                keep = false;
        }

        if (filter & RF433ANY_FD_NO_ERROR) {
            if (pdec->get_nb_errors())
                keep = false;
        }

        if (filter & RF433ANY_FD_DEDUP) {
            if (pdec != prev_pdec && pdec->get_id() == prev_pdec->get_id()) {
                const BitVector *p1;
                const BitVector *p2;
                if ((p1 = pdec->get_pdata()) && (p2 = prev_pdec->get_pdata())) {
                    if (!p1->cmp(p2)) {
                        keep = false;
                        prev_pdec->inc_repeats();
                    }
                }
            }
        }

        if (filter & (RF433ANY_FD_TRI | RF433ANY_FD_TRN | RF433ANY_FD_MAN)) {
            if (!(filter & RF433ANY_FD_TRI)
                    && pdec->get_id() == RF433ANY_ID_TRIBIT)
                keep = false;
            if (!(filter & RF433ANY_FD_TRN)
                    && pdec->get_id() == RF433ANY_ID_TRIBIT_INV)
                keep = false;
            if (!(filter & RF433ANY_FD_MAN)
                    && pdec->get_id() == RF433ANY_ID_MANCHESTER)
                keep = false;
        }

        if (keep) {
            prev_pdec = pdec;
            pdec = pdec->get_next();
        } else {
            Decoder *pdec_to_remove = pdec;
            if (pdec == pdec0) {
                assert(pdec0 == prev_pdec);
                pdec0 = pdec->get_next();
                prev_pdec = pdec0;
                pdec = pdec0;
            } else {
                pdec = pdec->get_next();
                prev_pdec->detach();
                prev_pdec->attach(pdec);
            }
            pdec_to_remove->detach();
            delete pdec_to_remove;
        }
    }

    return pdec0;
}

#ifdef RF433ANY_DBG_TIMINGS
void Track::dbg_timings() const {
    for (unsigned int i = 0; i + 1 < ih_dbg_pos; i += 2) {
        dbgf("%4u, %4u  |  %5u, %5u", ih_dbg_timings[i], ih_dbg_timings[i + 1],
             ih_dbg_exec[i], ih_dbg_exec[i + 1]);
    }
}
#endif

#ifdef RF433ANY_DBG_TRACK
const char* trk_names[] = {
    "TRK_WAIT",
    "TRK_RECV",
    "TRK_DATA"
};
void Track::track_debug() const {
    recorded_t xorval = r_low.rec ^ r_high.rec;
    dbgf("    \"trk\":%s,\"xorval\":0x" FMTRECORDEDT ",",
         trk_names[trk], xorval);
    if (trk != TRK_WAIT) {
        for (byte i = 0; i < 2; ++i) {
            dbgf("    \"%s\":{", (i == 0 ? "r_low" : "r_high"));
            (i == 0 ? &r_low : &r_high)->rail_debug();
            dbgf("    }%s", i == 1 ? "" : ",");
        }
    }

}
#endif

// vim: ts=4:sw=4:tw=80:et
