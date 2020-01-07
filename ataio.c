/*
 * ataio.c
 *
 *  Created on: Dec 29th, 2019
 *      Author: phamsodiep
 */
#include <string.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include "ataio.h"


static u_int16_t ident[256];
static vblkdev_t blkdev;


static int openImageFile(char const *imageFileName);
static void setushort(u_int16_t *a, int i, u_int16_t n);


int atainit(char const *imageFileName, int maj, int min) {
  int i;
  long long size;
  u_int8_t* pSize = (u_int8_t*)&size;
  u_int8_t* pStr = (u_int8_t*) (ident + 10);


  //////////////////////
  // Class properties //
  //////////////////////
  // Serial
  memcpy(
    pStr,
    "0.0:debian          ",
    20
  );
  pStr[0] = '0' + maj;
  pStr[2] = '0' + min;
  for (i = 10; i < 10 + 10; i++) {
    ident[i] = htons(ident[i]);
  }

  // Version
  pStr = (u_int8_t*) (ident + 23);
  memcpy(
    pStr,
    "V24     ",
    8
  );
  for (i = 23; i < 23 + 8; i++) {
    ident[i] = htons(ident[i]);
  }  

  // Model number
  pStr = (u_int8_t*) (ident + 27);
  memcpy(
    pStr,
    "Coraid EtherDrive vblade                ",
    40
  );
  for (i = 27; i < 27 + 20; i++) {
    ident[i] = htons(ident[i]);
  }

  // Other registries
	setushort(ident, 47, 0x8000);
	setushort(ident, 49, 0x0200);
	setushort(ident, 50, 0x4000);
	setushort(ident, 83, 0x5400);
	setushort(ident, 84, 0x4000);
	setushort(ident, 86, 0x1400);
	setushort(ident, 87, 0x4000);
	setushort(ident, 93, 0x400b);

  // Open instance image file
  if (!openImageFile(imageFileName)) {
    return 0;
  }

  // Instance properties
  // Little endian algorithms
  size = blkdev.size;
  if (size & ~ATA_MAXLBA28SIZE) {
    size = htonl(ATA_MAXLBA28SIZE);
  }
  else {
    size = htonl(size);
  }
  ident[60] = pSize[4];
  ident[61] = pSize[5];
  ident[62] = pSize[6];  
  ident[63] = pSize[7];

  // setlba48
  size = blkdev.size;
  //@TODO review me!, pointer to short type or char?
  ident[100] = size;
  ident[101] = size >>= 8;
  ident[102] = size >>= 8;
  ident[103] = size >>= 8;
  ident[104] = size >>= 8;
  ident[105] = size >>= 8;

  return 1;
}

int ataprocess(ata_t* msg, int msgLen) {
  // Return number of bytes need to be sent
  u_int8_t* payload = (u_int8_t*) (msg + 1);
  long long lba = 0
    | ((long long) msg->lba[0])
    | (((long long) msg->lba[1]) << 8)
    | (((long long) msg->lba[2]) << (8*2))
    | (((long long) msg->lba[3]) << (8*3))
    | (((long long) msg->lba[4]) << (8*4))
    | (((long long) msg->lba[5]) << (8*5));
  u_int8_t status = 0;
  int res = 0;

  switch (msg->cmd) {
    case ATA_IDENT_CMD:
      if (msg->sectors != 1) {
        printf("Failure to process identify device (unexpected sectors count).\n");
        return -1;
      }
      memcpy(payload, ident, ATA_SECTOR_BYTES);
      msg->err = 0;
      msg->cmd = ATA_DRDY_STAT;
      msg->sectors = 0;
#ifdef VERBOSE_DBG
      printf("ATA_IDENT_CMD processed ok\n");
#endif
      return sizeof(ata_t) + ATA_SECTOR_BYTES;

  case ATA_READ_SEC_CMD:
  case ATA_WRITE_SEC_CMD:
      lba = lba & ATA_MAXLBA28SIZE;
      break;

  case ATA_READ_SEC_EX_CMD:
  case ATA_WRITE_SEC_EX_CMD:
      lba = lba & ATA_MAXLBA48SIZE;
      break;

    default:
      msg->cmd = ATA_DRDY_STAT | ATA_ERR_STAT;
      msg->err = ATA_ABRT_ERR;
      break;
  }

  // Test for packet size limitation
  if (msg->sectors > 2) {
    printf(
      "Io more 2 sectors per request is not support yet since Jumpo frame "
      "is not supported yet.\n"
    );
    return -1;
  }

  // @TODO: Test for lba range
  if (msg->cmd == ATA_READ_SEC_EX_CMD || msg->cmd == ATA_READ_SEC_CMD) {
    res = pread(
      blkdev.fd,
      payload,
      msg->sectors * ATA_SECTOR_BYTES,
      lba * ATA_SECTOR_BYTES
    );
#ifdef VERBOSE_DBG
    printf("ATA_READ_SEC_CMD or ATA_READ_SEC_EX_CMD processed ok\n");
#endif
  }

  if (msg->cmd == ATA_WRITE_SEC_EX_CMD || msg->cmd == ATA_WRITE_SEC_CMD) {
    // @TODO: implement preconditional check:
    //    packet should be big enough to contain the data
    //    if (payload < 512 * p->sectors)
    //    return -1;
    res = pwrite(
      blkdev.fd,
      payload,
      msg->sectors * ATA_SECTOR_BYTES,
      lba * ATA_SECTOR_BYTES
    );
#ifdef VERBOSE_DBG
    printf("ATA_WRITE_SEC_CMD or ATA_WRITE_SEC_EX_CMD processed ok\n");
#endif
  }

  // Not error
  msg->cmd |= ATA_DRDY_STAT;
  msg->err = 0;
  //msg->lba += res; // AOE protocol does not use this value so skip to process
  msgLen =  sizeof(ata_t) + msg->sectors * ATA_SECTOR_BYTES;
  msg->sectors = 0;
  return msgLen;
}

int openImageFile(char const *imageFileName) {
  long long size;
  int res;
  int omode = 2;
  blkdev.fd = open(imageFileName, omode);
  if (blkdev.fd == -1) {
    return 0;
  }
  res = ioctl(blkdev.fd, BLKGETSIZE64, &size);
  if (res == -1) {
    struct stat s;
    res = fstat(blkdev.fd, &s);
    if (res == -1) {
      return 0;
    }
    size = s.st_size;
  }
  blkdev.size = size;
  return 1;
}

static void setushort(u_int16_t *a, int i, u_int16_t n)
{
	u_int8_t *p;

	p = (u_int8_t *)(a+i);
	*p++ = n & 0xff;
	*p++ = n >> 8;
}

