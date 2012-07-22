#ifndef _CONFIG_H
#define _CONFIG_H

#define PORTNAME            B
#define PIN_R               PB0  // pin 5
#define PIN_G               PB3  // pin 2
#define PIN_B               PB4  // pin 3

// the tiny85 has 512 bytes of EEPROM
#define EEPROM_SIZE         (512)

// set this to zero if you are using a common
// cathode RGB LED
#define COMMON_ANODE_LED    (1)
#endif
