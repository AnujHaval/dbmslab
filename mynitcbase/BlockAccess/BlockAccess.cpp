#include "BlockAccess.h"
#include <cstring>
#include<bits/stdc++.h>
using namespace std;
int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {

    double diff;
    if (attrType == STRING)
        diff = strcmp(attr1.sVal, attr2.sVal);

    else
        diff = attr1.nVal - attr2.nVal;

    if(diff == 0)
        return 0;
    else if (diff > 0)
        return 1;
    else
        return -1;
    /*
    if diff > 0 then return 1
    if diff < 0 then return -1
    if diff = 0 then return 0
    */
}
RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId,&prevRecId);
    int block,slot;
    // let block and slot denote the record id of the record being currently checked
    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(relId,&relCatBuf);

    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)
        block=relCatBuf.firstBlk;
        slot=0;
    }
    else
    {
        // (there is a hit from previous search; search should start from
        // the record next to the search index record)
        block= prevRecId.block;
        slot = prevRecId.slot+1;
        // block = search index's block
        // slot = search index's slot + 1
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
    while (block != -1)
    {
        /* create a RecBuffer object for block (use RecBuffer Constructor for
           existing block) */
        RecBuffer Recbuf(block);

        HeadInfo head;    //We start from the record id (block, slot) and iterate over the remaining records 
        Recbuf.getHeader(&head);
        unsigned char* slotmap = (unsigned char*)malloc(head.numSlots*sizeof(unsigned char));
        Recbuf.getSlotMap(slotmap);
        Attribute record[head.numAttrs];
        Recbuf.getRecord(record,slot);
        
        // get the record with id (block, slot) using RecBuffer::getRecord()
        // get header of the block using RecBuffer::getHeader() function
        // get slot map of the block using RecBuffer::getSlotMap() function

        // If slot >= the number of slots per block(i.e. no more slots in this block)
        if(slot >= relCatBuf.numSlotsPerBlk)
        {
            block = head.rblock; // update block = right block of block
            slot = 0; // update slot = 0
            continue;  // continue to the beginning of this while loop
        }

        // if slot is free skip the loop
        // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
        if(slotmap[slot] == SLOT_UNOCCUPIED)
        {
            slot = slot+1;// increment slot and continue to the next record slot
            continue;
        }
        
        AttrCatEntry attrCatBuf;
        AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatBuf);
        // compare record's attribute value to the the given attrVal as below:
        /*
            firstly get the attribute offset for the attrName attribute
            from the attribute cache entry of the relation using
            AttrCacheTable::getAttrCatEntry()
        */

        /* use the attribute offset to get the value of the attribute from
           current record */
        int attrOffset = attrCatBuf.offset;
        int cmpVal = compareAttrs(record[attrOffset],attrVal,attrCatBuf.attrType);  // will store the difference between the attributes
        // set cmpVal using compareAttrs()

        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)
            */
            RecId newRecId = {block,slot};
            RelCacheTable::setSearchIndex(relId,&newRecId);

            return RecId{block, slot};
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}
int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){

    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute newRelationName;
    strcpy(newRelationName.sVal,newName);
    RecId searchIndex = BlockAccess::linearSearch(RELCAT_RELID,(char*)RELCAT_ATTR_RELNAME,newRelationName,EQ);
    if(searchIndex.block != -1 && searchIndex.slot!= -1) return E_RELEXIST;
    
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute oldRelationName;    // set oldRelationName with oldName
    strcpy(oldRelationName.sVal,oldName);
    searchIndex = BlockAccess::linearSearch(RELCAT_RELID,(char*)RELCAT_ATTR_RELNAME,oldRelationName,EQ);
    if(searchIndex.block == -1 && searchIndex.slot == -1) return E_RELNOTEXIST;

    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    Attribute RelCatRecord[RELCAT_NO_ATTRS];
    RecBuffer RelCatBuf (RELCAT_BLOCK);
    RelCatBuf.getRecord(RelCatRecord,searchIndex.slot);
    strcpy(RelCatRecord[RELCAT_REL_NAME_INDEX].sVal,newName);
    RelCatBuf.setRecord(RelCatRecord,searchIndex.slot);
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    for(int i=0;i<RelCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;i++){
        searchIndex = BlockAccess::linearSearch(ATTRCAT_RELID,(char*)ATTRCAT_ATTR_RELNAME,oldRelationName,EQ);
        RecBuffer AttrCatBlock (searchIndex.block);
        Attribute AttrCatRecord[ATTRCAT_NO_ATTRS];
        AttrCatBlock.getRecord(AttrCatRecord,searchIndex.slot);

        strcpy(AttrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,newName);
        AttrCatBlock.setRecord(AttrCatRecord,searchIndex.slot);
    }
    cout << "HelloWorld";
    return SUCCESS;
}


int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr;
    strcpy(relNameAttr.sVal,relName);
    RecId searchIndex = BlockAccess::linearSearch(RELCAT_RELID,(char*)RELCAT_ATTR_RELNAME,relNameAttr,EQ);
    if(searchIndex.block == -1 && searchIndex.slot == -1) return E_RELNOTEXIST;
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    RecId attrToRenameRecId;
    attrToRenameRecId.block = -1;
    attrToRenameRecId.slot = -1;
    /* iterate over all Attribute Catalog Entry record corresponding to the
       relation to find the required attribute */
    while (true) {
        // linear search on the attribute catalog for RelName = relNameAttr
        searchIndex = BlockAccess::linearSearch(ATTRCAT_RELID,(char*)ATTRCAT_ATTR_RELNAME,relNameAttr,EQ);

        if(searchIndex.block == -1 && searchIndex.slot == -1) break;
        // if there are no more attributes left to check (linearSearch returned {-1,-1})
        //     break;
        RecBuffer AttrBuf(searchIndex.block);
        Attribute attr[ATTRCAT_NO_ATTRS];
        AttrBuf.getRecord(attr,searchIndex.slot);

        /* Get the record from the attribute catalog using RecBuffer.getRecord
          into attrCatEntryRecord */
        if(!strcmp(attr[ATTRCAT_ATTR_NAME_INDEX].sVal,oldName)){ 
            attrToRenameRecId.block = searchIndex.block;
            attrToRenameRecId.slot = searchIndex.slot;
        } 
        // if attrCatEntryRecord.attrName = oldName
        //     attrToRenameRecId = block and slot of this record
        if(!strcmp(attr[ATTRCAT_ATTR_NAME_INDEX].sVal,newName)){ 
            return E_ATTREXIST;
        } 
        // if attrCatEntryRecord.attrName = newName
        //     return E_ATTREXIST;
    }
    if(attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1) return E_ATTRNOTEXIST;
    // if attrToRenameRecId == {-1, -1}
    //     return E_ATTRNOTEXIST;

    RecBuffer temp(attrToRenameRecId.block);
    Attribute atr[ATTRCAT_NO_ATTRS];
    temp.getRecord(atr,attrToRenameRecId.slot);
    strcpy(atr[ATTRCAT_ATTR_NAME_INDEX].sVal,newName);
    temp.setRecord(atr,attrToRenameRecId.slot);
    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    //   update the AttrName of the record with newName
    //   set back the record with RecBuffer.setRecord

    return SUCCESS;
}