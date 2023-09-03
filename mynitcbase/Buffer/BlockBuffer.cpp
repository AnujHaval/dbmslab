#include "BlockBuffer.h"
#include <cstdlib>
#include <cstring>
#include<stdio.h>
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
    else{
      StaticBuffer::metainfo[bufferNum].timeStamp = -1;
      for(int i=0;i<BUFFER_CAPACITY;i++){
        if(StaticBuffer::metainfo[bufferNum].free == false && StaticBuffer::metainfo[bufferNum].dirty == true) StaticBuffer::metainfo[bufferNum].timeStamp++;
      }
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

int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
    unsigned char *bufferPtr;
    int retVal = loadBlockAndGetBufferPtr(&bufferPtr);
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    if(retVal!=SUCCESS) return retVal;
    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
    HeadInfo head;
    this->getHeader(&head);
    /* get the header of the block using the getHeader() function */
    int numattr = head.numAttrs;
    // get number of attributes in the block.
    int numslot = head.numSlots;
    // get the number of slots in the block.
    if(slotNum<0 || slotNum>=numslot) return E_OUTOFBOUND;
    // if input slotNum is not in the permitted range return E_OUTOFBOUND.
  int recordSize = numattr * ATTR_SIZE;
  int offset = HEADER_SIZE + numslot + (recordSize*slotNum);
  unsigned char *slotPointer = bufferPtr+offset;
  memcpy(slotPointer, rec, recordSize);
  retVal = StaticBuffer::setDirtyBit(this->blockNum);
  if (retVal != SUCCESS) {
		printf("There is some error in the code!\n");
		exit(1);
	}
  return SUCCESS;
}