#include "BlockBuffer.h"
#include <cstdlib>
#include <cstring>

// the declarations for these functions can be found in "BlockBuffer.h"

BlockBuffer::BlockBuffer(int blockNum) {
    this->blockNum = blockNum;
  // initialise this.blockNum with the argument
}



// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum){}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr){

    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);
    if(bufferNum==E_BLOCKNOTINBUFFER){
      bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);
      if(bufferNum == E_OUTOFBOUND) return E_OUTOFBOUND;
      Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
    }  

    *buffPtr = StaticBuffer::blocks[bufferNum];

    return SUCCESS;
}

// load the block header into the argument pointer
int BlockBuffer::getHeader(struct HeadInfo *head) {

  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if(ret!=SUCCESS) return ret;


  // read the block at this.blockNum into the buffer


  // populate the numEntries, numAttrs and numSlots fields in *head
  memcpy(&head->numSlots, bufferPtr + 24, 4);
  memcpy(&head->numAttrs, bufferPtr + 20, 4);
  memcpy(&head->numEntries, bufferPtr + 16, 4);
  memcpy(&head->rblock, bufferPtr + 12, 4);
  memcpy(&head->lblock, bufferPtr + 8, 4);

  return SUCCESS;
}

// load the record at slotNum into the argument pointerint RecBuffer::getRecord(union Attribute *rec, int slotNum) {
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {  
  struct HeadInfo head;
  
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if(ret!=SUCCESS) return ret;

  // get the header using this.getHeader() function
  this->getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;
  int recordSize = attrCount * ATTR_SIZE;

  // unsigned char buffer[BLOCK_SIZE];
  // Disk::readBlock(buffer, this->blockNum);


  int offset = HEADER_SIZE + slotCount + (recordSize*slotNum);

  unsigned char *slotPointer = bufferPtr+offset;

  // load the record into the rec data structure
  memcpy(rec, slotPointer, recordSize);

  return SUCCESS;
}

/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for `*slotMap`
*/
int RecBuffer::getSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr;

  // get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  struct HeadInfo head;
  // get the header of the block using getHeader() function
  this->getHeader(&head);

  int slotCount = head.numSlots;

  // get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

  // copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
  memcpy(slotMap,slotMapInBuffer,slotCount);
  return SUCCESS;
}
