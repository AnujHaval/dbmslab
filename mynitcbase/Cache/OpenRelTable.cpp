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
    if(!strcmp(OpenRelTable::tableMetaInfo[i].relName,relName)) return i;
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

  /* find a free slot in the Open Relation Table
     using OpenRelTable::getFreeOpenRelTableEntry(). */
    relId = OpenRelTable::getFreeOpenRelTableEntry();
  if (relId == E_CACHEFULL){
    return E_CACHEFULL;
  }

  // let relId be used to store the free slot.
  // int relId;

  /****** Setting up Relation Cache entry for the relation ******/

  /* search for the entry with relation name, relName, in the Relation Catalog using
      BlockAccess::linearSearch().
      Care should be taken to reset the searchIndex of the relation RELCAT_RELID
      before calling linearSearch().*/
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

      /* read the record entry corresponding to relcatRecId and create a relCacheEntry
      on it using RecBuffer::getRecord() and RelCacheTable::recordToRelCatEntry().
      update the recId field of this Relation Cache entry to relcatRecId.
      use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
    NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
  */

  /****** Setting up Attribute Cache entry for the relation ******/

  // let listHead be used to hold the head of the linked list of attrCache entries.
  AttrCacheEntry* listHead = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
  Attribute attrrecord[ATTRCAT_NO_ATTRS];

  AttrCacheEntry* attrcache = NULL;
  AttrCacheEntry* listhead=NULL;
  int numattr = RelCacheTable::relCache[relId]->relCatEntry.numAttrs;
  
  listhead = createAttrCacheEntryList(numattr);
  // int temp = numattr-1;
  // while(temp){
  //   AttrCacheEntry* current = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
  //   current->next = listhead;
  //   listhead=current;
  //   temp--;
  // }
  attrcache=listhead;

  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
  cout << "JOJO" << endl;
  while(numattr--){
      /* let attrcatRecId store a valid record id an entry of the relation, relName,
      in the Attribute Catalog.*/
      RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID,(char*)RELCAT_ATTR_RELNAME,attrVal,EQ);
      RecBuffer attrCatBlock(attrcatRecId.block);
      attrCatBlock.getRecord(attrrecord, attrcatRecId.slot);
      AttrCacheTable::recordToAttrCatEntry(attrrecord,&(attrcache->attrCatEntry));
      attrcache->recId.block = attrcatRecId.block;
      attrcache->recId.slot = attrcatRecId.slot;
      
      attrcache = attrcache->next;
  }
  cout << "Hello" << endl;
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
}