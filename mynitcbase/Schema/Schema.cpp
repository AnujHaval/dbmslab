#include "Schema.h"
#include <cmath>
#include <cstring>
#include<bits/stdc++.h>
using namespace std;
int Schema::openRel(char relName[ATTR_SIZE]) {
    int ret = OpenRelTable::openRel(relName);

    // the OpenRelTable::openRel() function returns the rel-id if successful
    // a valid rel-id will be within the range 0 <= relId < MAX_OPEN and any
    // error codes will be negative
    if(ret >= 0){
        return SUCCESS;
    }
    //otherwise it returns an error message
    return ret;
}
int Schema::closeRel(char relName[ATTR_SIZE]) {
    if (!strcmp(relName,RELCAT_RELNAME) || !strcmp(relName,ATTRCAT_RELNAME)) {
        return E_NOTPERMITTED;
    }

    // this function returns the rel-id of a relation if it is open or
    // E_RELNOTOPEN if it is not. we will implement this later.
    int relId = OpenRelTable::getRelId(relName);

    if (relId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    return OpenRelTable::closeRel(relId);
}
int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]) {
    if(!strcmp(oldRelName,RELCAT_RELNAME) || !strcmp(oldRelName,ATTRCAT_RELNAME) || !strcmp(newRelName,RELCAT_RELNAME) || !strcmp(newRelName,ATTRCAT_RELNAME)) return E_NOTPERMITTED;
    if(OpenRelTable::getRelId(oldRelName)!=E_RELNOTOPEN) return E_RELOPEN;
    int retVal = BlockAccess::renameRelation(oldRelName,newRelName);
    return retVal;
}
int Schema::renameAttr(char *relName, char *oldAttrName, char *newAttrName) {
    if(!strcmp(relName,RELCAT_RELNAME) || !strcmp(relName,ATTRCAT_RELNAME)) return E_NOTPERMITTED;
    if(OpenRelTable::getRelId(relName)!=E_RELNOTOPEN) return E_RELOPEN;
    int retVal = BlockAccess::renameAttribute(relName,oldAttrName,newAttrName);
    return retVal;
}
int Schema::deleteRel(char *relName) {
  if(!strcmp(relName,RELCAT_RELNAME) || !strcmp(relName,ATTRCAT_RELNAME)) return E_NOTPERMITTED;
    int relId = OpenRelTable::getRelId(relName);
    if(relId>=0 && relId<MAX_OPEN) return E_RELOPEN;
    int retVal = BlockAccess::deleteRelation(relName);
    return retVal;
}
int Schema::createRel(char relName[],int nAttrs, char attrs[][ATTR_SIZE],int attrtype[]){
    Attribute relNameAsAttribute;
    strcpy(relNameAsAttribute.sVal,(const char*)relName);
    RecId targetRelId;  
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    targetRelId = BlockAccess::linearSearch(RELCAT_RELID,RELCAT_ATTR_RELNAME,relNameAsAttribute,EQ);
    if(targetRelId.block != -1 && targetRelId.slot != -1) return E_RELEXIST;
    for(int i=0;i<nAttrs-1;i++){
      for(int j=i+1;j<nAttrs;j++){
        if(!strcmp(attrs[i],attrs[j])) return E_DUPLICATEATTR;
      }
    }
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    
    // fill relCatRecord fields as given below
    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,relName);
    relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal =  nAttrs;
    relCatRecord[RELCAT_NO_RECORDS_INDEX].nVal = 0;
    relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal = -1;
    relCatRecord[RELCAT_LAST_BLOCK_INDEX].nVal = -1;
    relCatRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = floor(((2016*1.00) / (16 * nAttrs + 1)));    // (number of slots is calculated as specified in the physical layer docs)
    
    int retVal = BlockAccess::insert(RELCAT_RELID,relCatRecord);
    if(retVal!=SUCCESS) return retVal;
    
    for(int i=0;i<nAttrs;i++){
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,relName);
        strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,attrs[i]);
        attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal=attrtype[i];
        attrCatRecord[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = -1;
        attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal = -1;
        attrCatRecord[ATTRCAT_OFFSET_INDEX].nVal = i;
        
        retVal = BlockAccess::insert(ATTRCAT_RELID,attrCatRecord);
        
        // printf("Exited insert\n");
        if(retVal!=SUCCESS){
          Schema::deleteRel(relName);
          return E_DISKFULL;
        } 
    }
    return SUCCESS;
}

int Schema::createIndex(char relName[ATTR_SIZE],char attrName[ATTR_SIZE]){
    if(!strcmp(relName,RELCAT_RELNAME) || !strcmp(relName,ATTRCAT_RELNAME)) return E_NOTPERMITTED;
    int relId = OpenRelTable::getRelId(relName);
    if(relId == E_RELNOTOPEN) return E_RELNOTOPEN;
    return BPlusTree::bPlusCreate(relId, attrName);
}
int Schema::dropIndex(char *relName, char *attrName) {
    if(!strcmp(relName,RELCAT_RELNAME) || !strcmp(relName,ATTRCAT_RELNAME)) return E_NOTPERMITTED;
    int relId = OpenRelTable::getRelId(relName);
    if(relId == E_RELNOTOPEN) return E_RELNOTOPEN;
    // if relation is not open in open relation table, return E_RELNOTOPEN
    // (check if the value returned from getRelId function call = E_RELNOTOPEN)


    AttrCatEntry attrCatBuf;
    int ret = AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatBuf);
    // get the attribute catalog entry corresponding to the attribute
    // using AttrCacheTable::getAttrCatEntry()
    if(ret != SUCCESS) return ret;
    // if getAttrCatEntry() fails, return E_ATTRNOTEXIST

    int rootBlock = attrCatBuf.rootBlock/* get the root block from attrcat entry */;

    if (rootBlock == -1) {
        return E_NOINDEX;
    }

    // destroy the bplus tree rooted at rootBlock using BPlusTree::bPlusDestroy()
    BPlusTree::bPlusDestroy(rootBlock);
    attrCatBuf.rootBlock = -1;
    AttrCacheTable::setAttrCatEntry(relId,attrName,&attrCatBuf);
    // set rootBlock = -1 in the attribute cache entry of the attribute using
    // AttrCacheTable::setAttrCatEntry()

    return SUCCESS;
}