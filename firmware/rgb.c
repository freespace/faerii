#include <util/delay.h>
#include <avr/eeprom.h>

#define PIN_R (1<<0)  // PB0, pin 5
#define PIN_G (1<<3)  // PB3, pin 2
#define PIN_B (1<<4)  // PB4, pin 3

void rgbPinSetup() {
  DDRB |= PIN_R | PIN_G | PIN_B;
}

void rgbPoll() {
}
