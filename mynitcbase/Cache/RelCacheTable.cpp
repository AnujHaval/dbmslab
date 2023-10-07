#include "RelCacheTable.h"
#include <cstring>
#include <bits/stdc++.h>
using namespace std;
RelCacheEntry* RelCacheTable::relCache[MAX_OPEN];

int RelCacheTable::getRelCatEntry(int relId,RelCatEntry* relCatBuf){
    if(relId<0 || relId>=MAX_OPEN) return E_OUTOFBOUND;
    if(relCache[relId] == NULL) return E_RELNOTOPEN;
    *relCatBuf = relCache[relId]->relCatEntry;
    return SUCCESS;
}

void RelCacheTable::recordToRelCatEntry(union Attribute record[RELCAT_NO_ATTRS],RelCatEntry* relCatEntry){
    strcpy(relCatEntry->relName, record[RELCAT_REL_NAME_INDEX].sVal);
    relCatEntry->numAttrs = (int)record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    
    relCatEntry->numRecs = (int)record[RELCAT_NO_RECORDS_INDEX].nVal;
    relCatEntry->firstBlk = (int)record[RELCAT_FIRST_BLOCK_INDEX].nVal;
    relCatEntry->lastBlk = (int)record[RELCAT_LAST_BLOCK_INDEX].nVal;
    relCatEntry->numSlotsPerBlk = (int)record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal;
}

int RelCacheTable::getSearchIndex(int relId, RecId* searchIndex) {
  if(relId<0 || relId>=MAX_OPEN) return E_OUTOFBOUND;
  if(relCache[relId]==NULL) return E_RELNOTOPEN;
  *searchIndex = relCache[relId]->searchIndex;
  return SUCCESS;
}

int RelCacheTable::setSearchIndex(int relId, RecId* searchIndex) {
  if(relId<0 || relId>=MAX_OPEN) return E_OUTOFBOUND;
  if(relCache[relId]==NULL) return E_RELNOTOPEN;
  relCache[relId]->searchIndex = *searchIndex;
  return SUCCESS;
}

int RelCacheTable::resetSearchIndex(int relId){
  // use setSearchIndex to set the search index to {-1, -1}
  RecId* SearchIndex = (RecId*)malloc(sizeof(RecId));
  SearchIndex->block = -1;
  SearchIndex->slot = -1;
  setSearchIndex(relId,SearchIndex);
  return SUCCESS;
}

int RelCacheTable::setRelCatEntry(int relId, RelCatEntry *relCatBuf) {

  if(relId<0 || relId>=MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if(relCache[relId]==nullptr) { //why not tableMetaInfo??9
    return E_RELNOTOPEN;
  }
  relCache[relId]->relCatEntry.firstBlk = relCatBuf->firstBlk;
  relCache[relId]->relCatEntry.lastBlk = relCatBuf->lastBlk;
  relCache[relId]->relCatEntry.numAttrs = relCatBuf->numAttrs;
  relCache[relId]->relCatEntry.numRecs = relCatBuf->numRecs;
  relCache[relId]->relCatEntry.numSlotsPerBlk = relCatBuf->numSlotsPerBlk;
  strcpy(relCache[relId]->relCatEntry.relName,relCatBuf->relName);
  relCache[relId]->dirty = true;
  return SUCCESS;
}

void RelCacheTable::relCatEntryToRecord(RelCatEntry *relCatEntry, union Attribute record[RELCAT_NO_ATTRS]){
  record[RELCAT_FIRST_BLOCK_INDEX].nVal = relCatEntry->firstBlk;
  record[RELCAT_LAST_BLOCK_INDEX].nVal = relCatEntry->lastBlk;
  record[RELCAT_NO_ATTRIBUTES_INDEX].nVal = relCatEntry->numAttrs;
  record[RELCAT_NO_RECORDS_INDEX].nVal = relCatEntry->numRecs;
  record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = relCatEntry->numSlotsPerBlk;
  strcpy(record[RELCAT_REL_NAME_INDEX].sVal,relCatEntry->relName);
}