/*
 * ataio.h
 *
 *  Created on: Dec 29th, 2019
 *      Author: phamsodiep
 */
#ifndef ATAIO_H_
#define ATAIO_H_


#include <netinet/in.h>
#include "aoeprotocol.h"


#define ATA_SECTOR_BYTES       (512)
#define ATA_MAXLBA28SIZE       (0x0fffffff)
#define ATA_MAXLBA48SIZE       (0x0000ffffffffffffLL)

#define ATA_IDENT_CMD          (0xec)
#define ATA_READ_SEC_CMD       (0x20)
#define ATA_WRITE_SEC_CMD      (0x30)
#define ATA_READ_SEC_EX_CMD    (0x24)
#define ATA_WRITE_SEC_EX_CMD   (0x34)

#define ATA_DRDY_STAT          (1 << 6)
#define ATA_ERR_STAT           (1)

#define ATA_ABRT_ERR           (1 << 2)


typedef struct ata_tag
{
  aoehdr_t h;
  u_int8_t aflag;
  u_int8_t err;
  u_int8_t sectors;
  u_int8_t cmd;
  u_int8_t lba[6];
  u_int8_t resvd[2];
} __attribute__((packed)) ata_t;

typedef struct vblkdev_tag
{
  int fd;
  long long size;
} vblkdev_t;


int atainit(char const *imageFileName, int maj, int min);
int ataprocess(ata_t* msg, int msgLen);


#endif /* ATAIO_H_ */

