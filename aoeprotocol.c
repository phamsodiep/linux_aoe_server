/*
 * aoeprotocol.c
 *
 *  Created on: Dec 29th, 2019
 *      Author: phamsodiep
 */
#include "aoeprotocol.h"
#include "ataio.h"


int aoeprocess(aoehdr_t* packet, int packetSize, int maj, int min) {
  ata_t* msg = (ata_t*) packet;

  // Process ATA CMD
  if (packet->cmd == AOE_ATA_CMD) {
    if (packetSize < sizeof(ata_t)) {
      return 0;
    }
    packetSize = ataprocess(msg, packetSize);
    if (packetSize > 0) {
      msg->h.flags |= AOE_F_RESP;
      msg->h.maj = htons(maj);
      msg->h.min = min;
    }
  }
  // Process unsupported cmds
  else {
    packet->error = AOE_BAD_CMD;
    packet->flags |= AOE_F_RESP | AOE_ERROR;
    packetSize = 0;
  }
  return packetSize;
}

