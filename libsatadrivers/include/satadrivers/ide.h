/*
 * Copyright 2019, DornerWorks
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <platsupport/io.h>
#include <satadrivers/common.h>

// Channels:
#define ATA_PRIMARY     0x00
#define ATA_SECONDARY   0x01

// Directions:
#define ATA_READ        0x00
#define ATA_WRITE       0x01
#define ATA_IDENTIFY    0x02

#define ATA_MASTER      0x00
#define ATA_SLAVE       0x01

#define DMA_DISABLE     0x00
#define DMA_ENABLE      0x01

// IDE Status
#define ATA_SR_BSY      0x80    // Busy
#define ATA_SR_DRDY     0x40    // Drive ready
#define ATA_SR_DF       0x20    // Drive write fault
#define ATA_SR_DSC      0x10    // Drive seek complete
#define ATA_SR_DRQ      0x08    // Data request ready
#define ATA_SR_CORR     0x04    // Corrected data
#define ATA_SR_IDX      0x02    // Inlex
#define ATA_SR_ERR      0x01    // Error

// IDE Errors
#define ATA_ER_BBK      0x80    // Bad sector
#define ATA_ER_UNC      0x40    // Uncorrectable data
#define ATA_ER_MC       0x20    // No media
#define ATA_ER_IDNF     0x10    // ID mark not found
#define ATA_ER_MCR      0x08    // No media
#define ATA_ER_ABRT     0x04    // Command aborted
#define ATA_ER_TK0NF    0x02    // Track 0 not found
#define ATA_ER_AMNF     0x01    // No address mark

// IDE Identification offsets
#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

// IDE Commands
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

// IDE Command Registers
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT   0x02
#define ATA_REG_LBA_LO     0x03
#define ATA_REG_LBA_MID    0x04
#define ATA_REG_LBA_HI     0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07

#define SET_CHS 0xA0
#define SET_LBA 0xE0

// IDE CONTROL BITS
#define ATA_CTRL_NIEN      0x02
#define ATA_CTRL_SRST      0x04
#define ATA_CTRL_HOB       0x80

// IDE Control Registers
#define ATA_REG_CONTROL    0x02
#define ATA_REG_ALTSTATUS  0x02

#define PRIMARY_CMD_BASE   0x01F0
#define PRIMARY_CTRL_BASE  0x03F4

#define DELAY_CYCLES       10000000

#define ATA_ERR            2
#define ATA_DRIVE_FAULT    1
#define ATA_DRQ_UNSET      3

#define ATA_WORDS          256 /* Almost every ATA drive has a sector-size of 512-byte. */

#define BUFFER_ID_SPACE    4096

#define PART_OFFSET 0x01BE
#define PART_SIZE 0x10
#define PART_ENTRIES 4

// Error Numbers
#define ATA_NO_ERR          0
#define NO_MEDIA_ERR        3
#define INV_ADDR_ERR        7
#define WRITE_PROTECT_ERR   8
#define BAD_SECT_ERR        13
#define DEV_FAULT_ERR       19
#define CMD_ABORT_ERR       20
#define ID_NOT_FOUND_ERR    21
#define UNCORRECT_DATA_ERR  22
#define READ_NOTHING_ERR    23

#define LBA28_MAX_SIZE 0x10000000
#define LBA_48_MODE 2
#define LBA_28_MODE 1
#define LBA_CHS_MODE 0

#define NUM_IDE_CHANNELS  2

#define NUM_SECT_PER_TRACK 63
#define NUM_LBA_BYTES 6

#define IDENT_LBA48_SUPPORT_BIT (1u << 26)

static inline void sata_delay(unsigned int delay)
{
    while (delay-- > 0) {
        __asm__ volatile("" : "+g" (delay) : :);
    }
}

int ide_init(ps_io_ops_t *io_ops);
uint8_t ide_ata_access(uint8_t direction, uint8_t drive, uint32_t lba, uint16_t numsects, uint8_t *buf);
uint8_t ide_print_error(uint32_t drive, uint8_t err);
