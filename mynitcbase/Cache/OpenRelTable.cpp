#include "OpenRelTable.h"
#include <cstring>
#include<stdlib.h>
#include <iostream>
using namespace std;
OpenRelTable::OpenRelTable() {

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
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

    /*----------------------------------------------------------------------------------------------------------------------------------*/

    /**** setting up Student relation in the Relation Cache Table ****/     // EXERCISE QUESTION
    RecBuffer studentBlock(6);
    Attribute stuRecord[4];
    
    relBlock.getRecord(stuRecord, 2);
    
    struct RelCacheEntry stuEntry;
    RelCacheTable::recordToRelCatEntry(stuRecord, &stuEntry.relCatEntry);
    stuEntry.recId.block = RELCAT_BLOCK;
    stuEntry.recId.slot = 2;

  // set the value at RelCacheTable::relCache[2]
    RelCacheTable::relCache[2] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[2]) = stuEntry;

    /*----------------------------------------------------------------------------------------------------------------------------------*/

  /************ Setting up Attribute cache entries ************/
  // (we need to populate attribute cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
  RecBuffer attrCatBlock(ATTRCAT_BLOCK);
  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

   struct AttrCacheEntry* head = NULL;
   for(int i=RELCAT_NO_ATTRS-1; i>=0; i--){
  	struct  AttrCacheEntry* attrrelEntry=(struct AttrCacheEntry*) malloc (sizeof(struct AttrCacheEntry));
  	attrCatBlock.getRecord(attrCatRecord, i);
  	AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrrelEntry->attrCatEntry);
  	attrrelEntry->recId.block = ATTRCAT_BLOCK;
  	attrrelEntry->recId.slot = i;
  	attrrelEntry->next=head;
  	head=attrrelEntry;
  }
  AttrCacheTable::attrCache[RELCAT_RELID] = head;


  /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/
   struct AttrCacheEntry* attrhead=NULL;
   for(int i=11; i>=6; i--){
    //printf("hello");
    	struct AttrCacheEntry* AttrEntry=(struct AttrCacheEntry*) malloc (sizeof(struct AttrCacheEntry));
  	  attrCatBlock.getRecord(attrCatRecord, i);
  	  AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&AttrEntry->attrCatEntry);
  	  AttrEntry->recId.block = ATTRCAT_BLOCK;
  	  AttrEntry->recId.slot = i;
  	  AttrEntry->next=attrhead;
  	  attrhead=AttrEntry;
   }
    AttrCacheTable::attrCache[ATTRCAT_RELID] = attrhead;

/*----------------------------------------------------------------------------------------------------------------------------------*/
  /**** setting up Student relation in the Attribute Cache Table ****/
  struct AttrCacheEntry* stuhead=NULL;
   for(int i=15; i>=12; i--){
    // printf("hello");
    	struct AttrCacheEntry* AttrEntry=(struct AttrCacheEntry*) malloc (sizeof(struct AttrCacheEntry));
  	  attrCatBlock.getRecord(attrCatRecord, i);
  	  AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&AttrEntry->attrCatEntry);
  	  AttrEntry->recId.block = ATTRCAT_BLOCK;
  	  AttrEntry->recId.slot = i;
  	  AttrEntry->next=stuhead;
  	  stuhead=AttrEntry;
   }
    AttrCacheTable::attrCache[2] = stuhead;
/*----------------------------------------------------------------------------------------------------------------------------------*/
}

OpenRelTable::~OpenRelTable() {
  // free all the memory that you allocated in the constructor
  for (int i = 0; i < MAX_OPEN; i++) {
    free(RelCacheTable::relCache[i]);
    free(AttrCacheTable::attrCache[i]);
  }
}