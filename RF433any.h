// RF433any.h

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

#ifndef _RF433ANY_H
#define _RF433ANY_H

// ****************************************************************************
// RF433ANY_TESTPLAN **********************************************************
#if RF433ANY_TESTPLAN == 1

#define RF433ANY_DBG_SIMULATE
#define RF433ANY_DBG_TRACK

#elif RF433ANY_TESTPLAN == 2 // RF433ANY_TESTPLAN

#define RF433ANY_DBG_SIMULATE
#define RF433ANY_DBG_RAWCODE

#elif RF433ANY_TESTPLAN == 3 // RF433ANY_TESTPLAN

#define RF433ANY_DBG_SIMULATE
#define RF433ANY_DBG_DECODER

#elif RF433ANY_TESTPLAN == 4 // RF433ANY_TESTPLAN

#define RF433ANY_DBG_SIMULATE
#define RF433ANY_DBG_DECODER
#define RF433ANY_DBG_SMALL_RECORDED
#define RF433ANY_MAX_SECTIONS 12

#elif RF433ANY_TESTPLAN == 5 // RF433ANY_TESTPLAN

#define RF433ANY_DBG_SIMULATE
#define RF433ANY_DBG_SMALL_RECORDED

#else // RF433ANY_TESTPLAN

#ifdef RF433ANY_TESTPLAN
#error "RF433ANY_TESTPLAN macro has an illegal value."
#endif
// RF433ANY_TESTPLAN **********************************************************
// ****************************************************************************


// It is OK to update the below, because if this code is compiled, then we are
// not in the test plan.

//#define RF433ANY_DBG_SIMULATE
//#define RF433ANY_DBG_TRACE
//#define RF433ANY_DBG_TIMINGS
//#define RF433ANY_DBG_TRACK
//#define RF433ANY_DBG_RAWCODE
//#define RF433ANY_DBG_DECODER
//#define RF433ANY_DBG_SMALL_RECORDED

#endif // RF433ANY_TESTPLAN

#if defined(RF433ANY_DBG_SIMULATE) || defined(RF433ANY_DBG_TRACE) \
    || defined(RF433ANY_DBG_TIMINGS) || defined(RF433ANY_DBG_TRACK) \
    || defined(RF433ANY_DBG_RAWCODE) || defined(RF433ANY_DBG_DECODER)
#define DEBUG
#endif

#ifdef RF433ANY_DBG_SIMULATE
#include "Serial.h"
#define SIM_TIMINGS_LEN 140
#endif

#ifdef DEBUG

#include "Debug.h"

#else

#define dbg(a)
#define dbgf(...)

#endif

#include <Arduino.h>

#define RF433ANY_MAX_DURATION     65535
#define RF433ANY_MAX_SEP_DURATION 65535
#ifndef RF433ANY_MAX_SECTIONS
#define RF433ANY_MAX_SECTIONS     8
#endif


// * **** *********************************************************************
// * Band *********************************************************************
// * **** *********************************************************************

#define BAND_MIN_D    64
    // IMPORTANT
    //   Value must be so that BAND_MAX_D * 2 can be stored in a uint16_t.
    //   That means BAND_MAX_D must be lower than 32768.
#define BAND_MAX_D 30000

struct Band {
    uint16_t inf;
    uint16_t mid;
    uint16_t sup;

    bool got_it;

    bool test_value_init_if_needed(uint16_t d);
    bool test_value(uint16_t d);

    void breset();
    bool init(uint16_t d);
    bool init_sep(uint16_t d);
};


// * **** *********************************************************************
// * Rail *********************************************************************
// * **** *********************************************************************

#ifdef RF433ANY_DBG_SIMULATE

#ifdef RF433ANY_DBG_SMALL_RECORDED

typedef uint8_t recorded_t;
#define FMTRECORDEDT "%02X"

#else

typedef uint32_t recorded_t;
#define FMTRECORDEDT "%08lX"

#endif

#else // RF433ANY_DBG_SIMULATE

typedef uint16_t recorded_t;
#define FMTRECORDEDT "%04lx"

#endif

#define RAIL_MOOD_STRICT 0
#define RAIL_MOOD_LAXIST 1

#define DEFAULT_RAIL_MOOD RAIL_MOOD_LAXIST

#define RAIL_OPEN     0
#define RAIL_FULL     1
#define RAIL_STP_RCVD 2
#define RAIL_CLOSED   3
#define RAIL_ERROR    4

class Rail {
    friend class Track;

    private:
        Band b_short;
        Band b_long;
        Band b_sep;

        byte last_bit_recorded;
        recorded_t rec;
        byte status;
        byte index;

        byte mood;

    public:
        Rail(byte arg_mood);
        bool rail_eat(uint16_t d);
        void rreset();
        void rreset_soft();
#ifdef RF433ANY_DBG_TRACK
        void rail_debug() const;
#endif
        byte get_band_count() const;
};


// * **** *********************************************************************
// * Misc *********************************************************************
// * **** *********************************************************************

typedef enum {
    STS_CONTINUED,
    STS_X_SEP, // FIXME
    STS_SHORT_SEP,
    STS_LONG_SEP,
    STS_SEP_SEP,
    STS_ERROR
} section_term_status_t;

struct Timings {
    uint16_t low_short;
    uint16_t low_long;
    uint16_t high_short;
    uint16_t high_long;
    uint16_t sep;
};

struct TimingsExt: public Timings {
    uint16_t initseq;
    uint16_t first_low;
    uint16_t first_high;
    uint16_t first_low_ignored;
    uint16_t last_low;
};

struct Section {
    recorded_t low_rec;
    unsigned char low_bits   :6;
    unsigned char low_bands  :2;
    recorded_t high_rec;
    unsigned char high_bits  :6;
    unsigned char high_bands :2;

    uint16_t first_low;
    uint16_t first_high;
    uint16_t last_low;

    Timings ts;

    section_term_status_t sts;
};

struct RawCode {
    uint16_t initseq;
    uint16_t max_code_d;
    byte nb_sections;
    Section sections[RF433ANY_MAX_SECTIONS];

    void debug_rawcode() const;
};


// * ********* ****************************************************************
// * BitVector ****************************************************************
// * ********* ****************************************************************

// vector-like of the (very) poor man. No time to make it fancier.
// It'll simply accept to add a bit at the beginning (add_bit),
// to get the number of bits and bytes, and access the Nth bit or byte.
// Also, an iterator would be best to walk through bits, but it is 'TO DO' for
// now.
class BitVector {
    private:
        uint8_t* array;
        byte allocated;
        byte nb_bits;
    public:
        BitVector();
        virtual ~BitVector();

        virtual void add_bit(byte v);

        virtual int get_nb_bits() const;
        virtual byte get_nb_bytes() const;
        virtual byte get_nth_bit(byte n) const;
        virtual byte get_nth_byte(byte n) const;

        virtual char *to_str() const;
        virtual short cmp(const BitVector *p) const;
};


// * ******* ******************************************************************
// * Decoder ******************************************************************
// * ******* ******************************************************************

    // IMPORTANT
    //   VALUES ARE NOT ARBITRARY.
    //   RF433ANY_CONV0 must be 0 and RF433ANY_CONV1 must be 1.
    //   This is due to the decoding that uses a bit value ultimately coming
    //   from RF433ANY_CONV0 or RF433ANY_CONV1.
#define RF433ANY_CONV0 0
#define RF433ANY_CONV1 1

enum class Signal {
    SHORT,
    LONG,
    OTHER
};

    // FD = Filter Data
    // Bit-mask values, to be used in conjunction
#define RF433ANY_FD_ALL       0
#define RF433ANY_FD_DECODED   1
#define RF433ANY_FD_NO_ERROR  2
#define RF433ANY_FD_DEDUP     4
#define RF433ANY_FD_TRI       8
#define RF433ANY_FD_TRN      16
#define RF433ANY_FD_MAN      32

#define RF433ANY_ID_RAW_INCONSISTENT   0
#define RF433ANY_ID_START              1 // Start enumeration of real decoders
#define RF433ANY_ID_RAW_SYNC           1
#define RF433ANY_ID_TRIBIT             2
#define RF433ANY_ID_TRIBIT_INV         3
#define RF433ANY_ID_MANCHESTER         4
#define RF433ANY_ID_RAW_UNKNOWN_CODING 5 // At last we use this one, that'll
                                         // always produce a successful result.
#define RF433ANY_ID_END                5 // End of enumeration of real decoders

class Decoder {
    private:
        Decoder *next;
        byte repeats;

    protected:
        BitVector* pdata;
        byte convention;
        byte nb_errors;

        TimingsExt tsext;

        void add_data_bit(byte valbit);
        virtual void add_signal_step(Signal low, Signal high) = 0;

    public:
        Decoder(byte arg_convention);
        virtual ~Decoder();
        virtual byte get_id() const = 0;
        virtual char get_id_letter() const = 0;

        static Decoder *build_decoder(byte id, byte convention);

        virtual void add_sync(byte n) { }
        virtual byte get_nb_errors() const;
        virtual int get_nb_bits() const;

        virtual void get_tsext(TimingsExt *p_tsext) const;
        virtual void set_ts(const uint16_t& arg_initseq, const Timings& arg_ts);
        virtual void decode_section(const Section *psec,
                                    bool is_cont_of_prev_sec);
        virtual void take_into_account_first_low_high(const Section *psec,
                                    bool is_cont_of_prev_sec);
        virtual uint16_t first_lo_ignored() const;

        virtual void attach(Decoder *pdec);
        virtual void detach();

        virtual bool data_got_decoded() const { return false; }
        virtual const BitVector* get_pdata() const;
        virtual BitVector* take_away_data();
        virtual Decoder* get_next() const { return next; }

        virtual void reset_repeats() { repeats = 0; }
        virtual void inc_repeats() { ++repeats; }
        virtual byte get_repeats() const { return repeats; };

#ifdef RF433ANY_DBG_DECODER
        virtual void dbg_data(byte seq) const;
        virtual void dbg_meta(byte disp_level) const;
        virtual void dbg_decoder(byte disp_level = 1, byte seq = 0) const
                = 0;
        virtual void dbg_next(byte disp_level, byte seq) const;
#endif
};


// * ********************** ***************************************************
// * DecoderRawInconsistent ***************************************************
// * ********************** ***************************************************

class DecoderRawInconsistent: public Decoder {
    public:
        DecoderRawInconsistent(): Decoder(RF433ANY_CONV0) { }
        ~DecoderRawInconsistent() { }

        virtual byte get_id() const override {
            return RF433ANY_ID_RAW_INCONSISTENT;
        }
        virtual char get_id_letter() const override { return 'I'; }

        virtual void add_signal_step(Signal lo, Signal hi) override { }

#ifdef RF433ANY_DBG_DECODER
        virtual void dbg_decoder(byte disp_level, byte seq) const override;
#endif
};


// * ************** ***********************************************************
// * DecoderRawSync ***********************************************************
// * ************** ***********************************************************

class DecoderRawSync: public Decoder {
    private:
        byte nb_low_high;
        Signal sync_shape;
        bool sync_shape_set;

    public:
        DecoderRawSync(byte arg_nb_low_high):
                Decoder(RF433ANY_CONV0),
                nb_low_high(arg_nb_low_high),
                sync_shape_set(false) { }
        ~DecoderRawSync() { }

        virtual byte get_id() const override { return RF433ANY_ID_RAW_SYNC; }
        virtual char get_id_letter() const override { return 'S'; }

        virtual void add_signal_step(Signal lo, Signal hi) override;

        virtual void add_sync(byte n) override;

        virtual int get_nb_bits() const override;

#ifdef RF433ANY_DBG_DECODER
        virtual void dbg_decoder(byte disp_level, byte seq) const override;
#endif

};


// * *********************** **************************************************
// * DecoderRawUnknownCoding **************************************************
// * *********************** **************************************************

class DecoderRawUnknownCoding: public Decoder {
    private:
        Signal unused_final_low;
        bool terminates_with_sep;

    public:
        DecoderRawUnknownCoding():
                Decoder(RF433ANY_CONV0),
                unused_final_low(Signal::OTHER),
                terminates_with_sep(false) { }
        ~DecoderRawUnknownCoding() { }

        virtual byte get_id() const override
            { return RF433ANY_ID_RAW_UNKNOWN_CODING; }
        virtual char get_id_letter() const override { return 'U'; }

        virtual void add_signal_step(Signal lo, Signal hi) override;

#ifdef RF433ANY_DBG_DECODER
        virtual void dbg_decoder(byte disp_level, byte seq) const override;
#endif

};


// * ************* ************************************************************
// * DecoderTriBit ************************************************************
// * ************* ************************************************************

class DecoderTriBit: public Decoder {
    public:
        DecoderTriBit(byte arg_convention = RF433ANY_CONV0)
                :Decoder(arg_convention) {
        }
        ~DecoderTriBit() { }

        virtual byte get_id() const override { return RF433ANY_ID_TRIBIT; }
        virtual char get_id_letter() const override { return 'T'; }
        virtual void add_signal_step(Signal low, Signal high)
            override;

        virtual bool data_got_decoded() const override {
            return pdata && pdata->get_nb_bits();
        }

#ifdef RF433ANY_DBG_DECODER
        virtual void dbg_decoder(byte disp_level, byte seq) const override;
#endif

};


// * **************** *********************************************************
// * DecoderTriBitInv *********************************************************
// * **************** *********************************************************

class DecoderTriBitInv: public Decoder {
    private:
        bool first_call_to_add_sgn_lo_hi;
        Signal unused_initial_low;
        Signal last_hi;

    public:
        DecoderTriBitInv(byte arg_convention = RF433ANY_CONV0)
                :Decoder(arg_convention),
                first_call_to_add_sgn_lo_hi(true),
                unused_initial_low(Signal::OTHER) {
        }
        ~DecoderTriBitInv() { }

        virtual byte get_id() const override { return RF433ANY_ID_TRIBIT_INV; }
        virtual char get_id_letter() const override { return 'N'; }
        virtual void add_signal_step(Signal low, Signal high)
            override;

        virtual bool data_got_decoded() const override {
            return pdata && pdata->get_nb_bits();
        }

        virtual uint16_t first_lo_ignored() const override;

#ifdef RF433ANY_DBG_DECODER
        virtual void dbg_decoder(byte disp_level, byte seq) const override;
#endif

};


// * ***************** ********************************************************
// * DecoderManchester ********************************************************
// * ***************** ********************************************************

class DecoderManchester: public Decoder {
    private:
        byte buf[3];
        byte buf_pos;
            // Manchester encoding comes with a mandatory leading 'short low'
            // (otherwise we could not distinguish it from the initialization
            // sequence).
            // Said differently: Manchester needs a leading '0' bit (if
            // considering low-then-high is '0'), that is not part of data.
        bool leading_lo_hi_has_been_passed;

        void add_buf(byte r);
        void consume_buf();

    public:
        DecoderManchester(byte arg_convention = RF433ANY_CONV0);
        ~DecoderManchester() { }

        virtual byte get_id() const override { return RF433ANY_ID_MANCHESTER; }
        virtual char get_id_letter() const override { return 'M'; }
        virtual void add_signal_step(Signal low, Signal high)
            override;

        virtual bool data_got_decoded() const override {
            return pdata && pdata->get_nb_bits();
        }

#ifdef RF433ANY_DBG_DECODER
        virtual void dbg_decoder(byte disp_level, byte seq) const override;
#endif

};


// * ***** ********************************************************************
// * Track ********************************************************************
// * ***** ********************************************************************

#define TRACK_MIN_INITSEQ_DURATION 4000
#define TRACK_MIN_BITS             7

    // IMPORTANT
    //   IH_MASK must be equal to the size of IH_timings - 1.
    //   The size of IH_timings must be a power of 2.
    //   Thus, IH_MASK allows to quickly calculate modulo, while walking through
    //   IH_timings.
#define IH_SIZE 4
#define IH_MASK (IH_SIZE - 1)

struct IH_timing_t {
    byte r;
    uint16_t d;

    IH_timing_t() { }
    IH_timing_t(const volatile IH_timing_t& t) {
        r = t.r;
        d = t.d;
    }
};

// NOTE - ABOUT STATIC MEMBER VARIABLES AND FUNCTIONS IN THE TRACK CLASS
//   The class is designed so that one object is useful at a time. This comes
//   from the fact that we attach interrupt handler to a static method (as is
//   mandatory: an object method would not be possible, compiler would block
//   because no way to populate 'this' pointer.)
//   At last, the distinction between static and non-static members is a bit
//   arbitrary.
//   I decided that variables and functions _directly_ tied to interrupt handler
//   are static, while all others are non-static.
typedef enum {TRK_WAIT, TRK_RECV, TRK_DATA} trk_t;
class Track {
    private:


#ifdef RF433ANY_DBG_TIMINGS
        static uint16_t ih_dbg_timings[40];
        static uint16_t ih_dbg_exec[40];
        static unsigned int ih_dbg_pos;
#endif
        static byte pin_number;
        static volatile IH_timing_t IH_timings[IH_SIZE];
        static volatile unsigned char IH_write_head;
        static volatile unsigned char IH_read_head;
        static byte IH_max_pending_timings;
        static bool IH_interrupt_handler_is_attached;

        volatile trk_t trk;
        byte count;

        Rail r_low;
        Rail r_high;
        byte prev_r;

        uint16_t first_low;
        uint16_t first_high;
        uint16_t last_low;

        RawCode rawcode;

        void reset_border_mgmt();
        Decoder* get_data_core(byte convention);

    public:
        Track(int arg_pin_number, byte mood = DEFAULT_RAIL_MOOD);

        static void ih_handle_interrupt();
        static byte ih_get_max_pending_timings() {
            return IH_max_pending_timings;
        }

        void treset();
        void track_eat(byte r, uint16_t d);
#ifdef RF433ANY_DBG_TRACK
        void track_debug() const;
#endif
#ifdef RF433ANY_DBG_TIMINGS
        void dbg_timings() const;
#endif

        trk_t get_trk() const { return trk; }

        void force_stop_recv();

        void activate_recording();
        void deactivate_recording();
        bool process_interrupt_timing();
        bool do_events();
        Decoder* get_data(uint16_t filter, byte convention = RF433ANY_CONV0);
};

#endif // _RF433ANY_H

// vim: ts=4:sw=4:tw=80:et
