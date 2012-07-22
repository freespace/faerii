/* rgb LED is controlled by control blocks residing in EEPROM.
 *
 * Control blocks are encoded in groups of 4 bytes. The bytes contain, in
 * order, values for:
 *
 *   red, green, blue, duration
 *
 * red, green, blue are unsigned. This represents the red, green or blue
 * intensity.
 *
 * duration also unsigned and specified a time interval, and has units of
 * 2*MS_PER_TICK. Not all durations are valid. See below. Furthermore,
 * because colour delta per unit duration is stored as integers, we can not
 * satisfy all transition requirements, namely those which require
 * intensity change per unit duration is less than 1. e.g. going from 0 to 10
 * over 200 unit duration. When this is encountered, the value is immediately
 * set to the average of the current intensity and the target intensity
 *
 * Each control block specifies the colour to transition to, and how
 * long that transition should take.
 *
 * The first control block is special. The first 3 bytes must have values of:
 *
 *   0xfa 0xe2 0x11
 * The 4th byte specifies special options. These optons are:
 *
 *  - RGB_REVERSE
 *  - RGB_RANDOM_ON_READ
 *
 * RGB_REVERSE when set will cause the block to run backwards when it
 * reaches the end.
 *
 * RGB_RANDOM_ON_READ will modify the intensity values after reading them so
 * next time it is read, the values will be different.
 *
 * Control blocks are read until a delimiter block is encountered, or until the
 * end of EEPROM. At this point, the entire block will repeat again starting
 * from the start of EEPROM, skipping the setup block, or the nearest delimiter
 * block.  If RGB_REVERSE is set, then then block is repeated in reverse
 * instead of from the beginning.
 *
 * e.g. given
 *
 *   d1 c1 c2 c3 c4 d2
 *
 * When d2 (delimiter 2) is reached, the block (c1, c2, c3, c4) will repeat
 * starting from d1, or go reverse.
 *
 * Delimiter block
 * ==================
 * The delimiter block is marked by its duration of 0xff. When it is
 * encountered, the block either restarts or reverses. The red, green and
 * blue value of the delimiter block is not used.
 *
 * Delimter blocks allow discrete sequences to be stored in EEPROM and
 * recalled at need. This minimises the number of required EEPROM writes.
 *
 * Timer1 and TIMER1_COMPB is used.
 */

#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include "config.h"
#include "types.h"
#include "ctrBlock.h"

#include "rgb.h"

// our internal time keeping ticks over once every MS_PER_TICK. Note that
// CYCLE_PER_TICK is equal to MS_PER_TICK _only_ for a timer running close
// to 1KHz.
#define CYCLE_PER_TICK        (10)
#define MS_PER_TICK           (10)

// it is unlikely this will change, but
// this makes parts of the code clearer
#define MS_PER_SEC            (1000)

#define MS_PER_UNIT_DURATION  (2*MS_PER_TICK)

#define _CONCAT(a,b)          (a ## b)
#define _IO_PORT(name)        _CONCAT(PORT, name)
#define IO_PORT               _IO_PORT(PORTNAME)
#define _DD_REG(name)        _CONCAT(DDR,name)
#define DD_REG               _DD_REG(PORTNAME)

#ifndef COMMON_ANODE_LED
#define LED_ON(bitpos)        (IO_PORT |= _BV(bitpos))
#define LED_OFF(bitpos)       (IO_PORT &= ~_BV(bitpos))
#else
#define LED_ON(bitpos)        (IO_PORT &= ~_BV(bitpos))
#define LED_OFF(bitpos)       (IO_PORT |= _BV(bitpos))
#endif

volatile ElapsedTime _elapsedTime;
volatile uint8 _error;

ISR(BADISR_vect) {
  _error = 1;
}

ISR(TIMER1_COMPB_vect) {
  _elapsedTime.ms += MS_PER_TICK;
  if (_elapsedTime.ms == MS_PER_SEC) {
    _elapsedTime.ms = 0;
    _elapsedTime.sec += 1;
  }
}

uint16 msSince(uint16 thepast) {
  if (_elapsedTime.ms>=thepast) return _elapsedTime.ms - thepast;
  else {
    // we have wrapped around
    return MS_PER_SEC - thepast + _elapsedTime.ms;
  }
}

void pwm(phase, intensity, bitpos) {
  if (phase < intensity) LED_ON(bitpos);
  else LED_OFF(bitpos);
}

int16 calcColorDelta(uint8 from, uint8 to, uint8 duration) {
  int16 d;
  if (from > to) d = -(from - to)/duration;
  else d = (to - from)/duration;

  // the +/-1 is so we over shoot when we have very small
  // delta steps. 1.2, 1.4 will truncate to 1, causing
  // undershoot. Aesthetically, it seems better to overshoot.
  // Large d is, smaller the effect this modification has.
  if (d) {
    if (d>0) d += 1;
    else d -= 1;
  }

  return d;
}

uint8 adjIntensity(uint8 i, int16 di) {
  int16 ii = i + di;
  if (ii > UINT8_MAX) ii = UINT8_MAX;
  else if (ii < 0) ii = 0;
  return ii;
}

uint8 _r, _g, _b;
uint8 _duration;

void copyColors(ControlBlock *cb) {
  _r = cb->r;
  _g = cb->g;
  _b = cb->b;
}

void rgbSetup() {
  _error = 0;
  ControlBlock *cb = ctrBlockSetup();
  if (cb) {
    _r = cb->r;
    _g = cb->g;
    _b = cb->b;
    _duration = cb->duration;
  } else _error = 1;

  // Setup timer1 to use system block divded by 16384.
  // This gives us
  // >>> 16500000/16384
  // 1007.080078125 ticks per second.
  TCCR1 = _BV(CS13) | _BV(CS12) | _BV(CS11) | _BV(CS10);

  // lets interrupt once every CYCLE_PER_TICK cycles
  OCR1B = CYCLE_PER_TICK;

  // clear TCNT1 whenever it hits OCR1B
  OCR1C = OCR1B;
  TCCR1 |= _BV(CTC1);

  // enable compare interrupt
  TIMSK = _BV(OCIE1B);

  // setup the rgb pins
  DD_REG = _BV(PIN_R) | _BV(PIN_G) | _BV(PIN_B);

  if (_error == 0) IO_PORT = 0;
}

int16 _dr, _dg, _db;
uint8 _pollCounter;
uint16 _lastms;

void rgbPoll() {
  if (_error) {
    LED_ON(PIN_R);
    return;
  }

  _pollCounter += 1;

  if (msSince(_lastms) > MS_PER_UNIT_DURATION) {
    _lastms = _elapsedTime.ms;
    _duration -= 1;

    _r = adjIntensity(_r, _dr);
    _g = adjIntensity(_g, _dg);
    _b = adjIntensity(_b, _db);

  }

  pwm(_pollCounter, _r, PIN_R);
  pwm(_pollCounter, _g, PIN_G);
  pwm(_pollCounter, _b, PIN_B);

  if (_duration == 0) {
    // first set ourselves to the value the control block specified, since we
    // probably didn't hit it due to founding errors.
    ControlBlock *cb = ctrBlockCurrent();
    copyColors(cb);

    cb = ctrBlockNext();
    _duration = cb->duration;

    if (_duration) {
      _dr = calcColorDelta(_r, cb->r, _duration);
      _dg = calcColorDelta(_g, cb->g, _duration);
      _db = calcColorDelta(_b, cb->b, _duration);

      if (_dr == 0) _r = (cb->r+_r)/2;
      if (_dg == 0) _g = (cb->g+_g)/2;
      if (_db == 0) _b = (cb->b+_b)/2;

    } else copyColors(cb);
  }
}
