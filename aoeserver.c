/*
 * aoeserver.c
 *
 *  Created on: Dec 29th, 2019
 *      Author: phamsodiep
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "rawsock.h"
#include "aoeprotocol.h"
#include "ataio.h"
#include "aoeserver.h"


static uint8_t packetBuf[PACKET_BUF_SIZE];


int serverProcess(char const *devName, int maj, int min, char const *imageFileName);


int main(int argc, char *argv[]) {
  if (argc != 5) {
    printf("Usage: %s <major> <minor> <net interface> <hdd image>\n", argv[0]);
    return -1;
  }
  return serverProcess(argv[3], atoi(argv[1]), atoi(argv[2]), argv[4]);
}


int serverProcess(char const *devName, int maj, int min, char const *imageFileName) {
  rsocket_t iSock;
  rsocket_t oSock;
  datalink_header_t* pL2 = (datalink_header_t*) packetBuf;
  aoehdr_t* pAoe = (aoehdr_t*) packetBuf;
  int bufSize = 0;


  if (maj > 9 || min > 9) {
    printf("Current version just supports shelf, slot in range of [0..9]\n");
    return -1;
  }

  if (!atainit(imageFileName, maj, min)) {
    printf("Failure to init virutal device: %s\n", imageFileName);
    return -1;
  }
    
  if (openInRSocket(devName, &iSock) <= 0) {
    printf("Failure to open listener socket\n");
    return -1;
  }
  if (openOutRSocket(devName, &oSock) <= 0) {
    printf("Failure to open sender socket\n");
    return -1;
  }

  while(1) {
    bufSize = datalinkFrameRead(&iSock, pL2, PACKET_BUF_SIZE);
    // @TODO: Filter package by source address
    bufSize = aoeprocess(pAoe, bufSize, maj, min);
    if (bufSize > 0) {
      if (bufSize > 60) {
        memcpy(pL2->ether_dhost, pL2->ether_shost, 6);
        pL2->ether_type = AOE_ETHER_TYPE;
        if (datalinkFrameWrite(&oSock, pL2, bufSize) < 0) {
          printf("Send packet error: %s\n", strerror(errno));
        }
      }
    }
  }
  return 0;
}

