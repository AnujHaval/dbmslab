#include "BPlusTree.h"
#include <cstring>
#include <bits/stdc++.h>
using namespace std;
RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    // declare searchIndex which will be used to store search index for attrName.
    IndexId searchIndex;
    AttrCacheTable::getSearchIndex(relId,attrName,&searchIndex);
    /* get the search index corresponding to attribute with name attrName
       using AttrCacheTable::getSearchIndex(). */

    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);
    /* load the attribute cache entry into attrCatEntry using
     AttrCacheTable::getAttrCatEntry(). */


    // declare variables block and index which will be used during search
    int block, index;

    if (searchIndex.block == -1 && searchIndex.index == -1) {
        // (search is done for the first time)
        // start the search from the first entry of root.
        block = attrCatEntry.rootBlock;
        index = 0;

        if (block == -1) return RecId{-1, -1};
    } 
    else{
        /*a valid searchIndex points to an entry in the leaf index of the attribute's
        B+ Tree which had previously satisfied the op for the given attrVal.*/

        block = searchIndex.block;
        index = searchIndex.index + 1;  // search is resumed from the next index.

        // load block into leaf using IndLeaf::IndLeaf().
        IndLeaf leaf(block);

        // declare leafHead which will be used to hold the header of leaf.
        HeadInfo leafHead;

        // load header into leafHead using BlockBuffer::getHeader().

        if (index >= leafHead.numEntries){

            /* (all the entries in the block has been searched; search from the
            beginning of the next leaf index block. */
            block = leafHead.rblock;
            index = 0;
            // update block to rblock of current block and index to 0.

            if (block == -1) {
                // (end of linked list reached - the search is done.)
                return RecId{-1, -1};
            }
        }
    }       

 
    /******  Traverse through all the internal nodes according to value
             of attrVal and the operator op                             ******/

    /* (This section is only needed when
        - search restarts from the root block (when searchIndex is reset by caller)
        - root is not a leaf
        If there was a valid search index, then we are already at a leaf block
        and the test condition in the following loop will fail)
    */

    while(StaticBuffer::getStaticBlockType(block) == IND_INTERNAL) {  //use StaticBuffer::getStaticBlockType()

        // load the block into internalBlk using IndInternal::IndInternal().
        IndInternal internalBlk(block);

        HeadInfo intHead;

        // load the header of internalBlk into intHead using BlockBuffer::getHeader()

        // declare intEntry which will be used to store an entry of internalBlk.
        InternalEntry intEntry;

        if (op == NE || op == LT || op == LE) {
            internalBlk.getEntry(&intEntry,0);
            block = intEntry.lChild;

        } 
        else {
            /*
            - EQ, GT and GE: move to the left child of the first entry that is
            greater than (or equal to) attrVal
            (we are trying to find the first entry that satisfies the condition.
            since the values are in ascending order we move to the left child which
            might contain more entries that satisfy the condition)
            */
            int found = 0;
            int i=0;
            for(;i<(intHead.numEntries);i++){
                int ret = internalBlk.getEntry(&intEntry,i);
                int cmpVal = compareAttrs(intEntry.attrVal,attrVal,attrCatEntry.attrType);
                if((op == EQ && cmpVal == 0) || (op==GE && cmpVal>=0) || (op==GT && cmpVal>0)) {found=1;break;}
            }
            /*
             traverse through all entries of internalBlk and find an entry that
             satisfies the condition.
             if op == EQ or GE, then intEntry.attrVal >= attrVal
             if op == GT, then intEntry.attrVal > attrVal
             Hint: the helper function compareAttrs() can be used for comparing
            */


            if (found) {
                // move to the left child of that entry
                block =  intEntry.lChild;// left child of the entry
            } else {
                // move to the right child of the last entry of the block
                // i.e numEntries - 1 th entry of the block
                
                block = intEntry.rChild;// right child of last entry
            }
        }
    }

    // NOTE: `block` now has the block number of a leaf index block.

    /******  Identify the first leaf index entry from the current position
                that satisfies our condition (moving right)             ******/

    while (block != -1) {
        // load the block into leafBlk using IndLeaf::IndLeaf().
        IndLeaf leafBlk(block);
        HeadInfo leafHead;

        // load the header to leafHead using BlockBuffer::getHeader().

        // declare leafEntry which will be used to store an entry from leafBlk
        Index leafEntry;

        while (index < leafHead.numEntries/*index < numEntries in leafBlk*/) {
            leafBlk.getEntry(&leafEntry,index);
            // load entry corresponding to block and index into leafEntry
            // using IndLeaf::getEntry().

            int cmpVal = compareAttrs(leafEntry.attrVal,attrVal,attrCatEntry.attrType);

            if (
                (op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == LT && cmpVal < 0) ||
                (op == GT && cmpVal > 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == NE && cmpVal != 0)
            ) {
                // (entry satisfying the condition found)
                searchIndex.block = block;
                searchIndex.index = index;
                AttrCacheTable::setSearchIndex(relId,attrName,&searchIndex);
                // set search index to {block, index}
                return RecId{leafEntry.block,leafEntry.slot};
                // return the recId {leafEntry.block, leafEntry.slot}.

            } else if ((op == EQ || op == LE || op == LT) && cmpVal > 0) {
                return RecId{-1,-1};
                /*future entries will not satisfy EQ, LE, LT since the values
                    are arranged in ascending order in the leaves */

                // return RecId {-1, -1};
            }

            // search next index.
            ++index;
        }

        /*only for NE operation do we have to check the entire linked list;
        for all the other op it is guaranteed that the block being searched
        will have an entry, if it exists, satisying that op. */
        if (op != NE) {
            break;
        }
        block = leafHead.rblock;
        index = 0;
        // block = next block in the linked list, i.e., the rblock in leafHead.
        // update index to 0.
    }
    return RecId{-1,-1};
    // no entry satisying the op was found; return the recId {-1,-1}
}

int BPlusTree::bPlusCreate(int relId, char attrName[ATTR_SIZE]) {
    if(relId == RELCAT_RELID || relId == ATTRCAT_RELID) return E_NOTPERMITTED;
    AttrCatEntry attrCatBuf;
    int ret = AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatBuf);
    if(ret != SUCCESS) return ret;
    if (attrCatBuf.rootBlock != -1) return SUCCESS;

    /******Creating a new B+ Tree ******/

    // get a free leaf block using constructor 1 to allocate a new block
    IndLeaf rootBlockBuf;
    // (if the block could not be allocated, the appropriate error code
    //  will be stored in the blockNum member field of the object)

    // declare rootBlock to store the blockNumber of the new leaf block
    int rootBlock = rootBlockBuf.getBlockNum();
    // if there is no more disk space for creating an index
    if (rootBlock == E_DISKFULL) return E_DISKFULL;
    attrCatBuf.rootBlock = rootBlock;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatBuf);

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId,&relCatEntry);
    int block = relCatEntry.firstBlk/* first record block of the relation */;
    // load the relation catalog entry into relCatEntry
    // using RelCacheTable::getRelCatEntry().
       
    /***** Traverse all the blocks in the relation and insert them one
           by one into the B+ Tree *****/
    while (block != -1) {
        // declare a RecBuffer object for `block` (using appropriate constructor)
        RecBuffer blockbuf(block);
        unsigned char slotMap[relCatEntry.numSlotsPerBlk];
        blockbuf.getSlotMap(slotMap);
        // load the slot map into slotMap using RecBuffer::getSlotMap().

        // for every occupied slot of the block
        int slotitr=0;
        while(slotitr < relCatEntry.numSlotsPerBlk){
            if(slotMap[slotitr] == SLOT_OCCUPIED)
            {
                Attribute record[relCatEntry.numAttrs];
                blockbuf.getRecord(record,slotitr);
                // load the record corresponding to the slot into `record`
                // using RecBuffer::getRecord().
                RecId recid;
                recid.block = block;
                recid.slot = slotitr;

                // declare recId and store the rec-id of this record in it
                // RecId recId{block, slot};
            
                ret = BPlusTree::bPlusInsert(relId,attrName,record[attrCatBuf.offset],recid);
                // insert the attribute value corresponding to attrName from the record
                // into the B+ tree using bPlusInsert.
                // (note that bPlusInsert will destroy any existing bplus tree if
                // insert fails i.e when disk is full)
                // retVal = bPlusInsert(relId, attrName, attribute value, recId);

                if (ret == E_DISKFULL) {
                    // (unable to get enough blocks to build the B+ Tree.)
                    return E_DISKFULL;
                }
            }
            slotitr++;
        }
        HeadInfo blockHeader;
        blockbuf.getHeader(&blockHeader);    
        // get the header of the block using BlockBuffer::getHeader()
        block = blockHeader.rblock;
        // set block = rblock of current block (from the header)
    }
    return SUCCESS;
}

int BPlusTree::bPlusDestroy(int rootBlockNum) {
    if (rootBlockNum<0 || rootBlockNum>=DISK_BLOCKS) return E_OUTOFBOUND;
    int type = StaticBuffer::getStaticBlockType(rootBlockNum);

    if (type == IND_LEAF) {
        // declare an instance of IndLeaf for rootBlockNum using appropriate
        // constructor
        IndLeaf Leaf(rootBlockNum);
        Leaf.releaseBlock();
        // release the block using BlockBuffer::releaseBlock().

        return SUCCESS;

    } 
    else if (type == IND_INTERNAL) {
        IndInternal Internal(rootBlockNum);
        
        HeadInfo inthead;
        Internal.getHeader(&inthead);
        int blockitr=0;
        InternalEntry entry;
        Internal.getEntry(&entry,blockitr);
        bPlusDestroy(entry.lChild);
        while(blockitr < inthead.numEntries){
            Internal.getEntry(&entry,blockitr);
            bPlusDestroy(entry.rChild);
            blockitr++;
        }

        Internal.releaseBlock();
        return SUCCESS;

    } 
    else return E_INVALIDBLOCK;
    
}
                                                    
int BPlusTree::bPlusInsert(int relId, char attrName[ATTR_SIZE], Attribute attrVal, RecId recId) {
    // get the attribute cache entry corresponding to attrName
    // using AttrCacheTable::getAttrCatEntry().
    AttrCatEntry AttrCatBuf;
    int ret = AttrCacheTable::getAttrCatEntry(relId,attrName,&AttrCatBuf);
    if(ret != SUCCESS) return ret;
    int rootblock = AttrCatBuf.rootBlock;
    if (rootblock == -1) return E_NOINDEX;
    int leafBlkNum = findLeafToInsert(rootblock,attrVal,AttrCatBuf.attrType);
/* findLeafToInsert(root block num, attrVal, attribute type) */
    Index leafindex;
    leafindex.attrVal = attrVal;
    leafindex.block = recId.block;
    leafindex.slot = recId.slot;
    ret = insertIntoLeaf(relId,attrName,leafBlkNum,leafindex);    //POTENTIAL ERROR PLEASE CHECK
    if (ret == E_DISKFULL) {
        // destroy the existing B+ tree by passing the rootBlock to bPlusDestroy().
        bPlusDestroy(rootblock);
        AttrCatBuf.rootBlock = -1;
        AttrCacheTable::setAttrCatEntry(relId,attrName,&AttrCatBuf);
        // update the rootBlock of attribute catalog cache entry to -1 using
        // AttrCacheTable::setAttrCatEntry().

        return E_DISKFULL;
    }
    return SUCCESS;
}
                                              
int BPlusTree::findLeafToInsert(int rootBlock, Attribute attrVal, int attrType) {
    int blockNum = rootBlock;
    while (StaticBuffer::getStaticBlockType(blockNum) != IND_LEAF/*block is not of type IND_LEAF */) {  // use StaticBuffer::getStaticBlockType()
        IndInternal currentblock(blockNum);
        HeadInfo currentHeader;
        currentblock.getHeader(&currentHeader);
        // declare an IndInternal object for block using appropriate constructor
        // get header of the block using BlockBuffer::getHeader()
        int i=0;
        InternalEntry IntEntry;
        for(;i<currentHeader.numEntries;i++){
            currentblock.getEntry(&IntEntry,i);
            
            if(compareAttrs(attrVal,IntEntry.attrVal,attrType) <= 0) {
                break;
            } 
        }
        
        /* iterate through all the entries, to find the first entry whose
             attribute value >= value to be inserted.
             NOTE: the helper function compareAttrs() declared in BlockBuffer.h
                   can be used to compare two Attribute values. */
        if (i==currentHeader.numEntries){
            // set blockNum = rChild of (nEntries-1)'th entry of the block
            // (i.e. rightmost child of the block)
            InternalEntry IntEntry;
            currentblock.getEntry(&IntEntry,currentHeader.numEntries-1);
            blockNum=IntEntry.rChild;
        } 
        else {
            InternalEntry IntEntry;
            currentblock.getEntry(&IntEntry,i);            
            blockNum=IntEntry.lChild;
            // set blockNum = lChild of the entry that was found
        }
    }
    return blockNum;
}

int BPlusTree::insertIntoLeaf(int relId, char attrName[ATTR_SIZE], int blockNum, Index indexEntry) {
    // get the attribute cache entry corresponding to attrName
    // using AttrCacheTable::getAttrCatEntry().
    AttrCatEntry AttrCatBuf;
    int ret = AttrCacheTable::getAttrCatEntry(relId,attrName,&AttrCatBuf);
    if(ret != SUCCESS) return ret;   
    // declare an IndLeaf instance for the block using appropriate constructor
    IndLeaf LeafBlock(blockNum);
    HeadInfo blockHeader;
    LeafBlock.getHeader(&blockHeader);
    // store the header of the leaf index block into blockHeader
    // using BlockBuffer::getHeader()

    // the following variable will be used to store a list of index entries with
    // existing indices + the new index to insert
    Index indices[blockHeader.numEntries + 1];
    Index entry;
    int flag = 0;
    for(int indexNum=0;indexNum<blockHeader.numEntries;indexNum++){
        LeafBlock.getEntry(&entry,indexNum);
        if(compareAttrs(entry.attrVal,indexEntry.attrVal,AttrCatBuf.attrType) <= 0){
            indices[indexNum] = entry;
        }
        else{
            indices[indexNum] = indexEntry;
            flag = 1;
            while(indexNum<(blockHeader.numEntries)){
                LeafBlock.getEntry(&entry,indexNum);
                indices[indexNum] = entry;
                indexNum++;
            }
            break;
        }
    }
    if(flag==0) indices[blockHeader.numEntries] = indexEntry; //Inserting at the End of the leaf block
    /*
    Iterate through all the entries in the block and copy them to the array indices.
    Also insert `indexEntry` at appropriate position in the indices array maintaining
    the ascending order.
    - use IndLeaf::getEntry() to get the entry
    - use compareAttrs() declared in BlockBuffer.h to compare two Attribute structs
    */

    if (blockHeader.numEntries != MAX_KEYS_LEAF) {
        // (leaf block has not reached max limit)

        blockHeader.numEntries++;
        LeafBlock.setHeader(&blockHeader);
        // increment blockHeader.numEntries and update the header of block
        // using BlockBuffer::setHeader().
        for(int indexNum=0;indexNum<blockHeader.numEntries;indexNum++) LeafBlock.setEntry(&indices[indexNum],indexNum);
        // iterate through all the entries of the array `indices` and populate the
        // entries of block with them using IndLeaf::setEntry().
        
        return SUCCESS;
    }

    int newRightBlk = splitLeaf(blockNum, indices);

    // if splitLeaf() returned E_DISKFULL
    if(newRightBlk == E_DISKFULL) return E_DISKFULL;
    int ret1,ret2;
    if (blockHeader.pblock != -1) {  // check pblock in header
        // insert the middle value from `indices` into the parent block using the
        // insertIntoInternal() function. (i.e the last value of the left block)
        InternalEntry midEntry;
        // the middle value will be at index 31 (given by constant MIDDLE_INDEX_LEAF)
        midEntry.attrVal = indices[MIDDLE_INDEX_LEAF].attrVal;
        midEntry.lChild = blockNum;
        midEntry.rChild = newRightBlk;
        // create a struct InternalEntry with attrVal = indices[MIDDLE_INDEX_LEAF].attrVal,
        // lChild = currentBlock, rChild = newRightBlk and pass it as argument to
        // the insertIntoInternalFunction as follows
        ret1 = insertIntoInternal(relId,attrName,blockHeader.pblock,midEntry);

        // insertIntoInternal(relId, attrName, parent of current block, new internal entry)

    } else {
        // the current block was the root block and is now split. a new internal index
        // block needs to be allocated and made the root of the tree.
        // To do this, call the createNewRoot() function with the following arguments
        ret2 = createNewRoot(relId,attrName,indices[MIDDLE_INDEX_LEAF].attrVal,blockNum,newRightBlk);
        // createNewRoot(relId, attrName, indices[MIDDLE_INDEX_LEAF].attrVal,
        //               current block, new right block)
    }

    // if either of the above calls returned an error (E_DISKFULL), then return that
    if(ret1 == E_DISKFULL || ret2 == E_DISKFULL) return E_DISKFULL;
    
    return SUCCESS;
}

int BPlusTree::splitLeaf(int leafBlockNum, Index indices[]) {
    // declare rightBlk, an instance of IndLeaf using constructor 1 to obtain new
    // leaf index block that will be used as the right block in the splitting
    IndLeaf rightBlk;
    IndLeaf leftBlk(leafBlockNum);

    // declare leftBlk, an instance of IndLeaf using constructor 2 to read from
    // the existing leaf block

    int rightBlkNum = rightBlk.getBlockNum();
    int leftBlkNum = leftBlk.getBlockNum();

    if (rightBlkNum == E_DISKFULL) return E_DISKFULL;
    

    HeadInfo leftBlkHeader, rightBlkHeader;
    // get the headers of left block and right block using BlockBuffer::getHeader()
    rightBlk.getHeader(&rightBlkHeader);
    leftBlk.getHeader(&leftBlkHeader);
    // set rightBlkHeader with the following values
    rightBlkHeader.numEntries = (MAX_KEYS_LEAF+1)/2;// - number of entries = (MAX_KEYS_LEAF+1)/2 = 32, 
    rightBlkHeader.pblock = leftBlkHeader.pblock;// - pblock = pblock of leftBlk
    rightBlkHeader.lblock = leftBlkNum;// - lblock = leftBlkNum
    rightBlkHeader.rblock = leftBlkHeader.rblock;// - rblock = rblock of leftBlk
    rightBlk.setHeader(&rightBlkHeader);
    // and update the header of rightBlk using BlockBuffer::setHeader()

    // set leftBlkHeader with the following values
    leftBlkHeader.numEntries = (MAX_KEYS_LEAF+1)/2;// - number of entries = (MAX_KEYS_LEAF+1)/2 = 32
    leftBlkHeader.rblock = rightBlkNum;// - rblock = rightBlkNum
    leftBlk.setHeader(&leftBlkHeader);// and update the header of leftBlk using BlockBuffer::setHeader() */
    int i=0;
    for(int indexentry=0;indexentry<32;indexentry++) {leftBlk.setEntry(&indices[i],indexentry); i++;}
    for(int indexentry=0;indexentry<32;indexentry++) {rightBlk.setEntry(&indices[i],indexentry);i++;}
    // set the first 32 entries of leftBlk = the first 32 entries of indices array
    // and set the first 32 entries of newRightBlk = the next 32 entries of
    // indices array using IndLeaf::setEntry().

    return rightBlkNum;
}

int BPlusTree::insertIntoInternal(int relId, char attrName[ATTR_SIZE], int intBlockNum, InternalEntry intEntry) {
    // get the attribute cache entry corresponding to attrName
    // using AttrCacheTable::getAttrCatEntry().
    AttrCatEntry AttrCatBuf;
    int ret = AttrCacheTable::getAttrCatEntry(relId,attrName,&AttrCatBuf);
    if(ret != SUCCESS) return ret;

    IndInternal intBlk(intBlockNum);
    // declare intBlk, an instance of IndInternal using constructor 2 for the block
    // corresponding to intBlockNum
    HeadInfo blockHeader;
    intBlk.getHeader(&blockHeader);
    // load blockHeader with header of intBlk using BlockBuffer::getHeader().

    // declare internalEntries to store all existing entries + the new entry
    InternalEntry internalEntries[blockHeader.numEntries + 1];
    int flag=-1;
    for(int intitr = 0;intitr<blockHeader.numEntries;intitr++){
        InternalEntry Entry;
        intBlk.getEntry(&Entry,intitr);
        if(compareAttrs(Entry.attrVal,intEntry.attrVal,AttrCatBuf.attrType) <= 0) {
            internalEntries[intitr] = Entry;
        }   
        else{            
            internalEntries[intitr] = intEntry;
            flag = intitr; //here flag stores the postition of entered internalEntry for the lchild,rchild purposes
            while(intitr<(blockHeader.numEntries+1)){
                intBlk.getEntry(&Entry,intitr);
                internalEntries[intitr] = Entry;
                intitr++;
            }
        }
    }
    if(flag==-1){
        internalEntries[blockHeader.numEntries] = intEntry;
        flag=blockHeader.numEntries;
    } 
    if(flag>0) internalEntries[flag-1].rChild = intEntry.lChild;
    if (flag < blockHeader.numEntries) internalEntries[flag+1].lChild = intEntry.rChild;
    
    /*
    Iterate through all the entries in the block and copy them to the array
    `internalEntries`. Insert `indexEntry` at appropriate position in the
    array maintaining the ascending order.
        - use IndInternal::getEntry() to get the entry
        - use compareAttrs() to compare two structs of type Attribute

    Update the lChild of the internalEntry immediately following the newly added
    entry to the rChild of the newly added entry.
    */

    if (blockHeader.numEntries != MAX_KEYS_INTERNAL) {
        // (internal index block has not reached max limit)
        blockHeader.numEntries++;
        intBlk.setHeader(&blockHeader);
        // increment blockheader.numEntries and update the header of intBlk
        // using BlockBuffer::setHeader().

        // iterate through all entries in internalEntries array and populate the
        // entries of intBlk with them using IndInternal::setEntry().
        for (int intitr = 0; intitr < blockHeader.numEntries; intitr++) intBlk.setEntry(&internalEntries[intitr], intitr);
        return SUCCESS;
    }

    // If we reached here, the `internalEntries` array has more than entries than
    // can fit in a single internal index block. Therefore, we will need to split
    // the entries in `internalEntries` between two internal index blocks. We do
    // this using the splitInternal() function.
    // This function will return the blockNum of the newly allocated block or
    // E_DISKFULL if there are no more blocks to be allocated.

    int newRightBlk = splitInternal(intBlockNum, internalEntries);
    int ret1,ret2;
    if (newRightBlk == E_DISKFULL/* splitInternal() returned E_DISKFULL */) {
        bPlusDestroy(intEntry.rChild);
        // Using bPlusDestroy(), destroy the right subtree, rooted at intEntry.rChild.
        // This corresponds to the tree built up till now that has not yet been
        // connected to the existing B+ Tree

        return E_DISKFULL;
    }
    if (blockHeader.pblock!=-1/* the current block was not the root */) {  // (check pblock in header)
        // insert the middle value from `internalEntries` into the parent block
        // using the insertIntoInternal() function (recursively).
        InternalEntry midEntry;
        midEntry.lChild = intBlockNum;
        midEntry.rChild = newRightBlk;

        // create a struct InternalEntry with lChild = current block, rChild = newRightBlk
        // and attrVal = internalEntries[MIDDLE_INDEX_INTERNAL].attrVal
        // and pass it as argument to the insertIntoInternalFunction as follows

        ret1 = insertIntoInternal(relId, attrName,blockHeader.pblock,midEntry);

    } else {
        // the current block was the root block and is now split. a new internal index
        // block needs to be allocated and made the root of the tree.
        // To do this, call the createNewRoot() function with the following arguments
        ret2 = createNewRoot(relId, attrName,internalEntries[MIDDLE_INDEX_INTERNAL].attrVal,intBlockNum, newRightBlk);
    }
    if(ret1 == E_DISKFULL || ret2 == E_DISKFULL) return E_DISKFULL;
    return SUCCESS;    
    // if either of the above calls returned an error (E_DISKFULL), then return that
    // else return SUCCESS
}

int BPlusTree::splitInternal(int intBlockNum, InternalEntry internalEntries[]) {
    // declare rightBlk, an instance of IndInternal using constructor 1 to obtain new
    // internal index block that will be used as the right block in the splitting
    IndInternal rightBlk;
    IndInternal leftBlk(intBlockNum);
    // declare leftBlk, an instance of IndInternal using constructor 2 to read from
    // the existing internal index block

    int rightBlkNum = rightBlk.getBlockNum()/* block num of right blk */;
    int leftBlkNum = leftBlk.getBlockNum(); /* block num of left blk */;

    if (rightBlkNum==E_DISKFULL) {
        //(failed to obtain a new internal index block because the disk is full)
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;
    // get the headers of left block and right block using BlockBuffer::getHeader()
    leftBlk.getHeader(&leftBlkHeader);
    rightBlk.getHeader(&rightBlkHeader);
    
    // set rightBlkHeader with the following values
    rightBlkHeader.numEntries = (MAX_KEYS_INTERNAL)/2;// - number of entries = (MAX_KEYS_INTERNAL)/2 = 50
    rightBlkHeader.pblock = leftBlkHeader.pblock;// - pblock = pblock of leftBlk
    // and update the header of rightBlk using BlockBuffer::setHeader()
    rightBlk.setHeader(&rightBlkHeader);

    // set leftBlkHeader with the following values
    leftBlkHeader.numEntries = (MAX_KEYS_INTERNAL)/2;// - number of entries = (MAX_KEYS_INTERNAL)/2 = 50
    leftBlkHeader.rblock = rightBlkNum;// - rblock = rightBlkNum
    // and update the header using BlockBuffer::setHeader()
    leftBlk.setHeader(&leftBlkHeader);
    int itr=0;
    for(int leftitr=0;leftitr<50;leftitr++){leftBlk.setEntry(&internalEntries[itr],leftitr);itr++;}
    for(int rightitr=0;rightitr<50;rightitr++){rightBlk.setEntry(&internalEntries[itr],rightitr);itr++;}
    /*
    - set the first 50 entries of leftBlk = index 0 to 49 of internalEntries
      array
    - set the first 50 entries of newRightBlk = entries from index 51 to 100
      of internalEntries array using IndInternal::setEntry().
      (index 50 will be moving to the parent internal index block)
    */
    int type = StaticBuffer::getStaticBlockType(internalEntries[0].lChild);/* block type of a child of any entry of the internalEntries array */;
    //            (use StaticBuffer::getStaticBlockType())
    BlockBuffer blockbuf(internalEntries[MIDDLE_INDEX_INTERNAL+1].lChild);
    HeadInfo blockHeader;
    blockbuf.getHeader(&blockHeader);
    blockHeader.pblock = rightBlkNum;   //WHO WAS THE PBLOCK BEFORE THIS STEP? WAS IT THE PRESPLIT INTERNAL BLOCK?
    blockbuf.setHeader(&blockHeader);

    for (int intitr=0;intitr<MIDDLE_INDEX_INTERNAL;intitr++/* each child block of the new right block */) {
        BlockBuffer blockbuf (internalEntries[intitr + MIDDLE_INDEX_INTERNAL+1].rChild);
        // declare an instance of BlockBuffer to access the child block using
        // constructor 2
        blockbuf.getHeader(&blockHeader);
        blockHeader.pblock = rightBlkNum; //WHO WAS THE PBLOCK BEFORE THIS STEP? WAS IT THE PRESPLIT INTERNAL BLOCK?
        blockbuf.setHeader(&blockHeader);
        // update pblock of the block to rightBlkNum using BlockBuffer::getHeader()
        // and BlockBuffer::setHeader().
    }

    return rightBlkNum;
}

int BPlusTree::createNewRoot(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int lChild, int rChild) {
    // get the attribute cache entry corresponding to attrName
    // using AttrCacheTable::getAttrCatEntry().
    AttrCatEntry AttrCatBuf;
    int ret = AttrCacheTable::getAttrCatEntry(relId,attrName,&AttrCatBuf);
    if(ret != SUCCESS) return ret;

    // declare newRootBlk, an instance of IndInternal using appropriate constructor
    // to allocate a new internal index block on the disk
    IndInternal newRootBlk;
    int newRootBlkNum = newRootBlk.getBlockNum()/* block number of newRootBlk */;

    if (newRootBlkNum == E_DISKFULL) {
        bPlusDestroy(rChild);
        return E_DISKFULL;
    }
    
    HeadInfo newRootBlkHeader;
    newRootBlk.getHeader(&newRootBlkHeader);
    newRootBlkHeader.numEntries = 1;
    newRootBlk.setHeader(&newRootBlkHeader);
   
    // update the header of the new block with numEntries = 1 using
    // BlockBuffer::getHeader() and BlockBuffer::setHeader()
    InternalEntry intEntry;
    intEntry.lChild = lChild;
    intEntry.rChild = rChild;
    intEntry.attrVal = attrVal;

    newRootBlk.setEntry(&intEntry,0);

    BlockBuffer lChildBlk (lChild);
    BlockBuffer rChildBlk (rChild);
    HeadInfo lChildHeader;
    HeadInfo rChildHeader;
    lChildBlk.getHeader(&lChildHeader);
    rChildBlk.getHeader(&rChildHeader);
    lChildHeader.pblock = newRootBlkNum;
    rChildHeader.pblock = newRootBlkNum;
    lChildBlk.setHeader(&lChildHeader);
    rChildBlk.setHeader(&rChildHeader);

    AttrCatBuf.rootBlock = newRootBlkNum;
    AttrCacheTable::setAttrCatEntry(relId,attrName,&AttrCatBuf);

    return SUCCESS;
}

