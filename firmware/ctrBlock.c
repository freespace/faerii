#include <avr/eeprom.h>

#include "ctrBlock.h"

uint16 _eepromAddr;
ControlBlock _curBlock;

#define INC_EEPROM_ADDR()   (_eepromAddr += sizeof(_curBlock))
#define DEC_EEPROM_ADDR()   (_eepromAddr -= sizeof(_curBlock))

void readCtrBlock() {
  eeprom_read_block(&_curBlock, (const void*)_eepromAddr, sizeof(_curBlock));
}

/**
 * Rewinds _eepromAddr to the first block after setup block, or the
 * the first block after next lowest delimiter block.
 *
 * After calling this function, _curBlock contains valid control
 * values, and _eepromAddr points to a valid block.
 * */
void rewind() {
  while (_eepromAddr>0) {
    DEC_EEPROM_ADDR();
    readCtrBlock();
    if (_curBlock.duration == 0xff) break;
  }

  INC_EEPROM_ADDR();
  readCtrBlock();
}

ControlBlock *ctrBlockCurrent() { return &_curBlock;}

ControlBlock *ctrBlockSetup() {
  readCtrBlock();

  // check to see if the EEPROM has been initalised
  if (  _curBlock.r   != 0xfa ||
        _curBlock.g != 0xe2 ||
        _curBlock.b  != 0x11 ||
        ) return NULL;
  else {
    // read the next block which actually contains
    // control information
    ctrBlockNext();

    // $todo parse options here
    return &_curBlock;
  }
}

ControlBlock *ctrBlockNext() {
  // $todo implement reversal here
  INC_EEPROM_ADDR();

  if (_eepromAddr >= 512) rewind();

  readCtrBlock();

  if (_curBlock.duration == 0xff) rewind();

  return &_curBlock;
}
