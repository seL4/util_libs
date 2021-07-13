/*
 * Copyright 2019, DornerWorks
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <satadrivers/gen_config.h>

#define SATA_BLK_SIZE          512

#define MAX_DRIVES        4

#define MODEL_STRING_SIZE 41

enum sata_errors {
    SATA_NO_ERR,
    DRIVE_NOT_FOUND,
    INVALID_POSITION,
    INVALID_TYPE,
    INVALID_DIRECTION,
    INVALID_PTR
};

enum drive_types {
    DEV_NULL,
    DEV_SATA,
    DEV_SATAPI,
    DEV_SEMB,
    DEV_PM,
    DEV_NUM_DRIVE_TYPES,
};

typedef struct partition_table {
    uint8_t boot;
    uint8_t head;
    uint16_t sec_cyl;
    uint8_t sys_id;
    uint8_t end_head;
    uint16_t end_sec_cyl;
    uint32_t start_lba;
    uint32_t num_sectors;
} partition_table_t;

typedef struct sata_device {
    uint8_t      Reserved;    // 0 (Empty) or 1 (This Drive really exists).
    uint8_t      Channel;     // 0 (Primary Channel) or 1 (Secondary Channel).
    uint8_t      Drive;       // 0 (Master Drive) or 1 (Slave Drive).
    uint16_t     Type;        // 0: ATA, 1:ATAPI.
    uint16_t     Signature;   // Drive Signature
    uint16_t     Capabilities;// Features.
    uint32_t     CommandSets; // Command Sets Supported.
    uint32_t     Size;        // Size in Sectors.
    uint8_t      Model[MODEL_STRING_SIZE];   // Model in string.
    ps_io_ops_t *io_ops;
} sata_dev_t;

extern sata_dev_t sata_devices[MAX_DRIVES];

int sata_get_partition_tables(uint8_t drive, partition_table_t *partition_tables, uint8_t *part_data);
int sata_read_sectors(uint8_t drive, uint16_t numsects, uint32_t lba, uint8_t *buf);
int sata_write_sectors(uint8_t drive, uint16_t numsects, uint32_t lba, uint8_t *buf);
int sata_init(ps_io_ops_t *io_ops, void *config);
