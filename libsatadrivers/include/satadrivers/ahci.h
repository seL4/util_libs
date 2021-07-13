/*
 * Copyright 2019, DornerWorks
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <platsupport/io.h>
#include <satadrivers/common.h>

#define SATA_SIG_ATA    0x00000101  // SATA drive
#define SATA_SIG_ATAPI  0xEB140101  // SATAPI drive
#define SATA_SIG_SEMB   0xC33C0101  // Enclosure management bridge
#define SATA_SIG_PM     0x96690101  // Port multiplier

#define HBA_PORT_IPM_NO_DEV       0x0
#define HBA_PORT_IPM_ACTIVE       0x1
#define HBA_PORT_IPM_PARTIAL_PWR  0x2
#define HBA_PORT_IPM_SLUMBER_PWR  0x6
#define HBA_PORT_IPM_DEVSLEEP_PWR 0x8

#define HBA_PORT_DET_NO_DEV          0x0
#define HBA_PORT_DET_PRESENT_NO_COM  0x1
#define HBA_PORT_DET_PRESENT_AND_COM 0x3
#define HBA_PORT_DET_PHY_OFFLINE     0x4

#define HBA_PxCMD_ST    0x0001
#define HBA_PxCMD_FRE   0x0010
#define HBA_PxCMD_FR    0x4000
#define HBA_PxCMD_CR    0x8000

#define HBA_PxIS_TFES   (1 << 30)
#define HBA_PxIS_IFS    (1 << 27)

#define PxSCTL_DET_MASK     0x0000000F
#define PxSCTL_DET_COMRESET 0x00000001

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

/* HOST_CTL bits */
#define GHC_HR_BIT         (1 << 0)  /* reset controller; self-clear */
#define GHC_IRQ_EN_BIT     (1 << 1)  /* global IRQ enable */
#define GHC_MRSM_BIT       (1 << 2)  /* MSI Revert to Single Message */
#define GHC_AHCI_EN_BIT    (1 << 31) /* AHCI enabled */

#define HBA_RESET_DONE 0x00

#define NUM_MAX_PORTS 32

// AHCI memory structure values
#define ENTRIES_PER_CMDLST  32
#define NUM_PRDT_ENTRIES    8

// sizes are in bytes
#define CMD_LIST_SIZE       1024 // commmand list is 1K per port
#define RCV_FIS_SIZE        256
#define CMD_TBL_SIZE        256

#define PRDT_ENTRIES_NEEDED(len) (uint16_t)(((len-1)>>3) + 1)

#define GET_IPM_BITS(val)  ((val >> 8) & 0x0F)
#define GET_DET_BITS(val)  (val & 0x0F)
#define GET_NUM_SLOTS(val) ((val & 0x0f00) >> 8)

#define GET_BYTE0(val) ((val & 0x000000FF) >> 0)
#define GET_BYTE1(val) ((val & 0x0000FF00) >> 8)
#define GET_BYTE2(val) ((val & 0x00FF0000) >> 16)
#define GET_BYTE3(val) ((val & 0xFF000000) >> 24)

#define SECTORS_TO_BYTES(num) ((num<<9)-1) // 512 bytes per sector

#define DATA_BLK_SIZE      4095   // 4K bytes (this value should always be set to 1 less than the actual value)
#define BYTES_PER_PRDT     4096
#define SECTORS_PER_PRDT   8

#define TIMEOUT_10S 10000
#define TIMEOUT_1S  1000

#define PxSERR_RESERVED_BITS(x) ({ typeof(x) __x = (x); \
            (((__x) >= 2 && (__x) <= 7) || ((__x) >= 12 && (__x) <= 15)); })

// AHCI driver error codes
enum ahci_error_codes {
    AHCI_NO_ERR,
    AHCI_NOT_ENABLED_ERR,
    AHCI_CMDLST_FULL_ERR,
    AHCI_CMD_NOT_FOUND_ERR,
    AHCI_CMD_FAILED_ERR,
    AHCI_PORT_HUNG_ERR,
    AHCI_READ_DISK_ERR,
    AHCI_NULL_PTR_ERR,
    AHCI_INVALID_NUM_ERR,
    AHCI_MEM_SIZE_ERR,
    AHCI_CONFIG_ERR,
    AHCI_TIMEOUT_ERR
};

typedef struct ahci_intel_config {
    void *bar0;
    void *clb;
    void *ctba;
    void *fb;
    void *data;
    uint32_t clb_size;
    uint32_t ctba_size;
    uint32_t fb_size;
    uint32_t data_size;
} ahci_intel_config_t;

int ahci_init(ps_io_ops_t *io_ops, void *config);
int ahci_exec_cmd(uint8_t command, uint8_t drive, uint32_t lba, uint16_t count, uint8_t *buf);
