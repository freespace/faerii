#include <stdint.h>

#ifndef _TYPES_H
#define _TYPES_H

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef int16_t int16;

typedef struct {
  uint16 ms;
  uint16 sec;
} ElapsedTime;

typedef struct {
  uint8 r;
  uint8 g;
  uint8 b;
  union {
    uint8 duration;
    uint8 options;
  };
} ControlBlock;
#endif
