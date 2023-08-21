#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <bits/stdc++.h>
using namespace std;
int main(int argc, char *argv[]) {
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;
  for(int i=0;i<3;i++){
    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(i,&relCatBuf);
    printf("Relation: %s\n", relCatBuf.relName);
    for(int j=0;j<relCatBuf.numAttrs;j++){
      AttrCatEntry* attrCatBuf= (AttrCatEntry*)malloc(sizeof(AttrCatEntry));
      AttrCacheTable::getAttrCatEntry(i,j,attrCatBuf);
      if(!strcmp(attrCatBuf->relName,"Students") && !strcmp(attrCatBuf->relName,"Class")){
          cout << "error" << endl;
          unsigned char temp[2048];
          Disk::readBlock(temp,5);
          int offset = attrCatBuf->offset;
          int k = 52 + 16*offset;
          char mess[] = "Batch";
          memcpy(temp+k,mess,5);
          Disk::writeBlock(temp,5);
      } 
      AttrCacheTable::getAttrCatEntry(i,j,attrCatBuf);
      if(!attrCatBuf->attrType) printf("  %s: NUM\n", attrCatBuf->attrName);
      else printf("  %s: STR\n", attrCatBuf->attrName);
    }
  }
  return 0;

  // unsigned char buffer[BLOCK_SIZE];
  // Disk::readBlock(buffer, 0);
  // for(int i=0;i<20;i++){
  //   printf("%d ",(int)buffer[i]);
  // }


}
//return FrontendInterface::handleFrontend(argc, argv);

