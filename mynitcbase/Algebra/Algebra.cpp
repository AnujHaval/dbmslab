#include "Algebra.h"
#include <cstring>
#include <bits/stdc++.h>
using namespace std;

bool isNumber(char *str) {
  int len;
  float ignore;
  int ret = sscanf(str, "%f %n", &ignore, &len);
  return ret == 1 && len == strlen(str);
}

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
  int srcRelId = OpenRelTable::getRelId(srcRel);   
  if (srcRelId == E_RELNOTOPEN) return E_RELNOTOPEN;
  AttrCatEntry attrCatEntry;
  int attrEntry = AttrCacheTable::getAttrCatEntry(srcRelId,attr,&attrCatEntry);
  if(attrEntry == E_ATTRNOTEXIST) return attrEntry;

  int type = attrCatEntry.attrType;
  Attribute attrVal;
  if (type == NUMBER) {
    if (isNumber(strVal)) {       
      attrVal.nVal = atof(strVal);
    } 
    else {
      return E_ATTRTYPEMISMATCH;
    }
  } else if (type == STRING) {
    strcpy(attrVal.sVal, strVal);
  }
    RelCatEntry srcRelCat;
    RelCacheTable::getRelCatEntry(srcRelId,&srcRelCat);
    int src_nAttrs = srcRelCat.numAttrs;

    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_types[src_nAttrs];
  for(int i=0;i<src_nAttrs;i++){
    AttrCatEntry srcAttrCat;
    AttrCacheTable::getAttrCatEntry(srcRelId,i,&srcAttrCat);
    strcpy(attr_names[i],srcAttrCat.attrName);
    attr_types[i] = srcAttrCat.attrType;
  }
    int retVal = Schema::createRel(targetRel,src_nAttrs,attr_names,attr_types);
    if(retVal != SUCCESS) return retVal;
    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId < 0  || targetRelId >= MAX_OPEN){
      Schema::deleteRel(targetRel);
      return targetRelId;
    }
    Attribute record[src_nAttrs];
    RelCacheTable::resetSearchIndex(srcRelId);
    AttrCacheTable::resetSearchIndex(srcRelId,attr);

    while (BlockAccess::search(srcRelId,record,attr,attrVal,op) == SUCCESS) {
        retVal = BlockAccess::insert(targetRelId,record);
        if(retVal != SUCCESS){
          Schema::closeRel(targetRel);
          Schema::deleteRel(targetRel);
          return retVal;
        }
    }
    Schema::closeRel(targetRel);
    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) {
    int srcRelId = OpenRelTable::getRelId(srcRel);/*srcRel's rel-id (use OpenRelTable::getRelId() function)*/
    if(srcRelId<0 || srcRelId>=MAX_OPEN) return E_RELNOTOPEN;
    RelCatEntry srcRelCat;
    RelCacheTable::getRelCatEntry(srcRelId,&srcRelCat);
    
    // RecBuffer TestBuffer(srcRelCat.firstBlk);
    // HeadInfo testhead;
    // TestBuffer.getHeader(&testhead);
    // while(testhead.rblock != -1){
    //   cout << testhead.rblock << endl;
    //   RecBuffer TestBuffer(testhead.rblock);
    //   TestBuffer.getHeader(&testhead); 
    // }

    // return SUCCESS;
    
    int src_nAttrs = srcRelCat.numAttrs;
    int attroffset[tar_nAttrs];
    int attr_types[tar_nAttrs];

    for(int i=0;i<tar_nAttrs;i++){
      AttrCatEntry srcAttrCat;
      int retVal = AttrCacheTable::getAttrCatEntry(srcRelId,tar_Attrs[i],&srcAttrCat);
      if(retVal == E_ATTRNOTEXIST) return E_ATTRNOTEXIST;
      attroffset[i] = srcAttrCat.offset;
      attr_types[i] = srcAttrCat.attrType;
    }

    /*** Creating and opening the target relation ***/
    int retVal = Schema::createRel(targetRel,tar_nAttrs,tar_Attrs,attr_types);
    if(retVal != SUCCESS) return retVal;
    int tarRelId = OpenRelTable::openRel(targetRel);
    if(tarRelId < 0 || tarRelId >= MAX_OPEN){
      Schema::deleteRel(targetRel);
      return tarRelId;
    }
    
    RelCacheTable::resetSearchIndex(srcRelId);
    Attribute record[src_nAttrs];

    while (BlockAccess::project(srcRelId,record) == SUCCESS) {
        // the variable `record` will contain the next record
        Attribute proj_record[tar_nAttrs];
        for(int i=0;i<tar_nAttrs;i++){
          proj_record[i] = record[attroffset[i]];
        }
        retVal = BlockAccess::insert(tarRelId,proj_record);
        if (retVal != SUCCESS) {
          Schema::closeRel(targetRel);
          Schema::deleteRel(targetRel);
          return retVal;
        }
    }
    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) {
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if(srcRelId<0 || srcRelId >= MAX_OPEN) return E_RELNOTOPEN;
    RelCatEntry srcRelCat;
    RelCacheTable::getRelCatEntry(srcRelId,&srcRelCat);
    int numAttrs = srcRelCat.numAttrs;

    char attrNames[numAttrs][ATTR_SIZE];
    int attrTypes[numAttrs];
    
    for(int i=0;i<numAttrs;i++){
      AttrCatEntry srcAttrCat;
      AttrCacheTable::getAttrCatEntry(srcRelId,i,&srcAttrCat);
      strcpy(attrNames[i],srcAttrCat.attrName);
      attrTypes[i] = srcAttrCat.attrType;
    }
    int retVal = Schema::createRel(targetRel,numAttrs,attrNames,attrTypes);
    if(retVal != SUCCESS) return retVal;
    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId < 0  || targetRelId >= MAX_OPEN){
      Schema::deleteRel(targetRel);
      return targetRelId;
    }
    RelCacheTable::resetSearchIndex(srcRelId);
    Attribute record[numAttrs];
    while (BlockAccess::project(srcRelId, record) == SUCCESS)
    {
        // record will contain the next record
          retVal = BlockAccess::insert(targetRelId,record);
        // ret = BlockAccess::insert(targetRelId, proj_record);

        if (retVal!=SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return retVal;
        }
    }
    Schema::closeRel(targetRel);
    return SUCCESS;
}

int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE]){
    // if relName is equal to "RELATIONCAT" or "ATTRIBUTECAT"
    // return E_NOTPERMITTED;
    if(!strcmp(relName,RELCAT_RELNAME) || !strcmp(relName,ATTRCAT_RELNAME)) return E_NOTPERMITTED;

    int relId = OpenRelTable::getRelId(relName);
    if(relId<0 || relId>=MAX_OPEN) return E_RELNOTOPEN;

    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(relId,&relCatBuf);
    
    if(relCatBuf.numAttrs != nAttrs) return E_NATTRMISMATCH;

    Attribute recVals[nAttrs];

    for(int i=0;i<nAttrs;i++){
      AttrCatEntry attrCatEntry;
      AttrCacheTable::getAttrCatEntry(relId,i,&attrCatEntry);
      int type = attrCatEntry.attrType;
      if(type == NUMBER){
        if(isNumber(record[i])) recVals[i].nVal = atof (record[i]);
        else return E_ATTRTYPEMISMATCH;
      }
      else if(type==STRING){
        strcpy(recVals[i].sVal, record[i]);
      }
    }
    int retVal = BlockAccess::insert(relId,recVals);
    return retVal;
}

//OLD ALGEBRA SELECT

// int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
//   int srcRelId = OpenRelTable::getRelId(srcRel);   
//   //cout<<srcRelId;   // we'll implement this later
//   if (srcRelId == E_RELNOTOPEN) {
//     return E_RELNOTOPEN;
//   }

//   AttrCatEntry attrCatEntry;
//   int attrEntry = AttrCacheTable::getAttrCatEntry(srcRelId,attr,&attrCatEntry);
//   if(attrEntry == E_ATTRNOTEXIST) return attrEntry;

//   /*** Convert strVal (string) to an attribute of data type NUMBER or STRING ***/
//   int type = attrCatEntry.attrType;
//   Attribute attrVal;
//   if (type == NUMBER) {
//     if (isNumber(strVal)) {       // the isNumber() function is implemented below
//       attrVal.nVal = atof(strVal);
//     } 
//     else {
//       return E_ATTRTYPEMISMATCH;
//     }
//   } else if (type == STRING) {
//     strcpy(attrVal.sVal, strVal);
//   }

//   /*** Selecting records from the source relation ***/

//   // Before calling the search function, reset the search to start from the first hit
//   // using RelCacheTable::resetSearchIndex()
//   RelCacheTable::resetSearchIndex(srcRelId);
//   RelCatEntry relCatEntry;
//   int relEntry = RelCacheTable::getRelCatEntry(srcRelId,&relCatEntry);
//     // get relCatEntry using RelCacheTable::getRelCatEntry()

//   /************************
//   The following code prints the contents of a relation directly to the output
//   console. Direct console output is not permitted by the actual the NITCbase
//   specification and the output can only be inserted into a new relation. We will
//   be modifying it in the later stages to match the specification.
//   ************************/

//   printf("|");
//   for (int i = 0; i < relCatEntry.numAttrs; ++i) {
//     AttrCatEntry attrCatEntry;
//     AttrCacheTable::getAttrCatEntry(srcRelId,i,&attrCatEntry);
//     // get attrCatEntry at offset i using AttrCacheTable::getAttrCatEntry()

//     printf(" %s |", attrCatEntry.attrName);
//   }
//   printf("\n");

//   while (true) {
//     RecId searchRes = BlockAccess::linearSearch(srcRelId, attr, attrVal, op);

//     if (searchRes.block != -1 && searchRes.slot != -1) {
//         RecBuffer blockbuf(searchRes.block);
//         HeadInfo head;
//         blockbuf.getHeader(&head);
//         Attribute record[head.numAttrs];
//         blockbuf.getRecord(record,searchRes.slot);
//         for(int i=0;i<head.numAttrs;i++){
//             AttrCacheTable::getAttrCatEntry(srcRelId,i,&attrCatEntry);
//             if(attrCatEntry.attrType == NUMBER) printf("%d |",(int)record[i].nVal);
//             else printf("%s |",record[i].sVal);
//         }
//         printf("\n");
//       // get the record at searchRes using BlockBuffer.getRecord

//       // print the attribute values in the same format as above

//     } else {

//       // (all records over)
//       break;
//     }
//   }

//   return SUCCESS;
// }