#include "StaticBuffer.h"
unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];
StaticBuffer::StaticBuffer(){
    unsigned char blocks[BLOCK_SIZE];
    for(int i=0;i<BLOCK_ALLOCATION_MAP_SIZE;i++){
        Disk::readBlock(blocks,i);
        for(int j=0;j<BLOCK_SIZE;j++){
            blockAllocMap[(i*BLOCK_SIZE) + j] = blocks[j];
        }
    }
    for(int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++){
        metainfo[bufferIndex].free = true;
        metainfo[bufferIndex].dirty = false;
        metainfo[bufferIndex].timeStamp = -1;
        metainfo[bufferIndex].blockNum = -1;
    }
}

StaticBuffer::~StaticBuffer(){
    unsigned char block[BLOCK_SIZE];
    for(int i=0;i<BLOCK_ALLOCATION_MAP_SIZE;i++){
        for(int j=0;j<BLOCK_SIZE;j++){
            block[j] = blockAllocMap[(i*BLOCK_SIZE)+j];
        }
        Disk::writeBlock(block,i);
    }
    for(int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++){
        if(metainfo[bufferIndex].free == false && metainfo[bufferIndex].dirty == true)
            Disk::writeBlock(blocks[bufferIndex],metainfo[bufferIndex].blockNum);
        metainfo[bufferIndex].free = false;
        metainfo[bufferIndex].dirty = true;
    }
}

int StaticBuffer::getFreeBuffer(int blockNum){
    if(blockNum<0 || blockNum>=DISK_BLOCKS) return E_OUTOFBOUND;

    for(int i=0;i<BUFFER_CAPACITY;i++){ 
        if(metainfo[i].free == false) (metainfo[i].timeStamp)++;
    } 

    int allocatedBuffer=0;
    for(;allocatedBuffer<BUFFER_CAPACITY;allocatedBuffer++){
        if(metainfo[allocatedBuffer].free == true) break;
    }
    
    
    //buffer is full case
    if(allocatedBuffer == BUFFER_CAPACITY){
        int maxtime = -1;
        for(int i=0;i<BUFFER_CAPACITY;i++){
            if((metainfo[i].free == false && metainfo[i].dirty == true) && (metainfo[i].timeStamp > maxtime)){
                allocatedBuffer = i;  
            }
        }
    }
    /*--------------------*/
    Disk::writeBlock(blocks[allocatedBuffer],metainfo[allocatedBuffer].blockNum);

    metainfo[allocatedBuffer].free = false;
    metainfo[allocatedBuffer].blockNum = blockNum;
    metainfo[allocatedBuffer].timeStamp = 0;

    return allocatedBuffer;
}

int StaticBuffer::getBufferNum(int blockNum){
    if(blockNum<0 || blockNum>DISK_BLOCKS) return E_OUTOFBOUND;

    for(int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++){
       if(metainfo[bufferIndex].blockNum == blockNum) return bufferIndex;
    }
    return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum){
    int bufferNum = getBufferNum(blockNum);
    // find the buffer index corresponding to the block using getBufferNum().
    if(bufferNum == E_BLOCKNOTINBUFFER) return E_BLOCKNOTINBUFFER;
    // if block is not present in the buffer (bufferNum = E_BLOCKNOTINBUFFER)
    //     return E_BLOCKNOTINBUFFER
    if(bufferNum == E_OUTOFBOUND) return E_OUTOFBOUND;
    // if blockNum is out of bound (bufferNum = E_OUTOFBOUND)
    //     return E_OUTOFBOUND
    metainfo[bufferNum].dirty = true;
    // else
    //     (the bufferNum is valid)
    //     set the dirty bit of that buffer to true in metainfo
    return SUCCESS;
}

int StaticBuffer::getStaticBlockType(int blockNum){
    // Check if blockNum is valid (non zero and less than number of disk blocks)
    // and return E_OUTOFBOUND if not valid.
    if(blockNum < 0 || blockNum >= DISK_BLOCKS) return E_OUTOFBOUND;
    // Access the entry in block allocation map corresponding to the blockNum argument
    // and return the block type after type casting to integer.
    return (int)blockAllocMap[blockNum];
}

