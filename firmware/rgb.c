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
 * duration also unsigned and specified a time interval, define in
 * 1/2*MS_PER_TICK. Not all durations are valid. See below.
 *
 * Each control block specifies the colour to transition to, and how
 * long that transition should take.
 *
 * The first control block is special. The first 3 bytes still specify
 * the colour intensity, but the 4th byte specifies special options instead of
 * duration.
 *
 * This optons are:
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

#include "types.h"
#include "ctrBlock.h"

#include "rgb.h"

#define PIN_R                 PB0  // pin 5
#define PIN_G                 PB3  // pin 2
#define PIN_B                 PB4  // pin 3

#define CYCLE_PER_TICK        (10)

// our internal time keeping ticks over
// once every MS_PER_TICK. Note that
// a value of 1 is unreliable
#define MS_PER_TICK           (10)

// it is unlikely this will change, but
// this makes parts of the code clearer
#define MS_PER_SEC            (1000)

#define MS_PER_UNIT_DURACTION (2*MS_PER_TICK)

volatile ElapsedTime _elapsedTime;

ISR(BADISR_vect) {
  PORTB |= _BV(PIN_G);
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
  if (phase < intensity) PORTB |= _BV(bitpos);
  else PORTB &= ~_BV(bitpos);
}

// colour and dcolor, packed in rgb order
uint8 _c[3], _dc[3];
uint8 _duration;

void rgbSetup() {
  ControlBlock *cb = ctrBlockSetup();
  _c[0] = cb->red;
  _c[1] = cb->green;
  _c[2] = cb->blue;

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
  DDRB = _BV(PIN_R) | _BV(PIN_G) | _BV(PIN_B);
}


uint8 _pollCounter;
uint16 _lastms;

void rgbPoll() {
  _pollCounter+=1;
  pwm(_pollCounter, _c[0], PIN_R);
  pwm(_pollCounter, _c[1], PIN_G);
  pwm(_pollCounter, _c[2], PIN_B);

  if (_duration == 0) {
    ControlBlock *cb = ctrBlockNext();

}