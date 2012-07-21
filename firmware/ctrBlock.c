#include <avr/eeprom.h>

#include "ctrBlock.h"

uint16 _eepromAddr;
ControlBlock _curBlock;

void readCtrBlock() {
  eeprom_read_block(&_curBlock, (const void*)_eepromAddr, sizeof(_curBlock));
}

void rewind() {
  // rewinds to either the last delimiter block, or to 0
  while (_eepromAddr>0) {
    _eepromAddr -= sizeof(_curBlock);
    readCtrBlock();
    if (_curBlock.duration == 0xff) break;
  }
}

ControlBlock *ctrBlockCurrent() { return &_curBlock;}

ControlBlock *ctrBlockSetup() {
  readCtrBlock();

  // $todo parse options here
  return &_curBlock;
}

ControlBlock *ctrBlockNext() {
  // $todo implement reversal here
  _eepromAddr += sizeof(_curBlock);

  if (_eepromAddr >= 512) rewind();

  readCtrBlock();

  if (_curBlock.duration == 0xff) {
    rewind();
    // need this b/c rewind set curBlock to a delimiter block
    readCtrBlock();
  }

  return &_curBlock;
}