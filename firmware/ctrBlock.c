#include <avr/eeprom.h>

#include "config.h"
#include "ctrBlock.h"

uint16 _eepromAddr;
ControlBlock _curBlock;

#define BLOCK_SIZE          (sizeof(_curBlock))

#define INC_EEPROM_ADDR()   (_eepromAddr += BLOCK_SIZE)
#define DEC_EEPROM_ADDR()   (_eepromAddr -= BLOCK_SIZE)

void readCtrBlock() {
  eeprom_read_block(&_curBlock, (const void*)_eepromAddr, BLOCK_SIZE);
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
  _eepromAddr = 0;
  readCtrBlock();

  // check to see if the EEPROM has been initalised
  if (  _curBlock.r   != 0xfa ||
        _curBlock.g != 0xe2 ||
        _curBlock.b  != 0x11 ) return NULL;
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

  if (_eepromAddr > EEPROM_SIZE-BLOCK_SIZE) rewind();

  readCtrBlock();

  if (_curBlock.duration == 0xff) rewind();

  return &_curBlock;
}

ControlBlock *ctrBlockGoto(uint8 blockNumber) {
  uint16 newaddr = blockNumber * BLOCK_SIZE;
  if (newaddr == 0 || newaddr > EEPROM_SIZE-BLOCK_SIZE) return NULL;
  else {
    _eepromAddr = newaddr;
    readCtrBlock();
    return &_curBlock;
  }
}
