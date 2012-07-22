#include "types.h"

#ifndef _CTRBLOCK_H
#define _CTRBLOCK_H
ControlBlock *ctrBlockSetup();
ControlBlock *ctrBlockCurrent();
ControlBlock *ctrBlockNext();
/**
 * Return the content of the block at blockNumber, or NULL
 * if blockNumber is zero or beyond the end of the array
 */
ControlBlock *ctrBlockGoto(uint8 blockNumber);
#endif
