/*
 * rawsock.h
 *
 *  Created on: Dec 15th, 2019
 *      Author: phamsodiep
 */
#ifndef RAWSOCK_H_
#define RAWSOCK_H_


#include <netinet/ether.h>
#include "rawsock.h"


typedef struct tag_rsocket_t {
  int ifIdx;
  int fd;
  char mac[6];
} rsocket_t;

typedef struct datalink_header_tag {
  uint8_t  ether_dhost[ETH_ALEN];                              // 6 bytes
  uint8_t  ether_shost[ETH_ALEN];                              // 6 bytes
  uint16_t ether_type;                                         // 2 bytes
} __attribute__((packed)) datalink_header_t;


int openInRSocket(const char* devName, rsocket_t* rsock);
int openOutRSocket(const char* devName, rsocket_t* rsock);

int datalinkFrameRead(rsocket_t* rsock, datalink_header_t* buf, int bufSize);
int datalinkFrameWrite(rsocket_t* rsock, datalink_header_t* buf, int requestedByte);


#endif /* RAWSOCK_H_ */

