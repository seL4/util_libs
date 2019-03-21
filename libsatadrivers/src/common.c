/*
 * Copyright 2019, DornerWorks
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <satadrivers/ide.h>
#include <satadrivers/common.h>
#include <satadrivers/ahci.h>

static int sata_access_sectors(sata_driver_t *driver, uint8_t direction, uint8_t drive, uint16_t numsects,
                               uint32_t lba, uint8_t *buf);

/*
 * Purpose: get the partition tables for the inputted drive
 *
 * Inputs:
 *   - drive: drive to read information from
 *   - *partition_tables: where to copy the partition data
 *
 * Returns: success or failure code
 *
 */
int sata_get_partition_tables(sata_driver_t *driver, uint8_t drive, partition_table_t *partition_tables,
                              uint8_t *part_data)
{
    int err;

    ZF_LOGV("SATA: Reading partition Tables for Drive: %u", drive);

    if (NULL == part_data) {
        ZF_LOGE("SATA: ERROR: part data buffer is null");
        return INVALID_PTR;
    }

    /* Read 1 Sector from the top of the drive to get the partition table */
    err = sata_access_sectors(driver, ATA_READ, drive, 1, 0, part_data);

    /* Check for errors */
    if (err != 0) {
        ZF_LOGE("SATA: ERROR: %d", err);
        return err;
    }

    /* Get & print partition information
     * NOTE: This assumes MBR */
    for (int i = 0; i < PART_ENTRIES; i++) {
        memcpy(&partition_tables[i], &part_data[PART_OFFSET + (PART_SIZE * i)], sizeof(partition_table_t));

        ZF_LOGV("Partition %i:", i);
        ZF_LOGV("\tStart: %u, Sectors: %u", partition_tables[i].start_lba,
                partition_tables[i].num_sectors);
        ZF_LOGV("\thead: %u, end_head: %u", partition_tables[i].head,
                partition_tables[i].end_head);
        ZF_LOGV("\tsec_cyl: %u, end_sec_cyl: %u", partition_tables[i].sec_cyl,
                partition_tables[i].end_sec_cyl);
        ZF_LOGV("\tBoot: %u, System ID: 0x%X", partition_tables[i].boot,
                partition_tables[i].sys_id);
    }
    return 0;
}

/*
 * Purpose: Read the sector of the drives
 *
 * Inputs:
 *   - drive: drive to read information from
 *   - numsects: number of sectors to be read
 *   - lba: address which allows us to access disks up to 2TB.
 *   - *buf: Buffer to write or read data from
 *
 * Returns: success or failure code
 *
 */
int sata_read_sectors(sata_driver_t *driver, uint8_t drive, uint16_t numsects, uint32_t lba, uint8_t *buf)
{
    return sata_access_sectors(driver, ATA_READ, drive, numsects, lba, buf);
}

/*
 * Purpose: Write the sector of the drives
 *
 * Inputs:
 *   - drive: drive to write information to
 *   - numsects: number of sectors to write
 *   - lba: address which allows us to access disks up to 2TB.
 *   - *buf: Buffer to write or read data from
 *
 * Returns: success or failure code
 *
 */
int sata_write_sectors(sata_driver_t *driver, uint8_t drive, uint16_t numsects, uint32_t lba, uint8_t *buf)
{
    return sata_access_sectors(driver, ATA_WRITE, drive, numsects, lba, buf);
}
/*
 * Purpose: Read/Write the sector of the drives
 *
 * Inputs:
 *   - direction: used to specify a read or a write
 *   - drive: drive to read from or write to
 *   - numsects: number of sectors to be read or write
 *   - lba: address which allows us to access disks up to 2TB.
 *   - *buf: Buffer to write or read data from
 *
 * Returns: success or failure code
 *
 */
static int sata_access_sectors(sata_driver_t *driver, uint8_t direction, uint8_t drive, uint16_t numsects, uint32_t lba,
                               uint8_t *buf)
{
    int err = SATA_NO_ERR;

    if (NULL == driver) {
        ZF_LOGE("SATA: ERROR: sata_driver is null");
        err = INVALID_PTR;
        return err;
    }

    if (NULL == buf) {
        ZF_LOGE("SATA: ERROR: buffer is null");
        err = INVALID_PTR;
        return err;
    }

    /* Check if the drive presents */
    else if (drive > (MAX_DRIVES - 1) || (0 == driver->sata_devices[drive].Reserved)) {
        err = DRIVE_NOT_FOUND;
    }

    /*Check if inputs are valid */
    else if ((lba + numsects) > driver->sata_devices[drive].Size) {
        err = INVALID_POSITION;
    }

    /*Check if device type is valid */
    else if (DEV_SATA != driver->sata_devices[drive].Type) {
        err = INVALID_TYPE;
    } else {
        if ((direction == ATA_READ) || (direction == ATA_WRITE)) {
            if (driver->mode == AHCI) {
                err = ahci_exec_cmd(driver, direction, drive, lba, numsects, buf);
            } else {
                err = ide_ata_access(driver, direction, drive, lba, numsects, buf);
                err = ide_print_error(driver, drive, err);
            }
        } else {
            err = INVALID_DIRECTION;
        }
    }
    return err;
}

/*
 * Purpose: Initialize the SATA Device
 *
 * Inputs:
 *   - io_ops: IO Operation Functions
 *   - *config: pointer to the driver config structure
 *
 * Returns: success or failure code
 *
 */
int sata_init(ps_io_ops_t *io_ops, sata_driver_t *driver, enum driver_mode mode, void *config)
{
    int err = SATA_NO_ERR;
    if ((NULL == io_ops) || (NULL == driver)) {
        ZF_LOGE("SATA INIT: ERROR: parameters are null");
        return INVALID_PTR;
    }

    if (mode == AHCI && NULL == config) {
        ZF_LOGE("SATA_INIT: config cannot be NULL for AHCI initialisation");
        return INVALID_PTR;
    }

    driver->mode = mode;
    if (mode == AHCI) {
        err = ahci_init(io_ops, driver, config);
    } else {
        err = ide_init(io_ops, driver);
    }

    return err;
}
