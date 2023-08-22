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

  // unsigned char block[BLOCK_SIZE];
  // Disk::readBlock(block,0);
  // for(int i=0;i<2048;i++){
  //   if(block[i]!=3) printf("%d ",block[i]);
  // }

  // return 0;
  return FrontendInterface::handleFrontend(argc, argv);
}

