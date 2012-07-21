#include <stdint.h>

#ifndef _TYPES_H
#define _TYPES_H
typedef uint8_t uint8;
typedef uint16_t uint16;

typedef struct {
  uint16 ms;
  uint16 sec;
} ElapsedTime;

typedef struct {
  uint8 red;
  uint8 green;
  uint8 blue;
  union {
    uint8 duration;
    uint8 options;
  };
} ControlSequence;
#endif
