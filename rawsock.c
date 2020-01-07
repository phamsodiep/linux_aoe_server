/*
 * rawsock.c
 *
 *  Created on: Dec 15th, 2019
 *      Author: phamsodiep
 */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "rawsock.h"


int openInRSocket(const char* devName, rsocket_t* rsock) {
  int i;
  int sockfd;
  struct ifreq ifopts;
  int sockopt;


  // 1. Create socket which interested packet of type ETH_P_ALL
  if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
    perror("listener: socket");
    return 0;
  }

  // 2. Set sockopt so that could receive all packet
  strncpy(ifopts.ifr_name, devName, IFNAMSIZ - 1);
  ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
  ifopts.ifr_flags |= IFF_PROMISC;
  ioctl(sockfd, SIOCSIFFLAGS, &ifopts);

  // 3. Get hardware address
  ioctl(sockfd, SIOCGIFHWADDR, &ifopts);

  // 4. Allow multiple socket open the same address (share)
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
    perror("setsockopt");
    close(sockfd);
    return 0;
  }

  // 5. Bind socket to the target device
  if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, devName, IFNAMSIZ - 1) == -1)	{
    perror("SO_BINDTODEVICE");
    close(sockfd);
    return 0;
  }

  // 6. Store result into socket structure
  rsock->fd = sockfd;
  for (i = 0; i < 6; i++) {
    rsock->mac[i] = ifopts.ifr_addr.sa_data[i];
  }

  return 1;
}


int openOutRSocket(const char* devName, rsocket_t* rsock) {
  int i;
  int sockfd;
  struct ifreq if_idx;
  struct ifreq if_mac;


  // 1. Create socket which interested packet of type ETH_P_ALL
  if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
    perror("sender: socket");
  }

  // 2. Retrieve interface index by its name
  memset(&if_idx, 0, sizeof(struct ifreq));
  strncpy(if_idx.ifr_name, devName, IFNAMSIZ - 1);
  if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0) {
    perror("SIOCGIFINDEX");
    return 0;
  }

  // 3. Retrieve MAC address by its name
  memset(&if_mac, 0, sizeof(struct ifreq));
  strncpy(if_mac.ifr_name, devName, IFNAMSIZ - 1);
  if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0) {
    perror("SIOCGIFHWADDR");
    return 0;
  }

  // 4. Store result into socket structure
  rsock->fd = sockfd;
  rsock->ifIdx = if_idx.ifr_ifindex;
  for (i = 0; i < 6; i++) {
    rsock->mac[i] = if_mac.ifr_addr.sa_data[i];
  }

  return 1;
}


int datalinkFrameRead(rsocket_t* rsock, datalink_header_t* buf, int bufSize) {
  static uint8_t BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  // Check if packet is send to me
  int bytesRead = recvfrom(rsock->fd, buf, bufSize, 0, NULL, NULL);
  int isForMe = strncmp((char*) (buf->ether_dhost), (char*) BROADCAST, 6) == 0;
  if (!isForMe) {
    isForMe = strncmp((char*) (buf->ether_dhost), (char*) rsock->mac, 6) == 0;
  }

#ifdef VERBOSE_TRACE
  printf(
    "(%d) %02X %02X %02X %02X %02X %02X\n",
    isForMe,
    buf->ether_dhost[0],
    buf->ether_dhost[1],
    buf->ether_dhost[2],
    buf->ether_dhost[3],
    buf->ether_dhost[4],
    buf->ether_dhost[5]
  );
#endif

  return isForMe ? bytesRead : -255;
}


int datalinkFrameWrite(rsocket_t* rsock, datalink_header_t* buf, int requestedByte) {
  int i;
  struct sockaddr_ll socket_address;

  
  socket_address.sll_ifindex = rsock->ifIdx;

  socket_address.sll_halen = ETH_ALEN;
  socket_address.sll_addr[0] = buf->ether_dhost[0];
  socket_address.sll_addr[1] = buf->ether_dhost[1];
  socket_address.sll_addr[2] = buf->ether_dhost[2];
  socket_address.sll_addr[3] = buf->ether_dhost[3];
  socket_address.sll_addr[4] = buf->ether_dhost[4];
  socket_address.sll_addr[5] = buf->ether_dhost[5];

  for (i = 0; i < 6; i++) {
    buf->ether_shost[i] = rsock->mac[i];
  }


  int result = sendto(
    rsock->fd,
    buf, requestedByte,
    0,
    (struct sockaddr*)&socket_address,
    sizeof(struct sockaddr_ll)
  );
  if (result < 0) {
    return -255;
  }

  return result;
}

