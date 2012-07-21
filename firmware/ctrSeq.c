#include <avr/eeprom.h>

#include "ctrseq.h"

uint16 _eepromAddr;
ControlSequence _curSeq;

ControlSequence *ctrSeqSetup() {
  eeprom_read_block(&_curSeq, (const void*)_eepromAddr, sizeof(_curSeq));

  if (_curSeq.red != 0xde ||
      _curSeq.green != 0xad ||
      _curSeq.blue != 0xbe ||
      _curSeq.duration != 0xef) PORTB |= _BV(PB3);

  return &_curSeq;
}
