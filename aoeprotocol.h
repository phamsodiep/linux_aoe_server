/*
 * aoeprotocol.h
 *
 *  Created on: Dec 29th, 2019
 *      Author: phamsodiep
 */
#ifndef AOEPROTOCOL_H_
#define AOEPROTOCOL_H_


#include <netinet/in.h>


#define AOE_ATA_CMD    (0)
#define AOE_BAD_CMD    (1)
#define AOE_ERROR      (1<<2)
#define AOE_F_RESP     (1<<3)


typedef struct aoehdr_tag {
  u_int8_t dst[6];
  u_int8_t src[6];
  u_int16_t type;
  u_int8_t flags;
  u_int8_t error;
  u_int16_t maj;
  u_int8_t min;
  u_int8_t cmd;
  u_int8_t tag[4];
} __attribute__((packed)) aoehdr_t;


int aoeprocess(aoehdr_t* packet, int packetSize, int maj, int min);


#endif /* AOEPROTOCOL_H_ */

