#include <endian.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sg_lib.h"
#include "sg_io_linux.h"

/* This program performs a READ_16 command as scsi mid-level support
   16 byte commands from lk 2.4.15

*  Copyright (C) 2001-2018 D. Gilbert
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.

   Invocation: sg_simple16 <scsi_device>

   Version 1.04 (20180218)

*/

#define READ10_REPLY_LEN 512
#define READ10_CMD_LEN 10

#define READ16_REPLY_LEN 512
#define READ16_CMD_LEN 16

#define EBUFF_SZ 256

#pragma pack(push, 1)
typedef struct {
    uint8_t opcode;
    
    uint8_t RDPROTECT:3;
    uint8_t DPO:1;
    uint8_t FUA:1;
    uint8_t RARC:1;
    uint8_t Obsolete:2;

    uint32_t lba;   // big-endian

    uint8_t reserved:3;
    uint8_t GROUP_NUMBER:5;

    uint16_t nb_blocks; // big-endian

    uint8_t CONTROL;
} scsi_read10_t;
#pragma pack(pop)

void dump(const uint8_t * const buf, const uint32_t len) {
    const uint32_t ALIGN_SZIE = 32UL;
    uint32_t i = 0UL;

    while (len > i) {
        if (0 == (i % ALIGN_SZIE)) {
            printf("\n");
        }

        printf("%02X ", buf[i]);

        i++;
    }
    
    printf("\n");
}

int main(int argc, char * argv[])
{
    int sg_fd, k, ok;

    // uint8_t r10_cdb [READ10_CMD_LEN] = {
    //     0x28,       // Opcode
    //     0,          // RDPROTECT | DPO | FUA | RARC | Obsolete | Obsolete
    //     0, 0, 0, 0, // LBA (big-endian)
    //     0,          // Reserved | GROUP NUMBER
    //     0, 1,       // Transfer Length
    //     0           // Control
    // };

    scsi_read10_t r10_cdb = {.opcode = 0x28, .nb_blocks = htobe16(1U)};

    sg_io_hdr_t io_hdr;
    char * file_name = 0;
    uint32_t lba;
    uint16_t nb_blocks;

    char ebuff[EBUFF_SZ];
    uint8_t inBuff[READ10_REPLY_LEN] = {0U};
    uint8_t sense_buffer[32];

    if (4 > argc) {
        printf("Usage: '%s <sg_device> <lba address> <number of blocks>'\n", argv[0]);
        return 1;
    }

    file_name = argv[1];

    if ((sg_fd = open(file_name, O_RDWR)) < 0) {
        snprintf(ebuff, EBUFF_SZ, "%s: error opening file: %s", argv[0], file_name);
        perror(ebuff);
        return 1;
    }
    /* Just to be safe, check we have a new sg device by trying an ioctl */
    if ((ioctl(sg_fd, SG_GET_VERSION_NUM, &k) < 0) || (k < 30000)) {
        printf("%s: %s doesn't seem to be an new sg device\n", argv[0], file_name);
        close(sg_fd);
        return 1;
    }

    /* Prepare READ_10 command */
    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = sizeof(r10_cdb);
    /* io_hdr.iovec_count = 0; */  /* memset takes care of this */
    io_hdr.mx_sb_len = sizeof(sense_buffer);
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = READ10_REPLY_LEN;
    io_hdr.dxferp = inBuff;
    io_hdr.cmdp = (uint8_t *)&r10_cdb;
    io_hdr.sbp = sense_buffer;
    io_hdr.timeout = 2000;      /* 2000 millisecs == 2 seconds */
    /* io_hdr.flags = 0; */     /* take defaults: indirect IO, etc */
    /* io_hdr.pack_id = 0; */
    /* io_hdr.usr_ptr = NULL; */

    lba = strtol(argv[2], (char **)NULL, 10);
    nb_blocks = (uint16_t)strtol(argv[3], (char **)NULL, 10);

    for (uint16_t i = 0; nb_blocks > i; i++) {
        printf("LBA: 0x%08X + %d\n", lba, i);

        r10_cdb.lba = htobe32(lba + i);

        if (ioctl(sg_fd, SG_IO, &io_hdr) < 0) {
            perror("Inquiry SG_IO ioctl error!");
            close(sg_fd);
            return 1;
        }

        /* now for the error processing */
        ok = 0;
        switch (sg_err_category3(&io_hdr)) {
            case SG_LIB_CAT_CLEAN:
                ok = 1;
                break;
            case SG_LIB_CAT_RECOVERED:
                printf("Recovered error on READ_10, continuing\n");
                ok = 1;
                break;
            default: /* won't bother decoding other categories */
                sg_chk_n_print3("READ_10 command error", &io_hdr, 1);
                break;
        }

        if (ok) { /* output result if it is available */
            printf("READ_10 duration=%u millisecs, resid=%d, msg_status=%d\n",
                io_hdr.duration, io_hdr.resid, (int)io_hdr.msg_status);

            dump(inBuff, sizeof(inBuff));
        }
    }

    close(sg_fd);
    return 0;
}
