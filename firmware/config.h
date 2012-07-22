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

/*
 * When the hosts writes data to the device, the first data byte
 * is a command byte. This reduces the number of bytes we can
 * transfer at once to 253.
 *
 * There are 3 write/reads because EEPROM is 512 bytes, while we can only
 * transfer 254 bytes at a time. The additional read and write commands
 * have different start addresses. e.g. READ_CMD reads to 0 onwards, while
 * READ_2 reads from 254 and READ_3 reads from 508.
 *
 * For now only READ, WRITE, RESTART, and GOTO is implemented.
 */

///#define CMD_READD            (0)
///#define CMD_READ2           (1)
///#define CMD_READ3           (3)
#define CMD_WRITE           (4)
///#define CMD_WRITE2          (5)
///#define CMD_WRITE3          (6)
#define CMD_RESTART         (7)
#define CMD_GOTO            (8)
#define CMD_NONE            (0xff)
#endif
