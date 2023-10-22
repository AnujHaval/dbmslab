#include "OpenRelTable.h"
#include <cstring>
#include<stdlib.h>
#include <bits/stdc++.h>
using namespace std;
OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];
AttrCacheEntry *createAttrCacheEntryList(int size)
{
	AttrCacheEntry *head = nullptr, *curr = nullptr;
	head = curr = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
	size--;
	while (size--)
	{
		curr->next = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
		curr = curr->next;
	}
	curr->next = nullptr;

	return head;
}
OpenRelTable::OpenRelTable() {
  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
    OpenRelTable::tableMetaInfo[i].free = true;
  }

  /************ Setting up Relation Cache entries ************/
  // (we need to populate relation cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Relation Cache Table****/
  RecBuffer relCatBlock(RELCAT_BLOCK);

  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

  /**** setting up Attribute Catalog relation in the Relation Cache Table ****/
  // set up the relation cache entry for the attribute catalog similarly
  // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT
    RecBuffer relBlock(RELCAT_BLOCK);
    Attribute relRecord[RELCAT_NO_ATTRS];
    relBlock.getRecord(relRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
    
    struct RelCacheEntry relEntry;
    RelCacheTable::recordToRelCatEntry(relRecord, &relEntry.relCatEntry);
    relEntry.recId.block = RELCAT_BLOCK;
    relEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

  // set the value at RelCacheTable::relCache[ATTRCAT_RELID]
    RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = relEntry;

  /************ Setting up tableMetaInfo entries ************/

  // in the tableMetaInfo array
  //   set free = false for RELCAT_RELID and ATTRCAT_RELID
  //   set relname for RELCAT_RELID and ATTRCAT_RELID

    tableMetaInfo[RELCAT_RELID].free = false;
    strcpy(tableMetaInfo[RELCAT_RELID].relName ,RELCAT_RELNAME);
    tableMetaInfo[ATTRCAT_RELID].free = false;
    strcpy(tableMetaInfo[ATTRCAT_RELID].relName,ATTRCAT_RELNAME);

    /*----------------------------------------------------------------------------------------------------------------------------------*/

  /************ Setting up Attribute cache entries ************/
  // (we need to populate attribute cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
  RecBuffer attrCatBlock(ATTRCAT_BLOCK);
  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

   struct AttrCacheEntry* head = NULL;
   for(int i=RELCAT_NO_ATTRS-1; i>=0; i--){
  	AttrCacheEntry* attrrelEntry=(AttrCacheEntry*) malloc (sizeof(AttrCacheEntry));
  	attrCatBlock.getRecord(attrCatRecord, i);
  	AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrrelEntry->attrCatEntry);
  	attrrelEntry->recId.block = ATTRCAT_BLOCK;
  	attrrelEntry->recId.slot = i;
  	attrrelEntry->next=head;
  	head=attrrelEntry;
  }
  AttrCacheTable::attrCache[RELCAT_RELID] = head;


  /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/
   AttrCacheEntry* attrhead=NULL;
   for(int i=11; i>=6; i--){
    	AttrCacheEntry* AttrEntry=(AttrCacheEntry*) malloc (sizeof(AttrCacheEntry));
  	  attrCatBlock.getRecord(attrCatRecord, i);
  	  AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&AttrEntry->attrCatEntry);
  	  AttrEntry->recId.block = ATTRCAT_BLOCK;
  	  AttrEntry->recId.slot = i;
  	  AttrEntry->next=attrhead;
  	  attrhead=AttrEntry;
   }
    AttrCacheTable::attrCache[ATTRCAT_RELID] = attrhead;
/*----------------------------------------------------------------------------------------------------------------------------------*/
}
int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {
  for(int i=0;i<MAX_OPEN;i++){
    if(!strcmp(OpenRelTable::tableMetaInfo[i].relName,relName) && OpenRelTable::tableMetaInfo[i].free == false) return i;
  }
  return E_RELNOTOPEN;
}
int OpenRelTable::getFreeOpenRelTableEntry() {
  for(int i=0;i<MAX_OPEN;i++){
    if(tableMetaInfo[i].free == true) return i;
  }
  return E_CACHEFULL;
}
int OpenRelTable::openRel(char relName[ATTR_SIZE]) {
  int relId = OpenRelTable::getRelId(relName);
  if(relId>=0){
    // (checked using OpenRelTable::getRelId())
      return relId;
    // return that relation id;
  }

    relId = OpenRelTable::getFreeOpenRelTableEntry();
  if (relId == E_CACHEFULL){
    return E_CACHEFULL;
  }

    Attribute attrVal;
    strcpy(attrVal.sVal,relName);
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

  // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.
  char errorhand [] = "RelName";
  RecId relcatRecId=BlockAccess::linearSearch(RELCAT_RELID,errorhand,attrVal,EQ);

  if (relcatRecId.block == -1 &&  relcatRecId.slot == -1) {
    // (the relation is not found in the Relation Catalog.)
    return E_RELNOTEXIST;
  }

    RelCacheEntry* relcache = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    RecBuffer relbuf(relcatRecId.block);
    Attribute relrecord[RELCAT_NO_ATTRS];
    relbuf.getRecord(relrecord,relcatRecId.slot);
    RelCacheTable::recordToRelCatEntry(relrecord,&(relcache->relCatEntry));
    relcache->recId.block = relcatRecId.block;
    relcache->recId.slot = relcatRecId.slot;
    RelCacheTable::relCache[relId] = relcache;
   
    AttrCacheEntry* listHead = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    Attribute attrrecord[ATTRCAT_NO_ATTRS];

  AttrCacheEntry* attrcache = NULL;
  AttrCacheEntry* listhead=NULL;
  int numattr = RelCacheTable::relCache[relId]->relCatEntry.numAttrs;
  
  listhead = createAttrCacheEntryList(numattr);
  attrcache=listhead;

  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
  while(numattr--){
      /* let attrcatRecId store a valid record id an entry of the relation, relName,
      in the Attribute Catalog.*/
      RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID,RELCAT_ATTR_RELNAME,attrVal,EQ);
      RecBuffer attrCatBlock(attrcatRecId.block);
      attrCatBlock.getRecord(attrrecord, attrcatRecId.slot);
      AttrCacheTable::recordToAttrCatEntry(attrrecord,&(attrcache->attrCatEntry));
      attrcache->recId.block = attrcatRecId.block;
      attrcache->recId.slot = attrcatRecId.slot;
      
      attrcache = attrcache->next;
  }
  AttrCacheTable::attrCache[relId] = listhead;

  tableMetaInfo[relId].free = false;
  strcpy(tableMetaInfo[relId].relName, relName);
  return relId;
}
int OpenRelTable::closeRel(int relId) {
  if (relId==RELCAT_RELID || relId==ATTRCAT_RELID) {
    return E_NOTPERMITTED;
  }

  if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if (tableMetaInfo[relId].free == true) {
    return E_RELNOTOPEN;
  }

  if (RelCacheTable::relCache[relId]->dirty)
  {
    Attribute record[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&RelCacheTable::relCache[relId]->relCatEntry, record);

    RecId recId = RelCacheTable::relCache[relId]->recId;
    RecBuffer relCatBlock(recId.block);

    relCatBlock.setRecord(record,recId.slot);
  }

  free(RelCacheTable::relCache[relId]);
  AttrCacheEntry* head = AttrCacheTable::attrCache[relId];
  AttrCacheEntry* next = head->next;

  while(next){
    free(head);
    head = next;
    next = next->next;
  }
  // free the memory allocated in the relation and attribute caches which was
  // allocated in the OpenRelTable::openRel() function

  tableMetaInfo[relId].free = true;
  RelCacheTable::relCache[relId] = NULL;
  AttrCacheTable::attrCache[relId] = NULL;
  // update `tableMetaInfo` to set `relId` as a free slot
  // update `relCache` and `attrCache` to set the entry at `relId` to nullptr

  return SUCCESS;
}
OpenRelTable::~OpenRelTable() {
    for (int i = 2; i < MAX_OPEN; ++i) {
      if (!tableMetaInfo[i].free) {
        OpenRelTable::closeRel(i); // we will implement this function later
      }
    }
    /**** Closing the catalog relations in the relation cache ****/

    //releasing the relation cache entry of the attribute catalog        
    if (RelCacheTable::relCache[ATTRCAT_RELID]->dirty) {
        Attribute AttrRecord[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry,AttrRecord);
        /* Get the Relation Catalog entry from RelCacheTable::relCache
        Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
        
        // declaring an object of RecBuffer class to write back to the buffer
        RecBuffer relCatBlock(RelCacheTable::relCache[ATTRCAT_RELID]->recId.block);
        relCatBlock.setRecord(AttrRecord,RelCacheTable::relCache[ATTRCAT_RELID]->recId.slot);
        // Write back to the buffer using relCatBlock.setRecord() with recId.slot
    }
    // free the memory dynamically allocated to this RelCacheEntry


    //releasing the relation cache entry of the relation catalog

    if (RelCacheTable::relCache[RELCAT_RELID]->dirty) {
        Attribute RelRecord[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&RelCacheTable::relCache[RELCAT_RELID]->relCatEntry,RelRecord);
        /* Get the Relation Catalog entry from RelCacheTable::relCache
        Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
        
        // declaring an object of RecBuffer class to write back to the buffer
        RecBuffer relCatBlock(RelCacheTable::relCache[RELCAT_RELID]->recId.block);
        relCatBlock.setRecord(RelRecord,RelCacheTable::relCache[RELCAT_RELID]->recId.slot);
        // Write back to the buffer using relCatBlock.setRecord() with recId.slot
    }
    // free the memory dynamically allocated for this RelCacheEntry
  free(RelCacheTable::relCache[RELCAT_RELID]);
  free(RelCacheTable::relCache[ATTRCAT_RELID]);
  
  AttrCacheEntry* eraser = AttrCacheTable::attrCache[RELCAT_RELID];
  AttrCacheEntry* temp;
  while(eraser != NULL){
    temp = eraser;
    eraser = eraser->next;
    free(temp);
  }
  eraser = AttrCacheTable::attrCache[ATTRCAT_RELID];

  while(eraser != NULL){
    temp = eraser;
    eraser = eraser->next;
    free(temp);
  }
    // free the memory allocated for the attribute cache entries of the
    // relation catalog and the attribute catalog
} 
