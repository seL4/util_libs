/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdio.h>
#include <stdint.h>

/*
 * Under PCI, each device has 256 bytes of configuration address space,
 * of which the first 64 bytes are standardized as follows:
 */
#define PCI_STD_HEADER_SIZEOF   64
#define PCI_VENDOR_ID           0x00    /* 16 bits */
#define PCI_DEVICE_ID           0x02    /* 16 bits */
#define PCI_COMMAND             0x04    /* 16 bits */
#define  PCI_COMMAND_IO         0x1     /* Enable response in I/O space */
#define  PCI_COMMAND_MEMORY     0x2     /* Enable response in Memory space */
#define  PCI_COMMAND_MASTER     0x4     /* Enable bus mastering */
#define  PCI_COMMAND_SPECIAL    0x8     /* Enable response to special cycles */
#define  PCI_COMMAND_INVALIDATE 0x10    /* Use memory write and invalidate */
#define  PCI_COMMAND_VGA_PALETTE 0x20   /* Enable palette snooping */
#define  PCI_COMMAND_PARITY     0x40    /* Enable parity checking */
#define  PCI_COMMAND_WAIT       0x80    /* Enable address/data stepping */
#define  PCI_COMMAND_SERR       0x100   /* Enable SERR */
#define  PCI_COMMAND_FAST_BACK  0x200   /* Enable back-to-back writes */
#define  PCI_COMMAND_INTX_DISABLE 0x400 /* INTx Emulation Disable */

#define PCI_STATUS              0x06    /* 16 bits */
#define  PCI_STATUS_INTERRUPT   0x08    /* Interrupt status */
#define  PCI_STATUS_CAP_LIST    0x10    /* Support Capability List */
#define  PCI_STATUS_66MHZ       0x20    /* Support 66 Mhz PCI 2.1 bus */
#define  PCI_STATUS_UDF         0x40    /* Support User Definable Features [obsolete] */
#define  PCI_STATUS_FAST_BACK   0x80    /* Accept fast-back to back */
#define  PCI_STATUS_PARITY      0x100   /* Detected parity error */
#define  PCI_STATUS_DEVSEL_MASK 0x600   /* DEVSEL timing */
#define  PCI_STATUS_DEVSEL_FAST         0x000
#define  PCI_STATUS_DEVSEL_MEDIUM       0x200
#define  PCI_STATUS_DEVSEL_SLOW         0x400
#define  PCI_STATUS_SIG_TARGET_ABORT    0x800 /* Set on target abort */
#define  PCI_STATUS_REC_TARGET_ABORT    0x1000 /* Master ack of " */
#define  PCI_STATUS_REC_MASTER_ABORT    0x2000 /* Set on master abort */
#define  PCI_STATUS_SIG_SYSTEM_ERROR    0x4000 /* Set when we drive SERR */
#define  PCI_STATUS_DETECTED_PARITY     0x8000 /* Set on parity error */

#define PCI_CLASS_REVISION      0x08    /* High 24 bits are class, low 8 revision */
#define PCI_REVISION_ID         0x08    /* Revision ID */
#define PCI_CLASS_PROG          0x09    /* Reg. Level Programming Interface */
#define PCI_CLASS_DEVICE        0x0a    /* Device class */

#define PCI_CACHE_LINE_SIZE     0x0c    /* 8 bits */
#define PCI_LATENCY_TIMER       0x0d    /* 8 bits */
#define PCI_HEADER_TYPE         0x0e    /* 8 bits */
#define  PCI_HEADER_TYPE_NORMAL         0
#define  PCI_HEADER_TYPE_BRIDGE         1
#define  PCI_HEADER_TYPE_CARDBUS        2

#define PCI_BIST                0x0f    /* 8 bits */
#define  PCI_BIST_CODE_MASK     0x0f    /* Return result */
#define  PCI_BIST_START         0x40    /* 1 to start BIST, 2 secs or less */
#define  PCI_BIST_CAPABLE       0x80    /* 1 if BIST capable */

/*
 * Base addresses specify locations in memory or I/O space.
 * Decoded size can be determined by writing a value of
 * 0xffffffff to the register, and reading it back.  Only
 * 1 bits are decoded.
 */
#define PCI_BASE_ADDRESS_0      0x10    /* 32 bits */
#define PCI_BASE_ADDRESS_1      0x14    /* 32 bits [htype 0,1 only] */
#define PCI_BASE_ADDRESS_2      0x18    /* 32 bits [htype 0 only] */
#define PCI_BASE_ADDRESS_3      0x1c    /* 32 bits */
#define PCI_BASE_ADDRESS_4      0x20    /* 32 bits */
#define PCI_BASE_ADDRESS_5      0x24    /* 32 bits */
#define  PCI_BASE_ADDRESS_SPACE         0x01    /* 0 = memory, 1 = I/O */
#define  PCI_BASE_ADDRESS_SPACE_IO      0x01
#define  PCI_BASE_ADDRESS_SPACE_MEMORY  0x00
#define  PCI_BASE_ADDRESS_MEM_TYPE_MASK 0x06
#define  PCI_BASE_ADDRESS_MEM_TYPE_32   0x00    /* 32 bit address */
#define  PCI_BASE_ADDRESS_MEM_TYPE_1M   0x02    /* Below 1M [obsolete] */
#define  PCI_BASE_ADDRESS_MEM_TYPE_64   0x04    /* 64 bit address */
#define  PCI_BASE_ADDRESS_MEM_PREFETCH  0x08    /* prefetchable? */
#define  PCI_BASE_ADDRESS_MEM_MASK      (~0x0fUL)
#define  PCI_BASE_ADDRESS_IO_MASK       (~0x03UL)
/* bit 1 is reserved if address_space = 1 */

/* Header type 0 (normal devices) */
#define PCI_CARDBUS_CIS         0x28
#define PCI_SUBSYSTEM_VENDOR_ID 0x2c
#define PCI_SUBSYSTEM_ID        0x2e
#define PCI_ROM_ADDRESS         0x30    /* Bits 31..11 are address, 10..1 reserved */
#define  PCI_ROM_ADDRESS_ENABLE 0x01
#define PCI_ROM_ADDRESS_MASK    (~0x7ffUL)

#define PCI_CAPABILITY_LIST     0x34    /* Offset of first capability list entry */

/* 0x35-0x3b are reserved */
#define PCI_INTERRUPT_LINE      0x3c    /* 8 bits */
#define PCI_INTERRUPT_PIN       0x3d    /* 8 bits */
#define PCI_MIN_GNT             0x3e    /* 8 bits */
#define PCI_MAX_LAT             0x3f    /* 8 bits */

/* Header type 1 (PCI-to-PCI bridges) */
#define PCI_PRIMARY_BUS         0x18    /* Primary bus number */
#define PCI_SECONDARY_BUS       0x19    /* Secondary bus number */
#define PCI_SUBORDINATE_BUS     0x1a    /* Highest bus number behind the bridge */
#define PCI_SEC_LATENCY_TIMER   0x1b    /* Latency timer for secondary interface */
#define PCI_IO_BASE             0x1c    /* I/O range behind the bridge */
#define PCI_IO_LIMIT            0x1d
#define  PCI_IO_RANGE_TYPE_MASK 0x0fUL  /* I/O bridging type */
#define  PCI_IO_RANGE_TYPE_16   0x00
#define  PCI_IO_RANGE_TYPE_32   0x01
#define  PCI_IO_RANGE_MASK      (~0x0fUL) /* Standard 4K I/O windows */
#define  PCI_IO_1K_RANGE_MASK   (~0x03UL) /* Intel 1K I/O windows */
#define PCI_SEC_STATUS          0x1e    /* Secondary status register, only bit 14 used */
#define PCI_MEMORY_BASE         0x20    /* Memory range behind */
#define PCI_MEMORY_LIMIT        0x22
#define  PCI_MEMORY_RANGE_TYPE_MASK 0x0fUL
#define  PCI_MEMORY_RANGE_MASK  (~0x0fUL)
#define PCI_PREF_MEMORY_BASE    0x24    /* Prefetchable memory range behind */
#define PCI_PREF_MEMORY_LIMIT   0x26
#define  PCI_PREF_RANGE_TYPE_MASK 0x0fUL
#define  PCI_PREF_RANGE_TYPE_32 0x00
#define  PCI_PREF_RANGE_TYPE_64 0x01
#define  PCI_PREF_RANGE_MASK    (~0x0fUL)
#define PCI_PREF_BASE_UPPER32   0x28    /* Upper half of prefetchable memory range */
#define PCI_PREF_LIMIT_UPPER32  0x2c
#define PCI_IO_BASE_UPPER16     0x30    /* Upper half of I/O addresses */
#define PCI_IO_LIMIT_UPPER16    0x32
/* 0x34 same as for htype 0 */
/* 0x35-0x3b is reserved */
#define PCI_ROM_ADDRESS1        0x38    /* Same as PCI_ROM_ADDRESS, but for htype 1 */
/* 0x3c-0x3d are same as for htype 0 */
#define PCI_BRIDGE_CONTROL      0x3e
#define  PCI_BRIDGE_CTL_PARITY  0x01    /* Enable parity detection on secondary interface */
#define  PCI_BRIDGE_CTL_SERR    0x02    /* The same for SERR forwarding */
#define  PCI_BRIDGE_CTL_ISA     0x04    /* Enable ISA mode */
#define  PCI_BRIDGE_CTL_VGA     0x08    /* Forward VGA addresses */
#define  PCI_BRIDGE_CTL_MASTER_ABORT    0x20  /* Report master aborts */
#define  PCI_BRIDGE_CTL_BUS_RESET       0x40    /* Secondary bus reset */
#define  PCI_BRIDGE_CTL_FAST_BACK       0x80    /* Fast Back2Back enabled on secondary interface */

/* Header type 2 (CardBus bridges) */
#define PCI_CB_CAPABILITY_LIST  0x14
/* 0x15 reserved */
#define PCI_CB_SEC_STATUS       0x16    /* Secondary status */
#define PCI_CB_PRIMARY_BUS      0x18    /* PCI bus number */
#define PCI_CB_CARD_BUS         0x19    /* CardBus bus number */
#define PCI_CB_SUBORDINATE_BUS  0x1a    /* Subordinate bus number */
#define PCI_CB_LATENCY_TIMER    0x1b    /* CardBus latency timer */
#define PCI_CB_MEMORY_BASE_0    0x1c
#define PCI_CB_MEMORY_LIMIT_0   0x20
#define PCI_CB_MEMORY_BASE_1    0x24
#define PCI_CB_MEMORY_LIMIT_1   0x28
#define PCI_CB_IO_BASE_0        0x2c
#define PCI_CB_IO_BASE_0_HI     0x2e
#define PCI_CB_IO_LIMIT_0       0x30
#define PCI_CB_IO_LIMIT_0_HI    0x32
#define PCI_CB_IO_BASE_1        0x34
#define PCI_CB_IO_BASE_1_HI     0x36
#define PCI_CB_IO_LIMIT_1       0x38
#define PCI_CB_IO_LIMIT_1_HI    0x3a
#define  PCI_CB_IO_RANGE_MASK   (~0x03UL)
/* 0x3c-0x3d are same as for htype 0 */
#define PCI_CB_BRIDGE_CONTROL   0x3e
#define  PCI_CB_BRIDGE_CTL_PARITY       0x01    /* Similar to standard bridge control register */
#define  PCI_CB_BRIDGE_CTL_SERR         0x02
#define  PCI_CB_BRIDGE_CTL_ISA          0x04
#define  PCI_CB_BRIDGE_CTL_VGA          0x08
#define  PCI_CB_BRIDGE_CTL_MASTER_ABORT 0x20
#define  PCI_CB_BRIDGE_CTL_CB_RESET     0x40    /* CardBus reset */
#define  PCI_CB_BRIDGE_CTL_16BIT_INT    0x80    /* Enable interrupt for 16-bit cards */
#define  PCI_CB_BRIDGE_CTL_PREFETCH_MEM0 0x100  /* Prefetch enable for both memory regions */
#define  PCI_CB_BRIDGE_CTL_PREFETCH_MEM1 0x200
#define  PCI_CB_BRIDGE_CTL_POST_WRITES  0x400
#define PCI_CB_SUBSYSTEM_VENDOR_ID      0x40
#define PCI_CB_SUBSYSTEM_ID             0x42
#define PCI_CB_LEGACY_MODE_BASE         0x44    /* 16-bit PC Card legacy mode base address (ExCa) */
/* 0x48-0x7f reserved */

/* Capability lists */

#define PCI_CAP_LIST_ID         0       /* Capability ID */
#define  PCI_CAP_ID_PM          0x01    /* Power Management */
#define  PCI_CAP_ID_AGP         0x02    /* Accelerated Graphics Port */
#define  PCI_CAP_ID_VPD         0x03    /* Vital Product Data */
#define  PCI_CAP_ID_SLOTID      0x04    /* Slot Identification */
#define  PCI_CAP_ID_MSI         0x05    /* Message Signalled Interrupts */
#define  PCI_CAP_ID_CHSWP       0x06    /* CompactPCI HotSwap */
#define  PCI_CAP_ID_PCIX        0x07    /* PCI-X */
#define  PCI_CAP_ID_HT          0x08    /* HyperTransport */
#define  PCI_CAP_ID_VNDR        0x09    /* Vendor specific */
#define  PCI_CAP_ID_DBG         0x0A    /* Debug port */
#define  PCI_CAP_ID_CCRC        0x0B    /* CompactPCI Central Resource Control */
#define  PCI_CAP_ID_SHPC        0x0C    /* PCI Standard Hot-Plug Controller */
#define  PCI_CAP_ID_SSVID       0x0D    /* Bridge subsystem vendor/device ID */
#define  PCI_CAP_ID_AGP3        0x0E    /* AGP Target PCI-PCI bridge */
#define  PCI_CAP_ID_SECDEV      0x0F    /* Secure Device */
#define  PCI_CAP_ID_EXP         0x10    /* PCI Express */
#define  PCI_CAP_ID_MSIX        0x11    /* MSI-X */
#define  PCI_CAP_ID_SATA        0x12    /* SATA Data/Index Conf. */
#define  PCI_CAP_ID_AF          0x13    /* PCI Advanced Features */
#define  PCI_CAP_ID_MAX         PCI_CAP_ID_AF
#define PCI_CAP_LIST_NEXT       1       /* Next capability in the list */
#define PCI_CAP_FLAGS           2       /* Capability defined flags (16 bits) */
#define PCI_CAP_SIZEOF          4

/* Power Management Registers */

#define PCI_PM_PMC              2       /* PM Capabilities Register */
#define  PCI_PM_CAP_VER_MASK    0x0007  /* Version */
#define  PCI_PM_CAP_PME_CLOCK   0x0008  /* PME clock required */
#define  PCI_PM_CAP_RESERVED    0x0010  /* Reserved field */
#define  PCI_PM_CAP_DSI         0x0020  /* Device specific initialization */
#define  PCI_PM_CAP_AUX_POWER   0x01C0  /* Auxiliary power support mask */
#define  PCI_PM_CAP_D1          0x0200  /* D1 power state support */
#define  PCI_PM_CAP_D2          0x0400  /* D2 power state support */
#define  PCI_PM_CAP_PME         0x0800  /* PME pin supported */
#define  PCI_PM_CAP_PME_MASK    0xF800  /* PME Mask of all supported states */
#define  PCI_PM_CAP_PME_D0      0x0800  /* PME# from D0 */
#define  PCI_PM_CAP_PME_D1      0x1000  /* PME# from D1 */
#define  PCI_PM_CAP_PME_D2      0x2000  /* PME# from D2 */
#define  PCI_PM_CAP_PME_D3      0x4000  /* PME# from D3 (hot) */
#define  PCI_PM_CAP_PME_D3cold  0x8000  /* PME# from D3 (cold) */
#define  PCI_PM_CAP_PME_SHIFT   11      /* Start of the PME Mask in PMC */
#define PCI_PM_CTRL             4       /* PM control and status register */
#define  PCI_PM_CTRL_STATE_MASK 0x0003  /* Current power state (D0 to D3) */
#define  PCI_PM_CTRL_NO_SOFT_RESET      0x0008  /* No reset for D3hot->D0 */
#define  PCI_PM_CTRL_PME_ENABLE 0x0100  /* PME pin enable */
#define  PCI_PM_CTRL_DATA_SEL_MASK      0x1e00  /* Data select (??) */
#define  PCI_PM_CTRL_DATA_SCALE_MASK    0x6000  /* Data scale (??) */
#define  PCI_PM_CTRL_PME_STATUS 0x8000  /* PME pin status */
#define PCI_PM_PPB_EXTENSIONS   6       /* PPB support extensions (??) */
#define  PCI_PM_PPB_B2_B3       0x40    /* Stop clock when in D3hot (??) */
#define  PCI_PM_BPCC_ENABLE     0x80    /* Bus power/clock control enable (??) */
#define PCI_PM_DATA_REGISTER    7       /* (??) */
#define PCI_PM_SIZEOF           8

/* AGP registers */

#define PCI_AGP_VERSION         2       /* BCD version number */
#define PCI_AGP_RFU             3       /* Rest of capability flags */
#define PCI_AGP_STATUS          4       /* Status register */
#define  PCI_AGP_STATUS_RQ_MASK 0xff000000      /* Maximum number of requests - 1 */
#define  PCI_AGP_STATUS_SBA     0x0200  /* Sideband addressing supported */
#define  PCI_AGP_STATUS_64BIT   0x0020  /* 64-bit addressing supported */
#define  PCI_AGP_STATUS_FW      0x0010  /* FW transfers supported */
#define  PCI_AGP_STATUS_RATE4   0x0004  /* 4x transfer rate supported */
#define  PCI_AGP_STATUS_RATE2   0x0002  /* 2x transfer rate supported */
#define  PCI_AGP_STATUS_RATE1   0x0001  /* 1x transfer rate supported */
#define PCI_AGP_COMMAND         8       /* Control register */
#define  PCI_AGP_COMMAND_RQ_MASK 0xff000000  /* Master: Maximum number of requests */
#define  PCI_AGP_COMMAND_SBA    0x0200  /* Sideband addressing enabled */
#define  PCI_AGP_COMMAND_AGP    0x0100  /* Allow processing of AGP transactions */
#define  PCI_AGP_COMMAND_64BIT  0x0020  /* Allow processing of 64-bit addresses */
#define  PCI_AGP_COMMAND_FW     0x0010  /* Force FW transfers */
#define  PCI_AGP_COMMAND_RATE4  0x0004  /* Use 4x rate */
#define  PCI_AGP_COMMAND_RATE2  0x0002  /* Use 2x rate */
#define  PCI_AGP_COMMAND_RATE1  0x0001  /* Use 1x rate */
#define PCI_AGP_SIZEOF          12

/* Vital Product Data */

#define PCI_VPD_ADDR            2       /* Address to access (15 bits!) */
#define  PCI_VPD_ADDR_MASK      0x7fff  /* Address mask */
#define  PCI_VPD_ADDR_F         0x8000  /* Write 0, 1 indicates completion */
#define PCI_VPD_DATA            4       /* 32-bits of data returned here */
#define PCI_CAP_VPD_SIZEOF      8

/* Slot Identification */

#define PCI_SID_ESR             2       /* Expansion Slot Register */
#define  PCI_SID_ESR_NSLOTS     0x1f    /* Number of expansion slots available */
#define  PCI_SID_ESR_FIC        0x20    /* First In Chassis Flag */
#define PCI_SID_CHASSIS_NR      3       /* Chassis Number */

/* Message Signalled Interrupts registers */

#define PCI_MSI_FLAGS           2       /* Message Control */
#define  PCI_MSI_FLAGS_ENABLE   0x0001  /* MSI feature enabled */
#define  PCI_MSI_FLAGS_QMASK    0x000e  /* Maximum queue size available */
#define  PCI_MSI_FLAGS_QSIZE    0x0070  /* Message queue size configured */
#define  PCI_MSI_FLAGS_64BIT    0x0080  /* 64-bit addresses allowed */
#define  PCI_MSI_FLAGS_MASKBIT  0x0100  /* Per-vector masking capable */
#define PCI_MSI_RFU             3       /* Rest of capability flags */
#define PCI_MSI_ADDRESS_LO      4       /* Lower 32 bits */
#define PCI_MSI_ADDRESS_HI      8       /* Upper 32 bits (if PCI_MSI_FLAGS_64BIT set) */
#define PCI_MSI_DATA_32         8       /* 16 bits of data for 32-bit devices */
#define PCI_MSI_MASK_32         12      /* Mask bits register for 32-bit devices */
#define PCI_MSI_PENDING_32      16      /* Pending intrs for 32-bit devices */
#define PCI_MSI_DATA_64         12      /* 16 bits of data for 64-bit devices */
#define PCI_MSI_MASK_64         16      /* Mask bits register for 64-bit devices */
#define PCI_MSI_PENDING_64      20      /* Pending intrs for 64-bit devices */

/* MSI-X registers */
#define PCI_MSIX_FLAGS          2       /* Message Control */
#define  PCI_MSIX_FLAGS_QSIZE   0x07FF  /* Table size */
#define  PCI_MSIX_FLAGS_MASKALL 0x4000  /* Mask all vectors for this function */
#define  PCI_MSIX_FLAGS_ENABLE  0x8000  /* MSI-X enable */
#define PCI_MSIX_TABLE          4       /* Table offset */
#define  PCI_MSIX_TABLE_BIR     0x00000007 /* BAR index */
#define  PCI_MSIX_TABLE_OFFSET  0xfffffff8 /* Offset into specified BAR */
#define PCI_MSIX_PBA            8       /* Pending Bit Array offset */
#define  PCI_MSIX_PBA_BIR       0x00000007 /* BAR index */
#define  PCI_MSIX_PBA_OFFSET    0xfffffff8 /* Offset into specified BAR */
#define  PCI_MSIX_FLAGS_BIRMASK (7 << 0)   /* deprecated */
#define PCI_CAP_MSIX_SIZEOF     12      /* size of MSIX registers */

/* MSI-X entry's format */
#define PCI_MSIX_ENTRY_SIZE             16
#define  PCI_MSIX_ENTRY_LOWER_ADDR      0
#define  PCI_MSIX_ENTRY_UPPER_ADDR      4
#define  PCI_MSIX_ENTRY_DATA            8
#define  PCI_MSIX_ENTRY_VECTOR_CTRL     12
#define   PCI_MSIX_ENTRY_CTRL_MASKBIT   1

/* CompactPCI Hotswap Register */

#define PCI_CHSWP_CSR           2       /* Control and Status Register */
#define  PCI_CHSWP_DHA          0x01    /* Device Hiding Arm */
#define  PCI_CHSWP_EIM          0x02    /* ENUM# Signal Mask */
#define  PCI_CHSWP_PIE          0x04    /* Pending Insert or Extract */
#define  PCI_CHSWP_LOO          0x08    /* LED On / Off */
#define  PCI_CHSWP_PI           0x30    /* Programming Interface */
#define  PCI_CHSWP_EXT          0x40    /* ENUM# status - extraction */
#define  PCI_CHSWP_INS          0x80    /* ENUM# status - insertion */

/* PCI Advanced Feature registers */

#define PCI_AF_LENGTH           2
#define PCI_AF_CAP              3
#define  PCI_AF_CAP_TP          0x01
#define  PCI_AF_CAP_FLR         0x02
#define PCI_AF_CTRL             4
#define  PCI_AF_CTRL_FLR        0x01
#define PCI_AF_STATUS           5
#define  PCI_AF_STATUS_TP       0x01
#define PCI_CAP_AF_SIZEOF       6       /* size of AF registers */

/* PCI-X registers (Type 0 (non-bridge) devices) */

#define PCI_X_CMD               2       /* Modes & Features */
#define  PCI_X_CMD_DPERR_E      0x0001  /* Data Parity Error Recovery Enable */
#define  PCI_X_CMD_ERO          0x0002  /* Enable Relaxed Ordering */
#define  PCI_X_CMD_READ_512     0x0000  /* 512 byte maximum read byte count */
#define  PCI_X_CMD_READ_1K      0x0004  /* 1Kbyte maximum read byte count */
#define  PCI_X_CMD_READ_2K      0x0008  /* 2Kbyte maximum read byte count */
#define  PCI_X_CMD_READ_4K      0x000c  /* 4Kbyte maximum read byte count */
#define  PCI_X_CMD_MAX_READ     0x000c  /* Max Memory Read Byte Count */
/* Max # of outstanding split transactions */
#define  PCI_X_CMD_SPLIT_1      0x0000  /* Max 1 */
#define  PCI_X_CMD_SPLIT_2      0x0010  /* Max 2 */
#define  PCI_X_CMD_SPLIT_3      0x0020  /* Max 3 */
#define  PCI_X_CMD_SPLIT_4      0x0030  /* Max 4 */
#define  PCI_X_CMD_SPLIT_8      0x0040  /* Max 8 */
#define  PCI_X_CMD_SPLIT_12     0x0050  /* Max 12 */
#define  PCI_X_CMD_SPLIT_16     0x0060  /* Max 16 */
#define  PCI_X_CMD_SPLIT_32     0x0070  /* Max 32 */
#define  PCI_X_CMD_MAX_SPLIT    0x0070  /* Max Outstanding Split Transactions */
#define  PCI_X_CMD_VERSION(x)   (((x) >> 12) & 3) /* Version */
#define PCI_X_STATUS            4       /* PCI-X capabilities */
#define  PCI_X_STATUS_DEVFN     0x000000ff      /* A copy of devfn */
#define  PCI_X_STATUS_BUS       0x0000ff00      /* A copy of bus nr */
#define  PCI_X_STATUS_64BIT     0x00010000      /* 64-bit device */
#define  PCI_X_STATUS_133MHZ    0x00020000      /* 133 MHz capable */
#define  PCI_X_STATUS_SPL_DISC  0x00040000      /* Split Completion Discarded */
#define  PCI_X_STATUS_UNX_SPL   0x00080000      /* Unexpected Split Completion */
#define  PCI_X_STATUS_COMPLEX   0x00100000      /* Device Complexity */
#define  PCI_X_STATUS_MAX_READ  0x00600000      /* Designed Max Memory Read Count */
#define  PCI_X_STATUS_MAX_SPLIT 0x03800000      /* Designed Max Outstanding Split Transactions */
#define  PCI_X_STATUS_MAX_CUM   0x1c000000      /* Designed Max Cumulative Read Size */
#define  PCI_X_STATUS_SPL_ERR   0x20000000      /* Rcvd Split Completion Error Msg */
#define  PCI_X_STATUS_266MHZ    0x40000000      /* 266 MHz capable */
#define  PCI_X_STATUS_533MHZ    0x80000000      /* 533 MHz capable */
#define PCI_X_ECC_CSR           8       /* ECC control and status */
#define PCI_CAP_PCIX_SIZEOF_V0  8       /* size of registers for Version 0 */
#define PCI_CAP_PCIX_SIZEOF_V1  24      /* size for Version 1 */
#define PCI_CAP_PCIX_SIZEOF_V2  PCI_CAP_PCIX_SIZEOF_V1  /* Same for v2 */

/* PCI-X registers (Type 1 (bridge) devices) */

#define PCI_X_BRIDGE_SSTATUS    2       /* Secondary Status */
#define  PCI_X_SSTATUS_64BIT    0x0001  /* Secondary AD interface is 64 bits */
#define  PCI_X_SSTATUS_133MHZ   0x0002  /* 133 MHz capable */
#define  PCI_X_SSTATUS_FREQ     0x03c0  /* Secondary Bus Mode and Frequency */
#define  PCI_X_SSTATUS_VERS     0x3000  /* PCI-X Capability Version */
#define  PCI_X_SSTATUS_V1       0x1000  /* Mode 2, not Mode 1 */
#define  PCI_X_SSTATUS_V2       0x2000  /* Mode 1 or Modes 1 and 2 */
#define  PCI_X_SSTATUS_266MHZ   0x4000  /* 266 MHz capable */
#define  PCI_X_SSTATUS_533MHZ   0x8000  /* 533 MHz capable */
#define PCI_X_BRIDGE_STATUS     4       /* Bridge Status */

/* PCI Bridge Subsystem ID registers */

#define PCI_SSVID_VENDOR_ID     4       /* PCI-Bridge subsystem vendor id register */
#define PCI_SSVID_DEVICE_ID     6       /* PCI-Bridge subsystem device id register */

/* PCI Express capability registers */

#define PCI_EXP_FLAGS           2       /* Capabilities register */
#define PCI_EXP_FLAGS_VERS      0x000f  /* Capability version */
#define PCI_EXP_FLAGS_TYPE      0x00f0  /* Device/Port type */
#define  PCI_EXP_TYPE_ENDPOINT  0x0     /* Express Endpoint */
#define  PCI_EXP_TYPE_LEG_END   0x1     /* Legacy Endpoint */
#define  PCI_EXP_TYPE_ROOT_PORT 0x4     /* Root Port */
#define  PCI_EXP_TYPE_UPSTREAM  0x5     /* Upstream Port */
#define  PCI_EXP_TYPE_DOWNSTREAM 0x6    /* Downstream Port */
#define  PCI_EXP_TYPE_PCI_BRIDGE 0x7    /* PCI/PCI-X Bridge */
#define  PCI_EXP_TYPE_PCIE_BRIDGE 0x8   /* PCI/PCI-X to PCIE Bridge */
#define  PCI_EXP_TYPE_RC_END    0x9     /* Root Complex Integrated Endpoint */
#define  PCI_EXP_TYPE_RC_EC     0xa     /* Root Complex Event Collector */
#define PCI_EXP_FLAGS_SLOT      0x0100  /* Slot implemented */
#define PCI_EXP_FLAGS_IRQ       0x3e00  /* Interrupt message number */
#define PCI_EXP_DEVCAP          4       /* Device capabilities */
#define  PCI_EXP_DEVCAP_PAYLOAD 0x07    /* Max_Payload_Size */
#define  PCI_EXP_DEVCAP_PHANTOM 0x18    /* Phantom functions */
#define  PCI_EXP_DEVCAP_EXT_TAG 0x20    /* Extended tags */
#define  PCI_EXP_DEVCAP_L0S     0x1c0   /* L0s Acceptable Latency */
#define  PCI_EXP_DEVCAP_L1      0xe00   /* L1 Acceptable Latency */
#define  PCI_EXP_DEVCAP_ATN_BUT 0x1000  /* Attention Button Present */
#define  PCI_EXP_DEVCAP_ATN_IND 0x2000  /* Attention Indicator Present */
#define  PCI_EXP_DEVCAP_PWR_IND 0x4000  /* Power Indicator Present */
#define  PCI_EXP_DEVCAP_RBER    0x8000  /* Role-Based Error Reporting */
#define  PCI_EXP_DEVCAP_PWR_VAL 0x3fc0000 /* Slot Power Limit Value */
#define  PCI_EXP_DEVCAP_PWR_SCL 0xc000000 /* Slot Power Limit Scale */
#define  PCI_EXP_DEVCAP_FLR     0x10000000 /* Function Level Reset */
#define PCI_EXP_DEVCTL          8       /* Device Control */
#define  PCI_EXP_DEVCTL_CERE    0x0001  /* Correctable Error Reporting En. */
#define  PCI_EXP_DEVCTL_NFERE   0x0002  /* Non-Fatal Error Reporting Enable */
#define  PCI_EXP_DEVCTL_FERE    0x0004  /* Fatal Error Reporting Enable */
#define  PCI_EXP_DEVCTL_URRE    0x0008  /* Unsupported Request Reporting En. */
#define  PCI_EXP_DEVCTL_RELAX_EN 0x0010 /* Enable relaxed ordering */
#define  PCI_EXP_DEVCTL_PAYLOAD 0x00e0  /* Max_Payload_Size */
#define  PCI_EXP_DEVCTL_EXT_TAG 0x0100  /* Extended Tag Field Enable */
#define  PCI_EXP_DEVCTL_PHANTOM 0x0200  /* Phantom Functions Enable */
#define  PCI_EXP_DEVCTL_AUX_PME 0x0400  /* Auxiliary Power PM Enable */
#define  PCI_EXP_DEVCTL_NOSNOOP_EN 0x0800  /* Enable No Snoop */
#define  PCI_EXP_DEVCTL_READRQ  0x7000  /* Max_Read_Request_Size */
#define  PCI_EXP_DEVCTL_BCR_FLR 0x8000  /* Bridge Configuration Retry / FLR */
#define PCI_EXP_DEVSTA          10      /* Device Status */
#define  PCI_EXP_DEVSTA_CED     0x01    /* Correctable Error Detected */
#define  PCI_EXP_DEVSTA_NFED    0x02    /* Non-Fatal Error Detected */
#define  PCI_EXP_DEVSTA_FED     0x04    /* Fatal Error Detected */
#define  PCI_EXP_DEVSTA_URD     0x08    /* Unsupported Request Detected */
#define  PCI_EXP_DEVSTA_AUXPD   0x10    /* AUX Power Detected */
#define  PCI_EXP_DEVSTA_TRPND   0x20    /* Transactions Pending */
#define PCI_EXP_LNKCAP          12      /* Link Capabilities */
#define  PCI_EXP_LNKCAP_SLS     0x0000000f /* Supported Link Speeds */
#define  PCI_EXP_LNKCAP_SLS_2_5GB 0x1   /* LNKCAP2 SLS Vector bit 0 (2.5GT/s) */
#define  PCI_EXP_LNKCAP_SLS_5_0GB 0x2   /* LNKCAP2 SLS Vector bit 1 (5.0GT/s) */
#define  PCI_EXP_LNKCAP_MLW     0x000003f0 /* Maximum Link Width */
#define  PCI_EXP_LNKCAP_ASPMS   0x00000c00 /* ASPM Support */
#define  PCI_EXP_LNKCAP_L0SEL   0x00007000 /* L0s Exit Latency */
#define  PCI_EXP_LNKCAP_L1EL    0x00038000 /* L1 Exit Latency */
#define  PCI_EXP_LNKCAP_CLKPM   0x00040000 /* Clock Power Management */
#define  PCI_EXP_LNKCAP_SDERC   0x00080000 /* Surprise Down Error Reporting Capable */
#define  PCI_EXP_LNKCAP_DLLLARC 0x00100000 /* Data Link Layer Link Active Reporting Capable */
#define  PCI_EXP_LNKCAP_LBNC    0x00200000 /* Link Bandwidth Notification Capability */
#define  PCI_EXP_LNKCAP_PN      0xff000000 /* Port Number */
#define PCI_EXP_LNKCTL          16      /* Link Control */
#define  PCI_EXP_LNKCTL_ASPMC   0x0003  /* ASPM Control */
#define  PCI_EXP_LNKCTL_ASPM_L0S  0x01  /* L0s Enable */
#define  PCI_EXP_LNKCTL_ASPM_L1   0x02  /* L1 Enable */
#define  PCI_EXP_LNKCTL_RCB     0x0008  /* Read Completion Boundary */
#define  PCI_EXP_LNKCTL_LD      0x0010  /* Link Disable */
#define  PCI_EXP_LNKCTL_RL      0x0020  /* Retrain Link */
#define  PCI_EXP_LNKCTL_CCC     0x0040  /* Common Clock Configuration */
#define  PCI_EXP_LNKCTL_ES      0x0080  /* Extended Synch */
#define  PCI_EXP_LNKCTL_CLKREQ_EN 0x100 /* Enable clkreq */
#define  PCI_EXP_LNKCTL_HAWD    0x0200  /* Hardware Autonomous Width Disable */
#define  PCI_EXP_LNKCTL_LBMIE   0x0400  /* Link Bandwidth Management Interrupt Enable */
#define  PCI_EXP_LNKCTL_LABIE   0x0800  /* Lnk Autonomous Bandwidth Interrupt Enable */
#define PCI_EXP_LNKSTA          18      /* Link Status */
#define  PCI_EXP_LNKSTA_CLS     0x000f  /* Current Link Speed */
#define  PCI_EXP_LNKSTA_CLS_2_5GB 0x01  /* Current Link Speed 2.5GT/s */
#define  PCI_EXP_LNKSTA_CLS_5_0GB 0x02  /* Current Link Speed 5.0GT/s */
#define  PCI_EXP_LNKSTA_NLW     0x03f0  /* Nogotiated Link Width */
#define  PCI_EXP_LNKSTA_NLW_SHIFT 4     /* start of NLW mask in link status */
#define  PCI_EXP_LNKSTA_LT      0x0800  /* Link Training */
#define  PCI_EXP_LNKSTA_SLC     0x1000  /* Slot Clock Configuration */
#define  PCI_EXP_LNKSTA_DLLLA   0x2000  /* Data Link Layer Link Active */
#define  PCI_EXP_LNKSTA_LBMS    0x4000  /* Link Bandwidth Management Status */
#define  PCI_EXP_LNKSTA_LABS    0x8000  /* Link Autonomous Bandwidth Status */
#define PCI_CAP_EXP_ENDPOINT_SIZEOF_V1  20      /* v1 endpoints end here */
#define PCI_EXP_SLTCAP          20      /* Slot Capabilities */
#define  PCI_EXP_SLTCAP_ABP     0x00000001 /* Attention Button Present */
#define  PCI_EXP_SLTCAP_PCP     0x00000002 /* Power Controller Present */
#define  PCI_EXP_SLTCAP_MRLSP   0x00000004 /* MRL Sensor Present */
#define  PCI_EXP_SLTCAP_AIP     0x00000008 /* Attention Indicator Present */
#define  PCI_EXP_SLTCAP_PIP     0x00000010 /* Power Indicator Present */
#define  PCI_EXP_SLTCAP_HPS     0x00000020 /* Hot-Plug Surprise */
#define  PCI_EXP_SLTCAP_HPC     0x00000040 /* Hot-Plug Capable */
#define  PCI_EXP_SLTCAP_SPLV    0x00007f80 /* Slot Power Limit Value */
#define  PCI_EXP_SLTCAP_SPLS    0x00018000 /* Slot Power Limit Scale */
#define  PCI_EXP_SLTCAP_EIP     0x00020000 /* Electromechanical Interlock Present */
#define  PCI_EXP_SLTCAP_NCCS    0x00040000 /* No Command Completed Support */
#define  PCI_EXP_SLTCAP_PSN     0xfff80000 /* Physical Slot Number */
#define PCI_EXP_SLTCTL          24      /* Slot Control */
#define  PCI_EXP_SLTCTL_ABPE    0x0001  /* Attention Button Pressed Enable */
#define  PCI_EXP_SLTCTL_PFDE    0x0002  /* Power Fault Detected Enable */
#define  PCI_EXP_SLTCTL_MRLSCE  0x0004  /* MRL Sensor Changed Enable */
#define  PCI_EXP_SLTCTL_PDCE    0x0008  /* Presence Detect Changed Enable */
#define  PCI_EXP_SLTCTL_CCIE    0x0010  /* Command Completed Interrupt Enable */
#define  PCI_EXP_SLTCTL_HPIE    0x0020  /* Hot-Plug Interrupt Enable */
#define  PCI_EXP_SLTCTL_AIC     0x00c0  /* Attention Indicator Control */
#define  PCI_EXP_SLTCTL_PIC     0x0300  /* Power Indicator Control */
#define  PCI_EXP_SLTCTL_PCC     0x0400  /* Power Controller Control */
#define  PCI_EXP_SLTCTL_EIC     0x0800  /* Electromechanical Interlock Control */
#define  PCI_EXP_SLTCTL_DLLSCE  0x1000  /* Data Link Layer State Changed Enable */
#define PCI_EXP_SLTSTA          26      /* Slot Status */
#define  PCI_EXP_SLTSTA_ABP     0x0001  /* Attention Button Pressed */
#define  PCI_EXP_SLTSTA_PFD     0x0002  /* Power Fault Detected */
#define  PCI_EXP_SLTSTA_MRLSC   0x0004  /* MRL Sensor Changed */
#define  PCI_EXP_SLTSTA_PDC     0x0008  /* Presence Detect Changed */
#define  PCI_EXP_SLTSTA_CC      0x0010  /* Command Completed */
#define  PCI_EXP_SLTSTA_MRLSS   0x0020  /* MRL Sensor State */
#define  PCI_EXP_SLTSTA_PDS     0x0040  /* Presence Detect State */
#define  PCI_EXP_SLTSTA_EIS     0x0080  /* Electromechanical Interlock Status */
#define  PCI_EXP_SLTSTA_DLLSC   0x0100  /* Data Link Layer State Changed */
#define PCI_EXP_RTCTL           28      /* Root Control */
#define  PCI_EXP_RTCTL_SECEE    0x01    /* System Error on Correctable Error */
#define  PCI_EXP_RTCTL_SENFEE   0x02    /* System Error on Non-Fatal Error */
#define  PCI_EXP_RTCTL_SEFEE    0x04    /* System Error on Fatal Error */
#define  PCI_EXP_RTCTL_PMEIE    0x08    /* PME Interrupt Enable */
#define  PCI_EXP_RTCTL_CRSSVE   0x10    /* CRS Software Visibility Enable */
#define PCI_EXP_RTCAP           30      /* Root Capabilities */
#define PCI_EXP_RTSTA           32      /* Root Status */
#define PCI_EXP_RTSTA_PME       0x10000 /* PME status */
#define PCI_EXP_RTSTA_PENDING   0x20000 /* PME pending */
/*
* Note that the following PCI Express 'Capability Structure' registers
* were introduced with 'Capability Version' 0x2 (v2).  These registers
* do not exist on devices with Capability Version 1.  Use pci_pcie_cap2()
* to use these fields safely.
*/
#define PCI_EXP_DEVCAP2         36      /* Device Capabilities 2 */
#define  PCI_EXP_DEVCAP2_ARI    0x20    /* Alternative Routing-ID */
#define  PCI_EXP_DEVCAP2_LTR    0x800   /* Latency tolerance reporting */
#define  PCI_EXP_OBFF_MASK      0xc0000 /* OBFF support mechanism */
#define  PCI_EXP_OBFF_MSG       0x40000 /* New message signaling */
#define  PCI_EXP_OBFF_WAKE      0x80000 /* Re-use WAKE# for OBFF */
#define PCI_EXP_DEVCTL2         40      /* Device Control 2 */
#define  PCI_EXP_DEVCTL2_ARI    0x20    /* Alternative Routing-ID */
#define  PCI_EXP_IDO_REQ_EN     0x100   /* ID-based ordering request enable */
#define  PCI_EXP_IDO_CMP_EN     0x200   /* ID-based ordering completion enable */
#define  PCI_EXP_LTR_EN         0x400   /* Latency tolerance reporting */
#define  PCI_EXP_OBFF_MSGA_EN   0x2000  /* OBFF enable with Message type A */
#define  PCI_EXP_OBFF_MSGB_EN   0x4000  /* OBFF enable with Message type B */
#define  PCI_EXP_OBFF_WAKE_EN   0x6000  /* OBFF using WAKE# signaling */
#define PCI_CAP_EXP_ENDPOINT_SIZEOF_V2  44      /* v2 endpoints end here */
#define PCI_EXP_LNKCAP2         44      /* Link Capability 2 */
#define  PCI_EXP_LNKCAP2_SLS_2_5GB 0x02 /* Supported Link Speed 2.5GT/s */
#define  PCI_EXP_LNKCAP2_SLS_5_0GB 0x04 /* Supported Link Speed 5.0GT/s */
#define  PCI_EXP_LNKCAP2_SLS_8_0GB 0x08 /* Supported Link Speed 8.0GT/s */
#define  PCI_EXP_LNKCAP2_CROSSLINK 0x100 /* Crosslink supported */
#define PCI_EXP_LNKCTL2         48      /* Link Control 2 */
#define PCI_EXP_LNKSTA2         50      /* Link Status 2 */
#define PCI_EXP_SLTCTL2         56      /* Slot Control 2 */

/* Extended Capabilities (PCI-X 2.0 and Express) */
#define PCI_EXT_CAP_ID(header)          (header & 0x0000ffff)
#define PCI_EXT_CAP_VER(header)         ((header >> 16) & 0xf)
#define PCI_EXT_CAP_NEXT(header)        ((header >> 20) & 0xffc)

#define PCI_EXT_CAP_ID_ERR      0x01    /* Advanced Error Reporting */
#define PCI_EXT_CAP_ID_VC       0x02    /* Virtual Channel Capability */
#define PCI_EXT_CAP_ID_DSN      0x03    /* Device Serial Number */
#define PCI_EXT_CAP_ID_PWR      0x04    /* Power Budgeting */
#define PCI_EXT_CAP_ID_RCLD     0x05    /* Root Complex Link Declaration */
#define PCI_EXT_CAP_ID_RCILC    0x06    /* Root Complex Internal Link Control */
#define PCI_EXT_CAP_ID_RCEC     0x07    /* Root Complex Event Collector */
#define PCI_EXT_CAP_ID_MFVC     0x08    /* Multi-Function VC Capability */
#define PCI_EXT_CAP_ID_VC9      0x09    /* same as _VC */
#define PCI_EXT_CAP_ID_RCRB     0x0A    /* Root Complex RB? */
#define PCI_EXT_CAP_ID_VNDR     0x0B    /* Vendor Specific */
#define PCI_EXT_CAP_ID_CAC      0x0C    /* Config Access - obsolete */
#define PCI_EXT_CAP_ID_ACS      0x0D    /* Access Control Services */
#define PCI_EXT_CAP_ID_ARI      0x0E    /* Alternate Routing ID */
#define PCI_EXT_CAP_ID_ATS      0x0F    /* Address Translation Services */
#define PCI_EXT_CAP_ID_SRIOV    0x10    /* Single Root I/O Virtualization */
#define PCI_EXT_CAP_ID_MRIOV    0x11    /* Multi Root I/O Virtualization */
#define PCI_EXT_CAP_ID_MCAST    0x12    /* Multicast */
#define PCI_EXT_CAP_ID_PRI      0x13    /* Page Request Interface */
#define PCI_EXT_CAP_ID_AMD_XXX  0x14    /* reserved for AMD */
#define PCI_EXT_CAP_ID_REBAR    0x15    /* resizable BAR */
#define PCI_EXT_CAP_ID_DPA      0x16    /* dynamic power alloc */
#define PCI_EXT_CAP_ID_TPH      0x17    /* TPH request */
#define PCI_EXT_CAP_ID_LTR      0x18    /* latency tolerance reporting */
#define PCI_EXT_CAP_ID_SECPCI   0x19    /* Secondary PCIe */
#define PCI_EXT_CAP_ID_PMUX     0x1A    /* Protocol Multiplexing */
#define PCI_EXT_CAP_ID_PASID    0x1B    /* Process Address Space ID */
#define PCI_EXT_CAP_ID_MAX      PCI_EXT_CAP_ID_PASID

#define PCI_EXT_CAP_DSN_SIZEOF  12
#define PCI_EXT_CAP_MCAST_ENDPOINT_SIZEOF 40

/* Advanced Error Reporting */
#define PCI_ERR_UNCOR_STATUS    4       /* Uncorrectable Error Status */
#define  PCI_ERR_UNC_TRAIN      0x00000001      /* Training */
#define  PCI_ERR_UNC_DLP        0x00000010      /* Data Link Protocol */
#define  PCI_ERR_UNC_SURPDN     0x00000020      /* Surprise Down */
#define  PCI_ERR_UNC_POISON_TLP 0x00001000      /* Poisoned TLP */
#define  PCI_ERR_UNC_FCP        0x00002000      /* Flow Control Protocol */
#define  PCI_ERR_UNC_COMP_TIME  0x00004000      /* Completion Timeout */
#define  PCI_ERR_UNC_COMP_ABORT 0x00008000      /* Completer Abort */
#define  PCI_ERR_UNC_UNX_COMP   0x00010000      /* Unexpected Completion */
#define  PCI_ERR_UNC_RX_OVER    0x00020000      /* Receiver Overflow */
#define  PCI_ERR_UNC_MALF_TLP   0x00040000      /* Malformed TLP */
#define  PCI_ERR_UNC_ECRC       0x00080000      /* ECRC Error Status */
#define  PCI_ERR_UNC_UNSUP      0x00100000      /* Unsupported Request */
#define  PCI_ERR_UNC_ACSV       0x00200000      /* ACS Violation */
#define  PCI_ERR_UNC_INTN       0x00400000      /* internal error */
#define  PCI_ERR_UNC_MCBTLP     0x00800000      /* MC blocked TLP */
#define  PCI_ERR_UNC_ATOMEG     0x01000000      /* Atomic egress blocked */
#define  PCI_ERR_UNC_TLPPRE     0x02000000      /* TLP prefix blocked */
#define PCI_ERR_UNCOR_MASK      8       /* Uncorrectable Error Mask */
/* Same bits as above */
#define PCI_ERR_UNCOR_SEVER     12      /* Uncorrectable Error Severity */
/* Same bits as above */
#define PCI_ERR_COR_STATUS      16      /* Correctable Error Status */
#define  PCI_ERR_COR_RCVR       0x00000001      /* Receiver Error Status */
#define  PCI_ERR_COR_BAD_TLP    0x00000040      /* Bad TLP Status */
#define  PCI_ERR_COR_BAD_DLLP   0x00000080      /* Bad DLLP Status */
#define  PCI_ERR_COR_REP_ROLL   0x00000100      /* REPLAY_NUM Rollover */
#define  PCI_ERR_COR_REP_TIMER  0x00001000      /* Replay Timer Timeout */
#define  PCI_ERR_COR_ADV_NFAT   0x00002000      /* Advisory Non-Fatal */
#define  PCI_ERR_COR_INTERNAL   0x00004000      /* Corrected Internal */
#define  PCI_ERR_COR_LOG_OVER   0x00008000      /* Header Log Overflow */
#define PCI_ERR_COR_MASK        20      /* Correctable Error Mask */
/* Same bits as above */
#define PCI_ERR_CAP             24      /* Advanced Error Capabilities */
#define  PCI_ERR_CAP_FEP(x)     ((x) & 31)      /* First Error Pointer */
#define  PCI_ERR_CAP_ECRC_GENC  0x00000020      /* ECRC Generation Capable */
#define  PCI_ERR_CAP_ECRC_GENE  0x00000040      /* ECRC Generation Enable */
#define  PCI_ERR_CAP_ECRC_CHKC  0x00000080      /* ECRC Check Capable */
#define  PCI_ERR_CAP_ECRC_CHKE  0x00000100      /* ECRC Check Enable */
#define PCI_ERR_HEADER_LOG      28      /* Header Log Register (16 bytes) */
#define PCI_ERR_ROOT_COMMAND    44      /* Root Error Command */
/* Correctable Err Reporting Enable */
#define PCI_ERR_ROOT_CMD_COR_EN         0x00000001
/* Non-fatal Err Reporting Enable */
#define PCI_ERR_ROOT_CMD_NONFATAL_EN    0x00000002
/* Fatal Err Reporting Enable */
#define PCI_ERR_ROOT_CMD_FATAL_EN       0x00000004
#define PCI_ERR_ROOT_STATUS     48
#define PCI_ERR_ROOT_COR_RCV            0x00000001      /* ERR_COR Received */
/* Multi ERR_COR Received */
#define PCI_ERR_ROOT_MULTI_COR_RCV      0x00000002
/* ERR_FATAL/NONFATAL Recevied */
#define PCI_ERR_ROOT_UNCOR_RCV          0x00000004
/* Multi ERR_FATAL/NONFATAL Recevied */
#define PCI_ERR_ROOT_MULTI_UNCOR_RCV    0x00000008
#define PCI_ERR_ROOT_FIRST_FATAL        0x00000010      /* First Fatal */
#define PCI_ERR_ROOT_NONFATAL_RCV       0x00000020      /* Non-Fatal Received */
#define PCI_ERR_ROOT_FATAL_RCV          0x00000040      /* Fatal Received */
#define PCI_ERR_ROOT_ERR_SRC    52      /* Error Source Identification */

/* Virtual Channel */
#define PCI_VC_PORT_REG1        4
#define  PCI_VC_REG1_EVCC       0x7     /* extended vc count */
#define PCI_VC_PORT_REG2        8
#define  PCI_VC_REG2_32_PHASE   0x2
#define  PCI_VC_REG2_64_PHASE   0x4
#define  PCI_VC_REG2_128_PHASE  0x8
#define PCI_VC_PORT_CTRL        12
#define PCI_VC_PORT_STATUS      14
#define PCI_VC_RES_CAP          16
#define PCI_VC_RES_CTRL         20
#define PCI_VC_RES_STATUS       26
#define PCI_CAP_VC_BASE_SIZEOF          0x10
#define PCI_CAP_VC_PER_VC_SIZEOF        0x0C

/* Power Budgeting */
#define PCI_PWR_DSR             4       /* Data Select Register */
#define PCI_PWR_DATA            8       /* Data Register */
#define  PCI_PWR_DATA_BASE(x)   ((x) & 0xff)        /* Base Power */
#define  PCI_PWR_DATA_SCALE(x)  (((x) >> 8) & 3)    /* Data Scale */
#define  PCI_PWR_DATA_PM_SUB(x) (((x) >> 10) & 7)   /* PM Sub State */
#define  PCI_PWR_DATA_PM_STATE(x) (((x) >> 13) & 3) /* PM State */
#define  PCI_PWR_DATA_TYPE(x)   (((x) >> 15) & 7)   /* Type */
#define  PCI_PWR_DATA_RAIL(x)   (((x) >> 18) & 7)   /* Power Rail */
#define PCI_PWR_CAP             12      /* Capability */
#define  PCI_PWR_CAP_BUDGET(x)  ((x) & 1)       /* Included in system budget */
#define PCI_EXT_CAP_PWR_SIZEOF  16

/* Vendor-Specific (VSEC, PCI_EXT_CAP_ID_VNDR) */
#define PCI_VNDR_HEADER         4       /* Vendor-Specific Header */
#define  PCI_VNDR_HEADER_ID(x)  ((x) & 0xffff)
#define  PCI_VNDR_HEADER_REV(x) (((x) >> 16) & 0xf)
#define  PCI_VNDR_HEADER_LEN(x) (((x) >> 20) & 0xfff)

/*
* Hypertransport sub capability types
*
* Unfortunately there are both 3 bit and 5 bit capability types defined
* in the HT spec, catering for that is a little messy. You probably don't
* want to use these directly, just use pci_find_ht_capability() and it
* will do the right thing for you.
*/
#define HT_3BIT_CAP_MASK        0xE0
#define HT_CAPTYPE_SLAVE        0x00    /* Slave/Primary link configuration */
#define HT_CAPTYPE_HOST         0x20    /* Host/Secondary link configuration */

#define HT_5BIT_CAP_MASK        0xF8
#define HT_CAPTYPE_IRQ          0x80    /* IRQ Configuration */
#define HT_CAPTYPE_REMAPPING_40 0xA0    /* 40 bit address remapping */
#define HT_CAPTYPE_REMAPPING_64 0xA2    /* 64 bit address remapping */
#define HT_CAPTYPE_UNITID_CLUMP 0x90    /* Unit ID clumping */
#define HT_CAPTYPE_EXTCONF      0x98    /* Extended Configuration Space Access */
#define HT_CAPTYPE_MSI_MAPPING  0xA8    /* MSI Mapping Capability */
#define  HT_MSI_FLAGS           0x02            /* Offset to flags */
#define  HT_MSI_FLAGS_ENABLE    0x1             /* Mapping enable */
#define  HT_MSI_FLAGS_FIXED     0x2             /* Fixed mapping only */
#define  HT_MSI_FIXED_ADDR      0x00000000FEE00000ULL   /* Fixed addr */
#define  HT_MSI_ADDR_LO         0x04            /* Offset to low addr bits */
#define  HT_MSI_ADDR_LO_MASK    0xFFF00000      /* Low address bit mask */
#define  HT_MSI_ADDR_HI         0x08            /* Offset to high addr bits */
#define HT_CAPTYPE_DIRECT_ROUTE 0xB0    /* Direct routing configuration */
#define HT_CAPTYPE_VCSET        0xB8    /* Virtual Channel configuration */
#define HT_CAPTYPE_ERROR_RETRY  0xC0    /* Retry on error configuration */
#define HT_CAPTYPE_GEN3         0xD0    /* Generation 3 hypertransport configuration */
#define HT_CAPTYPE_PM           0xE0    /* Hypertransport powermanagement configuration */
#define HT_CAP_SIZEOF_LONG      28      /* slave & primary */
#define HT_CAP_SIZEOF_SHORT     24      /* host & secondary */

/* Alternative Routing-ID Interpretation */
#define PCI_ARI_CAP             0x04    /* ARI Capability Register */
#define  PCI_ARI_CAP_MFVC       0x0001  /* MFVC Function Groups Capability */
#define  PCI_ARI_CAP_ACS        0x0002  /* ACS Function Groups Capability */
#define  PCI_ARI_CAP_NFN(x)     (((x) >> 8) & 0xff) /* Next Function Number */
#define PCI_ARI_CTRL            0x06    /* ARI Control Register */
#define  PCI_ARI_CTRL_MFVC      0x0001  /* MFVC Function Groups Enable */
#define  PCI_ARI_CTRL_ACS       0x0002  /* ACS Function Groups Enable */
#define  PCI_ARI_CTRL_FG(x)     (((x) >> 4) & 7) /* Function Group */
#define PCI_EXT_CAP_ARI_SIZEOF  8

/* Address Translation Service */
#define PCI_ATS_CAP             0x04    /* ATS Capability Register */
#define  PCI_ATS_CAP_QDEP(x)    ((x) & 0x1f)    /* Invalidate Queue Depth */
#define  PCI_ATS_MAX_QDEP       32      /* Max Invalidate Queue Depth */
#define PCI_ATS_CTRL            0x06    /* ATS Control Register */
#define  PCI_ATS_CTRL_ENABLE    0x8000  /* ATS Enable */
#define  PCI_ATS_CTRL_STU(x)    ((x) & 0x1f)    /* Smallest Translation Unit */
#define  PCI_ATS_MIN_STU        12      /* shift of minimum STU block */
#define PCI_EXT_CAP_ATS_SIZEOF  8

/* Page Request Interface */
#define PCI_PRI_CTRL            0x04    /* PRI control register */
#define  PCI_PRI_CTRL_ENABLE    0x01    /* Enable */
#define  PCI_PRI_CTRL_RESET     0x02    /* Reset */
#define PCI_PRI_STATUS          0x06    /* PRI status register */
#define  PCI_PRI_STATUS_RF      0x001   /* Response Failure */
#define  PCI_PRI_STATUS_UPRGI   0x002   /* Unexpected PRG index */
#define  PCI_PRI_STATUS_STOPPED 0x100   /* PRI Stopped */
#define PCI_PRI_MAX_REQ         0x08    /* PRI max reqs supported */
#define PCI_PRI_ALLOC_REQ       0x0c    /* PRI max reqs allowed */
#define PCI_EXT_CAP_PRI_SIZEOF  16

/* PASID capability */
#define PCI_PASID_CAP           0x04    /* PASID feature register */
#define  PCI_PASID_CAP_EXEC     0x02    /* Exec permissions Supported */
#define  PCI_PASID_CAP_PRIV     0x04    /* Priviledge Mode Supported */
#define PCI_PASID_CTRL          0x06    /* PASID control register */
#define  PCI_PASID_CTRL_ENABLE  0x01    /* Enable bit */
#define  PCI_PASID_CTRL_EXEC    0x02    /* Exec permissions Enable */
#define  PCI_PASID_CTRL_PRIV    0x04    /* Priviledge Mode Enable */
#define PCI_EXT_CAP_PASID_SIZEOF        8

/* Single Root I/O Virtualization */
#define PCI_SRIOV_CAP           0x04    /* SR-IOV Capabilities */
#define  PCI_SRIOV_CAP_VFM      0x01    /* VF Migration Capable */
#define  PCI_SRIOV_CAP_INTR(x)  ((x) >> 21) /* Interrupt Message Number */
#define PCI_SRIOV_CTRL          0x08    /* SR-IOV Control */
#define  PCI_SRIOV_CTRL_VFE     0x01    /* VF Enable */
#define  PCI_SRIOV_CTRL_VFM     0x02    /* VF Migration Enable */
#define  PCI_SRIOV_CTRL_INTR    0x04    /* VF Migration Interrupt Enable */
#define  PCI_SRIOV_CTRL_MSE     0x08    /* VF Memory Space Enable */
#define  PCI_SRIOV_CTRL_ARI     0x10    /* ARI Capable Hierarchy */
#define PCI_SRIOV_STATUS        0x0a    /* SR-IOV Status */
#define  PCI_SRIOV_STATUS_VFM   0x01    /* VF Migration Status */
#define PCI_SRIOV_INITIAL_VF    0x0c    /* Initial VFs */
#define PCI_SRIOV_TOTAL_VF      0x0e    /* Total VFs */
#define PCI_SRIOV_NUM_VF        0x10    /* Number of VFs */
#define PCI_SRIOV_FUNC_LINK     0x12    /* Function Dependency Link */
#define PCI_SRIOV_VF_OFFSET     0x14    /* First VF Offset */
#define PCI_SRIOV_VF_STRIDE     0x16    /* Following VF Stride */
#define PCI_SRIOV_VF_DID        0x1a    /* VF Device ID */
#define PCI_SRIOV_SUP_PGSIZE    0x1c    /* Supported Page Sizes */
#define PCI_SRIOV_SYS_PGSIZE    0x20    /* System Page Size */
#define PCI_SRIOV_BAR           0x24    /* VF BAR0 */
#define  PCI_SRIOV_NUM_BARS     6       /* Number of VF BARs */
#define PCI_SRIOV_VFM           0x3c    /* VF Migration State Array Offset*/
#define  PCI_SRIOV_VFM_BIR(x)   ((x) & 7)       /* State BIR */
#define  PCI_SRIOV_VFM_OFFSET(x) ((x) & ~7)     /* State Offset */
#define  PCI_SRIOV_VFM_UA       0x0     /* Inactive.Unavailable */
#define  PCI_SRIOV_VFM_MI       0x1     /* Dormant.MigrateIn */
#define  PCI_SRIOV_VFM_MO       0x2     /* Active.MigrateOut */
#define  PCI_SRIOV_VFM_AV       0x3     /* Active.Available */
#define PCI_EXT_CAP_SRIOV_SIZEOF 64

#define PCI_LTR_MAX_SNOOP_LAT   0x4
#define PCI_LTR_MAX_NOSNOOP_LAT 0x6
#define  PCI_LTR_VALUE_MASK     0x000003ff
#define  PCI_LTR_SCALE_MASK     0x00001c00
#define  PCI_LTR_SCALE_SHIFT    10
#define PCI_EXT_CAP_LTR_SIZEOF  8

/* Access Control Service */
#define PCI_ACS_CAP             0x04    /* ACS Capability Register */
#define  PCI_ACS_SV             0x01    /* Source Validation */
#define  PCI_ACS_TB             0x02    /* Translation Blocking */
#define  PCI_ACS_RR             0x04    /* P2P Request Redirect */
#define  PCI_ACS_CR             0x08    /* P2P Completion Redirect */
#define  PCI_ACS_UF             0x10    /* Upstream Forwarding */
#define  PCI_ACS_EC             0x20    /* P2P Egress Control */
#define  PCI_ACS_DT             0x40    /* Direct Translated P2P */
#define PCI_ACS_EGRESS_BITS     0x05    /* ACS Egress Control Vector Size */
#define PCI_ACS_CTRL            0x06    /* ACS Control Register */
#define PCI_ACS_EGRESS_CTL_V    0x08    /* ACS Egress Control Vector */

#define PCI_VSEC_HDR            4       /* extended cap - vendor specific */
#define  PCI_VSEC_HDR_LEN_SHIFT 20      /* shift for length field */

/* sata capability */
#define PCI_SATA_REGS           4       /* SATA REGs specifier */
#define  PCI_SATA_REGS_MASK     0xF     /* location - BAR#/inline */
#define  PCI_SATA_REGS_INLINE   0xF     /* REGS in config space */
#define PCI_SATA_SIZEOF_SHORT   8
#define PCI_SATA_SIZEOF_LONG    16

/* resizable BARs */
#define PCI_REBAR_CTRL          8       /* control register */
#define  PCI_REBAR_CTRL_NBAR_MASK       (7 << 5)        /* mask for # bars */
#define  PCI_REBAR_CTRL_NBAR_SHIFT      5       /* shift for # bars */

/* dynamic power allocation */
#define PCI_DPA_CAP             4       /* capability register */
#define  PCI_DPA_CAP_SUBSTATE_MASK      0x1F    /* # substates - 1 */
#define PCI_DPA_BASE_SIZEOF     16      /* size with 0 substates */

/* TPH Requester */
#define PCI_TPH_CAP             4       /* capability register */
#define  PCI_TPH_CAP_LOC_MASK   0x600   /* location mask */
#define   PCI_TPH_LOC_NONE      0x000   /* no location */
#define   PCI_TPH_LOC_CAP       0x200   /* in capability */
#define   PCI_TPH_LOC_MSIX      0x400   /* in MSI-X */
#define PCI_TPH_CAP_ST_MASK     0x07FF0000      /* st table mask */
#define PCI_TPH_CAP_ST_SHIFT    16      /* st table shift */
#define PCI_TPH_BASE_SIZEOF     12      /* size with no st table */

// -------------------------------------------------------------------------------------------------

#define PCI_CLASS_NOT_DEFINED           0x0000
#define PCI_CLASS_NOT_DEFINED_VGA       0x0001

#define PCI_BASE_CLASS_STORAGE          0x01
#define PCI_CLASS_STORAGE_SCSI          0x0100
#define PCI_CLASS_STORAGE_IDE           0x0101
#define PCI_CLASS_STORAGE_FLOPPY        0x0102
#define PCI_CLASS_STORAGE_IPI           0x0103
#define PCI_CLASS_STORAGE_RAID          0x0104
#define PCI_CLASS_STORAGE_SATA          0x0106
#define PCI_CLASS_STORAGE_SATA_AHCI     0x010601
#define PCI_CLASS_STORAGE_SAS           0x0107
#define PCI_CLASS_STORAGE_OTHER         0x0180

#define PCI_BASE_CLASS_NETWORK          0x02
#define PCI_CLASS_NETWORK_ETHERNET      0x0200
#define PCI_CLASS_NETWORK_TOKEN_RING    0x0201
#define PCI_CLASS_NETWORK_FDDI          0x0202
#define PCI_CLASS_NETWORK_ATM           0x0203
#define PCI_CLASS_NETWORK_OTHER         0x0280

#define PCI_BASE_CLASS_DISPLAY          0x03
#define PCI_CLASS_DISPLAY_VGA           0x0300
#define PCI_CLASS_DISPLAY_XGA           0x0301
#define PCI_CLASS_DISPLAY_3D            0x0302
#define PCI_CLASS_DISPLAY_OTHER         0x0380

#define PCI_BASE_CLASS_MULTIMEDIA       0x04
#define PCI_CLASS_MULTIMEDIA_VIDEO      0x0400
#define PCI_CLASS_MULTIMEDIA_AUDIO      0x0401
#define PCI_CLASS_MULTIMEDIA_PHONE      0x0402
#define PCI_CLASS_MULTIMEDIA_OTHER      0x0480

#define PCI_BASE_CLASS_MEMORY           0x05
#define PCI_CLASS_MEMORY_RAM            0x0500
#define PCI_CLASS_MEMORY_FLASH          0x0501
#define PCI_CLASS_MEMORY_OTHER          0x0580

#define PCI_BASE_CLASS_BRIDGE           0x06
#define PCI_CLASS_BRIDGE_HOST           0x0600
#define PCI_CLASS_BRIDGE_ISA            0x0601
#define PCI_CLASS_BRIDGE_EISA           0x0602
#define PCI_CLASS_BRIDGE_MC             0x0603
#define PCI_CLASS_BRIDGE_PCI            0x0604
#define PCI_CLASS_BRIDGE_PCMCIA         0x0605
#define PCI_CLASS_BRIDGE_NUBUS          0x0606
#define PCI_CLASS_BRIDGE_CARDBUS        0x0607
#define PCI_CLASS_BRIDGE_RACEWAY        0x0608
#define PCI_CLASS_BRIDGE_OTHER          0x0680

#define PCI_BASE_CLASS_COMMUNICATION    0x07
#define PCI_CLASS_COMMUNICATION_SERIAL  0x0700
#define PCI_CLASS_COMMUNICATION_PARALLEL 0x0701
#define PCI_CLASS_COMMUNICATION_MULTISERIAL 0x0702
#define PCI_CLASS_COMMUNICATION_MODEM   0x0703
#define PCI_CLASS_COMMUNICATION_OTHER   0x0780

#define PCI_BASE_CLASS_SYSTEM           0x08
#define PCI_CLASS_SYSTEM_PIC            0x0800
#define PCI_CLASS_SYSTEM_PIC_IOAPIC     0x080010
#define PCI_CLASS_SYSTEM_PIC_IOXAPIC    0x080020
#define PCI_CLASS_SYSTEM_DMA            0x0801
#define PCI_CLASS_SYSTEM_TIMER          0x0802
#define PCI_CLASS_SYSTEM_RTC            0x0803
#define PCI_CLASS_SYSTEM_PCI_HOTPLUG    0x0804
#define PCI_CLASS_SYSTEM_SDHCI          0x0805
#define PCI_CLASS_SYSTEM_OTHER          0x0880

#define PCI_BASE_CLASS_INPUT            0x09
#define PCI_CLASS_INPUT_KEYBOARD        0x0900
#define PCI_CLASS_INPUT_PEN             0x0901
#define PCI_CLASS_INPUT_MOUSE           0x0902
#define PCI_CLASS_INPUT_SCANNER         0x0903
#define PCI_CLASS_INPUT_GAMEPORT        0x0904
#define PCI_CLASS_INPUT_OTHER           0x0980

#define PCI_BASE_CLASS_DOCKING          0x0a
#define PCI_CLASS_DOCKING_GENERIC       0x0a00
#define PCI_CLASS_DOCKING_OTHER         0x0a80

#define PCI_BASE_CLASS_PROCESSOR        0x0b
#define PCI_CLASS_PROCESSOR_386         0x0b00
#define PCI_CLASS_PROCESSOR_486         0x0b01
#define PCI_CLASS_PROCESSOR_PENTIUM     0x0b02
#define PCI_CLASS_PROCESSOR_ALPHA       0x0b10
#define PCI_CLASS_PROCESSOR_POWERPC     0x0b20
#define PCI_CLASS_PROCESSOR_MIPS        0x0b30
#define PCI_CLASS_PROCESSOR_CO          0x0b40

#define PCI_BASE_CLASS_SERIAL           0x0c
#define PCI_CLASS_SERIAL_FIREWIRE       0x0c00
#define PCI_CLASS_SERIAL_FIREWIRE_OHCI  0x0c0010
#define PCI_CLASS_SERIAL_ACCESS         0x0c01
#define PCI_CLASS_SERIAL_SSA            0x0c02
#define PCI_CLASS_SERIAL_USB            0x0c03
#define PCI_CLASS_SERIAL_USB_UHCI       0x0c0300
#define PCI_CLASS_SERIAL_USB_OHCI       0x0c0310
#define PCI_CLASS_SERIAL_USB_EHCI       0x0c0320
#define PCI_CLASS_SERIAL_USB_XHCI       0x0c0330
#define PCI_CLASS_SERIAL_FIBER          0x0c04
#define PCI_CLASS_SERIAL_SMBUS          0x0c05

#define PCI_BASE_CLASS_WIRELESS                 0x0d
#define PCI_CLASS_WIRELESS_RF_CONTROLLER        0x0d10
#define PCI_CLASS_WIRELESS_WHCI                 0x0d1010

#define PCI_BASE_CLASS_INTELLIGENT      0x0e
#define PCI_CLASS_INTELLIGENT_I2O       0x0e00

#define PCI_BASE_CLASS_SATELLITE        0x0f
#define PCI_CLASS_SATELLITE_TV          0x0f00
#define PCI_CLASS_SATELLITE_AUDIO       0x0f01
#define PCI_CLASS_SATELLITE_VOICE       0x0f03
#define PCI_CLASS_SATELLITE_DATA        0x0f04

#define PCI_BASE_CLASS_CRYPT            0x10
#define PCI_CLASS_CRYPT_NETWORK         0x1000
#define PCI_CLASS_CRYPT_ENTERTAINMENT   0x1001
#define PCI_CLASS_CRYPT_OTHER           0x1080

#define PCI_BASE_CLASS_SIGNAL_PROCESSING 0x11
#define PCI_CLASS_SP_DPIO               0x1100
#define PCI_CLASS_SP_OTHER              0x1180

#define PCI_CLASS_OTHERS                0xff

/* Vendors and devices.  Sort key: vendor first, device next. */

#define PCI_VENDOR_ID_TTTECH            0x0357
#define PCI_DEVICE_ID_TTTECH_MC322      0x000a

#define PCI_VENDOR_ID_DYNALINK          0x0675
#define PCI_DEVICE_ID_DYNALINK_IS64PH   0x1702

#define PCI_VENDOR_ID_BERKOM                    0x0871
#define PCI_DEVICE_ID_BERKOM_A1T                0xffa1
#define PCI_DEVICE_ID_BERKOM_T_CONCEPT          0xffa2
#define PCI_DEVICE_ID_BERKOM_A4T                0xffa4
#define PCI_DEVICE_ID_BERKOM_SCITEL_QUADRO      0xffa8

#define PCI_VENDOR_ID_COMPAQ            0x0e11
#define PCI_DEVICE_ID_COMPAQ_TOKENRING  0x0508
#define PCI_DEVICE_ID_COMPAQ_TACHYON    0xa0fc
#define PCI_DEVICE_ID_COMPAQ_SMART2P    0xae10
#define PCI_DEVICE_ID_COMPAQ_NETEL100   0xae32
#define PCI_DEVICE_ID_COMPAQ_NETEL10    0xae34
#define PCI_DEVICE_ID_COMPAQ_TRIFLEX_IDE 0xae33
#define PCI_DEVICE_ID_COMPAQ_NETFLEX3I  0xae35
#define PCI_DEVICE_ID_COMPAQ_NETEL100D  0xae40
#define PCI_DEVICE_ID_COMPAQ_NETEL100PI 0xae43
#define PCI_DEVICE_ID_COMPAQ_NETEL100I  0xb011
#define PCI_DEVICE_ID_COMPAQ_CISS       0xb060
#define PCI_DEVICE_ID_COMPAQ_CISSB      0xb178
#define PCI_DEVICE_ID_COMPAQ_CISSC      0x46
#define PCI_DEVICE_ID_COMPAQ_THUNDER    0xf130
#define PCI_DEVICE_ID_COMPAQ_NETFLEX3B  0xf150

#define PCI_VENDOR_ID_NCR               0x1000
#define PCI_VENDOR_ID_LSI_LOGIC         0x1000
#define PCI_DEVICE_ID_NCR_53C810        0x0001
#define PCI_DEVICE_ID_NCR_53C820        0x0002
#define PCI_DEVICE_ID_NCR_53C825        0x0003
#define PCI_DEVICE_ID_NCR_53C815        0x0004
#define PCI_DEVICE_ID_LSI_53C810AP      0x0005
#define PCI_DEVICE_ID_NCR_53C860        0x0006
#define PCI_DEVICE_ID_LSI_53C1510       0x000a
#define PCI_DEVICE_ID_NCR_53C896        0x000b
#define PCI_DEVICE_ID_NCR_53C895        0x000c
#define PCI_DEVICE_ID_NCR_53C885        0x000d
#define PCI_DEVICE_ID_NCR_53C875        0x000f
#define PCI_DEVICE_ID_NCR_53C1510       0x0010
#define PCI_DEVICE_ID_LSI_53C895A       0x0012
#define PCI_DEVICE_ID_LSI_53C875A       0x0013
#define PCI_DEVICE_ID_LSI_53C1010_33    0x0020
#define PCI_DEVICE_ID_LSI_53C1010_66    0x0021
#define PCI_DEVICE_ID_LSI_53C1030       0x0030
#define PCI_DEVICE_ID_LSI_1030_53C1035  0x0032
#define PCI_DEVICE_ID_LSI_53C1035       0x0040
#define PCI_DEVICE_ID_NCR_53C875J       0x008f
#define PCI_DEVICE_ID_LSI_FC909         0x0621
#define PCI_DEVICE_ID_LSI_FC929         0x0622
#define PCI_DEVICE_ID_LSI_FC929_LAN     0x0623
#define PCI_DEVICE_ID_LSI_FC919         0x0624
#define PCI_DEVICE_ID_LSI_FC919_LAN     0x0625
#define PCI_DEVICE_ID_LSI_FC929X        0x0626
#define PCI_DEVICE_ID_LSI_FC939X        0x0642
#define PCI_DEVICE_ID_LSI_FC949X        0x0640
#define PCI_DEVICE_ID_LSI_FC949ES       0x0646
#define PCI_DEVICE_ID_LSI_FC919X        0x0628
#define PCI_DEVICE_ID_NCR_YELLOWFIN     0x0701
#define PCI_DEVICE_ID_LSI_61C102        0x0901
#define PCI_DEVICE_ID_LSI_63C815        0x1000
#define PCI_DEVICE_ID_LSI_SAS1064       0x0050
#define PCI_DEVICE_ID_LSI_SAS1064R      0x0411
#define PCI_DEVICE_ID_LSI_SAS1066       0x005E
#define PCI_DEVICE_ID_LSI_SAS1068       0x0054
#define PCI_DEVICE_ID_LSI_SAS1064A      0x005C
#define PCI_DEVICE_ID_LSI_SAS1064E      0x0056
#define PCI_DEVICE_ID_LSI_SAS1066E      0x005A
#define PCI_DEVICE_ID_LSI_SAS1068E      0x0058
#define PCI_DEVICE_ID_LSI_SAS1078       0x0060

#define PCI_VENDOR_ID_ATI               0x1002
/* Mach64 */
#define PCI_DEVICE_ID_ATI_68800         0x4158
#define PCI_DEVICE_ID_ATI_215CT222      0x4354
#define PCI_DEVICE_ID_ATI_210888CX      0x4358
#define PCI_DEVICE_ID_ATI_215ET222      0x4554
/* Mach64 / Rage */
#define PCI_DEVICE_ID_ATI_215GB         0x4742
#define PCI_DEVICE_ID_ATI_215GD         0x4744
#define PCI_DEVICE_ID_ATI_215GI         0x4749
#define PCI_DEVICE_ID_ATI_215GP         0x4750
#define PCI_DEVICE_ID_ATI_215GQ         0x4751
#define PCI_DEVICE_ID_ATI_215XL         0x4752
#define PCI_DEVICE_ID_ATI_215GT         0x4754
#define PCI_DEVICE_ID_ATI_215GTB        0x4755
#define PCI_DEVICE_ID_ATI_215_IV        0x4756
#define PCI_DEVICE_ID_ATI_215_IW        0x4757
#define PCI_DEVICE_ID_ATI_215_IZ        0x475A
#define PCI_DEVICE_ID_ATI_210888GX      0x4758
#define PCI_DEVICE_ID_ATI_215_LB        0x4c42
#define PCI_DEVICE_ID_ATI_215_LD        0x4c44
#define PCI_DEVICE_ID_ATI_215_LG        0x4c47
#define PCI_DEVICE_ID_ATI_215_LI        0x4c49
#define PCI_DEVICE_ID_ATI_215_LM        0x4c4D
#define PCI_DEVICE_ID_ATI_215_LN        0x4c4E
#define PCI_DEVICE_ID_ATI_215_LR        0x4c52
#define PCI_DEVICE_ID_ATI_215_LS        0x4c53
#define PCI_DEVICE_ID_ATI_264_LT        0x4c54
/* Mach64 VT */
#define PCI_DEVICE_ID_ATI_264VT         0x5654
#define PCI_DEVICE_ID_ATI_264VU         0x5655
#define PCI_DEVICE_ID_ATI_264VV         0x5656
/* Rage128 GL */
#define PCI_DEVICE_ID_ATI_RAGE128_RE    0x5245
#define PCI_DEVICE_ID_ATI_RAGE128_RF    0x5246
#define PCI_DEVICE_ID_ATI_RAGE128_RG    0x5247
/* Rage128 VR */
#define PCI_DEVICE_ID_ATI_RAGE128_RK    0x524b
#define PCI_DEVICE_ID_ATI_RAGE128_RL    0x524c
#define PCI_DEVICE_ID_ATI_RAGE128_SE    0x5345
#define PCI_DEVICE_ID_ATI_RAGE128_SF    0x5346
#define PCI_DEVICE_ID_ATI_RAGE128_SG    0x5347
#define PCI_DEVICE_ID_ATI_RAGE128_SH    0x5348
#define PCI_DEVICE_ID_ATI_RAGE128_SK    0x534b
#define PCI_DEVICE_ID_ATI_RAGE128_SL    0x534c
#define PCI_DEVICE_ID_ATI_RAGE128_SM    0x534d
#define PCI_DEVICE_ID_ATI_RAGE128_SN    0x534e
/* Rage128 Ultra */
#define PCI_DEVICE_ID_ATI_RAGE128_TF    0x5446
#define PCI_DEVICE_ID_ATI_RAGE128_TL    0x544c
#define PCI_DEVICE_ID_ATI_RAGE128_TR    0x5452
#define PCI_DEVICE_ID_ATI_RAGE128_TS    0x5453
#define PCI_DEVICE_ID_ATI_RAGE128_TT    0x5454
#define PCI_DEVICE_ID_ATI_RAGE128_TU    0x5455
/* Rage128 M3 */
#define PCI_DEVICE_ID_ATI_RAGE128_LE    0x4c45
#define PCI_DEVICE_ID_ATI_RAGE128_LF    0x4c46
/* Rage128 M4 */
#define PCI_DEVICE_ID_ATI_RAGE128_MF    0x4d46
#define PCI_DEVICE_ID_ATI_RAGE128_ML    0x4d4c
/* Rage128 Pro GL */
#define PCI_DEVICE_ID_ATI_RAGE128_PA    0x5041
#define PCI_DEVICE_ID_ATI_RAGE128_PB    0x5042
#define PCI_DEVICE_ID_ATI_RAGE128_PC    0x5043
#define PCI_DEVICE_ID_ATI_RAGE128_PD    0x5044
#define PCI_DEVICE_ID_ATI_RAGE128_PE    0x5045
#define PCI_DEVICE_ID_ATI_RAGE128_PF    0x5046
/* Rage128 Pro VR */
#define PCI_DEVICE_ID_ATI_RAGE128_PG    0x5047
#define PCI_DEVICE_ID_ATI_RAGE128_PH    0x5048
#define PCI_DEVICE_ID_ATI_RAGE128_PI    0x5049
#define PCI_DEVICE_ID_ATI_RAGE128_PJ    0x504A
#define PCI_DEVICE_ID_ATI_RAGE128_PK    0x504B
#define PCI_DEVICE_ID_ATI_RAGE128_PL    0x504C
#define PCI_DEVICE_ID_ATI_RAGE128_PM    0x504D
#define PCI_DEVICE_ID_ATI_RAGE128_PN    0x504E
#define PCI_DEVICE_ID_ATI_RAGE128_PO    0x504F
#define PCI_DEVICE_ID_ATI_RAGE128_PP    0x5050
#define PCI_DEVICE_ID_ATI_RAGE128_PQ    0x5051
#define PCI_DEVICE_ID_ATI_RAGE128_PR    0x5052
#define PCI_DEVICE_ID_ATI_RAGE128_PS    0x5053
#define PCI_DEVICE_ID_ATI_RAGE128_PT    0x5054
#define PCI_DEVICE_ID_ATI_RAGE128_PU    0x5055
#define PCI_DEVICE_ID_ATI_RAGE128_PV    0x5056
#define PCI_DEVICE_ID_ATI_RAGE128_PW    0x5057
#define PCI_DEVICE_ID_ATI_RAGE128_PX    0x5058
/* Rage128 M4 */
/* Radeon R100 */
#define PCI_DEVICE_ID_ATI_RADEON_QD     0x5144
#define PCI_DEVICE_ID_ATI_RADEON_QE     0x5145
#define PCI_DEVICE_ID_ATI_RADEON_QF     0x5146
#define PCI_DEVICE_ID_ATI_RADEON_QG     0x5147
/* Radeon RV100 (VE) */
#define PCI_DEVICE_ID_ATI_RADEON_QY     0x5159
#define PCI_DEVICE_ID_ATI_RADEON_QZ     0x515a
/* Radeon R200 (8500) */
#define PCI_DEVICE_ID_ATI_RADEON_QL     0x514c
#define PCI_DEVICE_ID_ATI_RADEON_QN     0x514e
#define PCI_DEVICE_ID_ATI_RADEON_QO     0x514f
#define PCI_DEVICE_ID_ATI_RADEON_Ql     0x516c
#define PCI_DEVICE_ID_ATI_RADEON_BB     0x4242
/* Radeon R200 (9100) */
#define PCI_DEVICE_ID_ATI_RADEON_QM     0x514d
/* Radeon RV200 (7500) */
#define PCI_DEVICE_ID_ATI_RADEON_QW     0x5157
#define PCI_DEVICE_ID_ATI_RADEON_QX     0x5158
/* Radeon NV-100 */
/* Radeon RV250 (9000) */
#define PCI_DEVICE_ID_ATI_RADEON_Id     0x4964
#define PCI_DEVICE_ID_ATI_RADEON_Ie     0x4965
#define PCI_DEVICE_ID_ATI_RADEON_If     0x4966
#define PCI_DEVICE_ID_ATI_RADEON_Ig     0x4967
/* Radeon RV280 (9200) */
#define PCI_DEVICE_ID_ATI_RADEON_Ya     0x5961
#define PCI_DEVICE_ID_ATI_RADEON_Yd     0x5964
/* Radeon R300 (9500) */
/* Radeon R300 (9700) */
#define PCI_DEVICE_ID_ATI_RADEON_ND     0x4e44
#define PCI_DEVICE_ID_ATI_RADEON_NE     0x4e45
#define PCI_DEVICE_ID_ATI_RADEON_NF     0x4e46
#define PCI_DEVICE_ID_ATI_RADEON_NG     0x4e47
/* Radeon R350 (9800) */
/* Radeon RV350 (9600) */
/* Radeon M6 */
#define PCI_DEVICE_ID_ATI_RADEON_LY     0x4c59
#define PCI_DEVICE_ID_ATI_RADEON_LZ     0x4c5a
/* Radeon M7 */
#define PCI_DEVICE_ID_ATI_RADEON_LW     0x4c57
#define PCI_DEVICE_ID_ATI_RADEON_LX     0x4c58
/* Radeon M9 */
#define PCI_DEVICE_ID_ATI_RADEON_Ld     0x4c64
#define PCI_DEVICE_ID_ATI_RADEON_Le     0x4c65
#define PCI_DEVICE_ID_ATI_RADEON_Lf     0x4c66
#define PCI_DEVICE_ID_ATI_RADEON_Lg     0x4c67
/* Radeon */
/* RadeonIGP */
#define PCI_DEVICE_ID_ATI_RS100         0xcab0
#define PCI_DEVICE_ID_ATI_RS200         0xcab2
#define PCI_DEVICE_ID_ATI_RS200_B       0xcbb2
#define PCI_DEVICE_ID_ATI_RS250         0xcab3
#define PCI_DEVICE_ID_ATI_RS300_100     0x5830
#define PCI_DEVICE_ID_ATI_RS300_133     0x5831
#define PCI_DEVICE_ID_ATI_RS300_166     0x5832
#define PCI_DEVICE_ID_ATI_RS300_200     0x5833
#define PCI_DEVICE_ID_ATI_RS350_100     0x7830
#define PCI_DEVICE_ID_ATI_RS350_133     0x7831
#define PCI_DEVICE_ID_ATI_RS350_166     0x7832
#define PCI_DEVICE_ID_ATI_RS350_200     0x7833
#define PCI_DEVICE_ID_ATI_RS400_100     0x5a30
#define PCI_DEVICE_ID_ATI_RS400_133     0x5a31
#define PCI_DEVICE_ID_ATI_RS400_166     0x5a32
#define PCI_DEVICE_ID_ATI_RS400_200     0x5a33
#define PCI_DEVICE_ID_ATI_RS480         0x5950
/* ATI IXP Chipset */
#define PCI_DEVICE_ID_ATI_IXP200_IDE    0x4349
#define PCI_DEVICE_ID_ATI_IXP200_SMBUS  0x4353
#define PCI_DEVICE_ID_ATI_IXP300_SMBUS  0x4363
#define PCI_DEVICE_ID_ATI_IXP300_IDE    0x4369
#define PCI_DEVICE_ID_ATI_IXP300_SATA   0x436e
#define PCI_DEVICE_ID_ATI_IXP400_SMBUS  0x4372
#define PCI_DEVICE_ID_ATI_IXP400_IDE    0x4376
#define PCI_DEVICE_ID_ATI_IXP400_SATA   0x4379
#define PCI_DEVICE_ID_ATI_IXP400_SATA2  0x437a
#define PCI_DEVICE_ID_ATI_IXP600_SATA   0x4380
#define PCI_DEVICE_ID_ATI_SBX00_SMBUS   0x4385
#define PCI_DEVICE_ID_ATI_IXP600_IDE    0x438c
#define PCI_DEVICE_ID_ATI_IXP700_SATA   0x4390
#define PCI_DEVICE_ID_ATI_IXP700_IDE    0x439c

#define PCI_VENDOR_ID_VLSI              0x1004
#define PCI_DEVICE_ID_VLSI_82C592       0x0005
#define PCI_DEVICE_ID_VLSI_82C593       0x0006
#define PCI_DEVICE_ID_VLSI_82C594       0x0007
#define PCI_DEVICE_ID_VLSI_82C597       0x0009
#define PCI_DEVICE_ID_VLSI_82C541       0x000c
#define PCI_DEVICE_ID_VLSI_82C543       0x000d
#define PCI_DEVICE_ID_VLSI_82C532       0x0101
#define PCI_DEVICE_ID_VLSI_82C534       0x0102
#define PCI_DEVICE_ID_VLSI_82C535       0x0104
#define PCI_DEVICE_ID_VLSI_82C147       0x0105
#define PCI_DEVICE_ID_VLSI_VAS96011     0x0702

/* AMD RD890 Chipset */
#define PCI_DEVICE_ID_RD890_IOMMU       0x5a23

#define PCI_VENDOR_ID_ADL               0x1005
#define PCI_DEVICE_ID_ADL_2301          0x2301

#define PCI_VENDOR_ID_NS                0x100b
#define PCI_DEVICE_ID_NS_87415          0x0002
#define PCI_DEVICE_ID_NS_87560_LIO      0x000e
#define PCI_DEVICE_ID_NS_87560_USB      0x0012
#define PCI_DEVICE_ID_NS_83815          0x0020
#define PCI_DEVICE_ID_NS_83820          0x0022
#define PCI_DEVICE_ID_NS_CS5535_ISA     0x002b
#define PCI_DEVICE_ID_NS_CS5535_IDE     0x002d
#define PCI_DEVICE_ID_NS_CS5535_AUDIO   0x002e
#define PCI_DEVICE_ID_NS_CS5535_USB     0x002f
#define PCI_DEVICE_ID_NS_GX_VIDEO       0x0030
#define PCI_DEVICE_ID_NS_SATURN         0x0035
#define PCI_DEVICE_ID_NS_SCx200_BRIDGE  0x0500
#define PCI_DEVICE_ID_NS_SCx200_SMI     0x0501
#define PCI_DEVICE_ID_NS_SCx200_IDE     0x0502
#define PCI_DEVICE_ID_NS_SCx200_AUDIO   0x0503
#define PCI_DEVICE_ID_NS_SCx200_VIDEO   0x0504
#define PCI_DEVICE_ID_NS_SCx200_XBUS    0x0505
#define PCI_DEVICE_ID_NS_SC1100_BRIDGE  0x0510
#define PCI_DEVICE_ID_NS_SC1100_SMI     0x0511
#define PCI_DEVICE_ID_NS_SC1100_XBUS    0x0515
#define PCI_DEVICE_ID_NS_87410          0xd001

#define PCI_DEVICE_ID_NS_GX_HOST_BRIDGE  0x0028

#define PCI_VENDOR_ID_TSENG             0x100c
#define PCI_DEVICE_ID_TSENG_W32P_2      0x3202
#define PCI_DEVICE_ID_TSENG_W32P_b      0x3205
#define PCI_DEVICE_ID_TSENG_W32P_c      0x3206
#define PCI_DEVICE_ID_TSENG_W32P_d      0x3207
#define PCI_DEVICE_ID_TSENG_ET6000      0x3208

#define PCI_VENDOR_ID_WEITEK            0x100e
#define PCI_DEVICE_ID_WEITEK_P9000      0x9001
#define PCI_DEVICE_ID_WEITEK_P9100      0x9100

#define PCI_VENDOR_ID_DEC               0x1011
#define PCI_DEVICE_ID_DEC_BRD           0x0001
#define PCI_DEVICE_ID_DEC_TULIP         0x0002
#define PCI_DEVICE_ID_DEC_TGA           0x0004
#define PCI_DEVICE_ID_DEC_TULIP_FAST    0x0009
#define PCI_DEVICE_ID_DEC_TGA2          0x000D
#define PCI_DEVICE_ID_DEC_FDDI          0x000F
#define PCI_DEVICE_ID_DEC_TULIP_PLUS    0x0014
#define PCI_DEVICE_ID_DEC_21142         0x0019
#define PCI_DEVICE_ID_DEC_21052         0x0021
#define PCI_DEVICE_ID_DEC_21150         0x0022
#define PCI_DEVICE_ID_DEC_21152         0x0024
#define PCI_DEVICE_ID_DEC_21153         0x0025
#define PCI_DEVICE_ID_DEC_21154         0x0026
#define PCI_DEVICE_ID_DEC_21285         0x1065
#define PCI_DEVICE_ID_COMPAQ_42XX       0x0046

#define PCI_VENDOR_ID_CIRRUS            0x1013
#define PCI_DEVICE_ID_CIRRUS_7548       0x0038
#define PCI_DEVICE_ID_CIRRUS_5430       0x00a0
#define PCI_DEVICE_ID_CIRRUS_5434_4     0x00a4
#define PCI_DEVICE_ID_CIRRUS_5434_8     0x00a8
#define PCI_DEVICE_ID_CIRRUS_5436       0x00ac
#define PCI_DEVICE_ID_CIRRUS_5446       0x00b8
#define PCI_DEVICE_ID_CIRRUS_5480       0x00bc
#define PCI_DEVICE_ID_CIRRUS_5462       0x00d0
#define PCI_DEVICE_ID_CIRRUS_5464       0x00d4
#define PCI_DEVICE_ID_CIRRUS_5465       0x00d6
#define PCI_DEVICE_ID_CIRRUS_6729       0x1100
#define PCI_DEVICE_ID_CIRRUS_6832       0x1110
#define PCI_DEVICE_ID_CIRRUS_7543       0x1202
#define PCI_DEVICE_ID_CIRRUS_4610       0x6001
#define PCI_DEVICE_ID_CIRRUS_4612       0x6003
#define PCI_DEVICE_ID_CIRRUS_4615       0x6004

#define PCI_VENDOR_ID_IBM               0x1014
#define PCI_DEVICE_ID_IBM_TR            0x0018
#define PCI_DEVICE_ID_IBM_TR_WAKE       0x003e
#define PCI_DEVICE_ID_IBM_CPC710_PCI64  0x00fc
#define PCI_DEVICE_ID_IBM_SNIPE         0x0180
#define PCI_DEVICE_ID_IBM_CITRINE               0x028C
#define PCI_DEVICE_ID_IBM_GEMSTONE              0xB166
#define PCI_DEVICE_ID_IBM_OBSIDIAN              0x02BD
#define PCI_DEVICE_ID_IBM_ICOM_DEV_ID_1 0x0031
#define PCI_DEVICE_ID_IBM_ICOM_DEV_ID_2 0x0219
#define PCI_DEVICE_ID_IBM_ICOM_V2_TWO_PORTS_RVX         0x021A
#define PCI_DEVICE_ID_IBM_ICOM_V2_ONE_PORT_RVX_ONE_PORT_MDM     0x0251
#define PCI_DEVICE_ID_IBM_ICOM_V2_ONE_PORT_RVX_ONE_PORT_MDM_PCIE 0x0361
#define PCI_DEVICE_ID_IBM_ICOM_FOUR_PORT_MODEL  0x252

#define PCI_SUBVENDOR_ID_IBM            0x1014
#define PCI_SUBDEVICE_ID_IBM_SATURN_SERIAL_ONE_PORT     0x03d4

#define PCI_VENDOR_ID_UNISYS            0x1018
#define PCI_DEVICE_ID_UNISYS_DMA_DIRECTOR 0x001C

#define PCI_VENDOR_ID_COMPEX2           0x101a /* pci.ids says "AT&T GIS (NCR)" */
#define PCI_DEVICE_ID_COMPEX2_100VG     0x0005

#define PCI_VENDOR_ID_WD                0x101c
#define PCI_DEVICE_ID_WD_90C            0xc24a

#define PCI_VENDOR_ID_AMI               0x101e
#define PCI_DEVICE_ID_AMI_MEGARAID3     0x1960
#define PCI_DEVICE_ID_AMI_MEGARAID      0x9010
#define PCI_DEVICE_ID_AMI_MEGARAID2     0x9060

#define PCI_VENDOR_ID_AMD               0x1022
#define PCI_DEVICE_ID_AMD_K8_NB         0x1100
#define PCI_DEVICE_ID_AMD_K8_NB_ADDRMAP 0x1101
#define PCI_DEVICE_ID_AMD_K8_NB_MEMCTL  0x1102
#define PCI_DEVICE_ID_AMD_K8_NB_MISC    0x1103
#define PCI_DEVICE_ID_AMD_10H_NB_HT     0x1200
#define PCI_DEVICE_ID_AMD_10H_NB_MAP    0x1201
#define PCI_DEVICE_ID_AMD_10H_NB_DRAM   0x1202
#define PCI_DEVICE_ID_AMD_10H_NB_MISC   0x1203
#define PCI_DEVICE_ID_AMD_10H_NB_LINK   0x1204
#define PCI_DEVICE_ID_AMD_11H_NB_HT     0x1300
#define PCI_DEVICE_ID_AMD_11H_NB_MAP    0x1301
#define PCI_DEVICE_ID_AMD_11H_NB_DRAM   0x1302
#define PCI_DEVICE_ID_AMD_11H_NB_MISC   0x1303
#define PCI_DEVICE_ID_AMD_11H_NB_LINK   0x1304
#define PCI_DEVICE_ID_AMD_15H_M10H_F3   0x1403
#define PCI_DEVICE_ID_AMD_15H_NB_F0     0x1600
#define PCI_DEVICE_ID_AMD_15H_NB_F1     0x1601
#define PCI_DEVICE_ID_AMD_15H_NB_F2     0x1602
#define PCI_DEVICE_ID_AMD_15H_NB_F3     0x1603
#define PCI_DEVICE_ID_AMD_15H_NB_F4     0x1604
#define PCI_DEVICE_ID_AMD_15H_NB_F5     0x1605
#define PCI_DEVICE_ID_AMD_16H_NB_F3     0x1533
#define PCI_DEVICE_ID_AMD_16H_NB_F4     0x1534
#define PCI_DEVICE_ID_AMD_CNB17H_F3     0x1703
#define PCI_DEVICE_ID_AMD_LANCE         0x2000
#define PCI_DEVICE_ID_AMD_LANCE_HOME    0x2001
#define PCI_DEVICE_ID_AMD_SCSI          0x2020
#define PCI_DEVICE_ID_AMD_SERENADE      0x36c0
#define PCI_DEVICE_ID_AMD_FE_GATE_7006  0x7006
#define PCI_DEVICE_ID_AMD_FE_GATE_7007  0x7007
#define PCI_DEVICE_ID_AMD_FE_GATE_700C  0x700C
#define PCI_DEVICE_ID_AMD_FE_GATE_700E  0x700E
#define PCI_DEVICE_ID_AMD_COBRA_7401    0x7401
#define PCI_DEVICE_ID_AMD_VIPER_7409    0x7409
#define PCI_DEVICE_ID_AMD_VIPER_740B    0x740B
#define PCI_DEVICE_ID_AMD_VIPER_7410    0x7410
#define PCI_DEVICE_ID_AMD_VIPER_7411    0x7411
#define PCI_DEVICE_ID_AMD_VIPER_7413    0x7413
#define PCI_DEVICE_ID_AMD_VIPER_7440    0x7440
#define PCI_DEVICE_ID_AMD_OPUS_7441     0x7441
#define PCI_DEVICE_ID_AMD_OPUS_7443     0x7443
#define PCI_DEVICE_ID_AMD_VIPER_7443    0x7443
#define PCI_DEVICE_ID_AMD_OPUS_7445     0x7445
#define PCI_DEVICE_ID_AMD_8111_PCI      0x7460
#define PCI_DEVICE_ID_AMD_8111_LPC      0x7468
#define PCI_DEVICE_ID_AMD_8111_IDE      0x7469
#define PCI_DEVICE_ID_AMD_8111_SMBUS2   0x746a
#define PCI_DEVICE_ID_AMD_8111_SMBUS    0x746b
#define PCI_DEVICE_ID_AMD_8111_AUDIO    0x746d
#define PCI_DEVICE_ID_AMD_8151_0        0x7454
#define PCI_DEVICE_ID_AMD_8131_BRIDGE   0x7450
#define PCI_DEVICE_ID_AMD_8131_APIC     0x7451
#define PCI_DEVICE_ID_AMD_8132_BRIDGE   0x7458
#define PCI_DEVICE_ID_AMD_CS5535_IDE    0x208F
#define PCI_DEVICE_ID_AMD_CS5536_ISA    0x2090
#define PCI_DEVICE_ID_AMD_CS5536_FLASH  0x2091
#define PCI_DEVICE_ID_AMD_CS5536_AUDIO  0x2093
#define PCI_DEVICE_ID_AMD_CS5536_OHC    0x2094
#define PCI_DEVICE_ID_AMD_CS5536_EHC    0x2095
#define PCI_DEVICE_ID_AMD_CS5536_UDC    0x2096
#define PCI_DEVICE_ID_AMD_CS5536_UOC    0x2097
#define PCI_DEVICE_ID_AMD_CS5536_IDE    0x209A
#define PCI_DEVICE_ID_AMD_LX_VIDEO  0x2081
#define PCI_DEVICE_ID_AMD_LX_AES    0x2082
#define PCI_DEVICE_ID_AMD_HUDSON2_SATA_IDE      0x7800
#define PCI_DEVICE_ID_AMD_HUDSON2_SMBUS         0x780b
#define PCI_DEVICE_ID_AMD_HUDSON2_IDE           0x780c

#define PCI_VENDOR_ID_TRIDENT           0x1023
#define PCI_DEVICE_ID_TRIDENT_4DWAVE_DX 0x2000
#define PCI_DEVICE_ID_TRIDENT_4DWAVE_NX 0x2001
#define PCI_DEVICE_ID_TRIDENT_9320      0x9320
#define PCI_DEVICE_ID_TRIDENT_9388      0x9388
#define PCI_DEVICE_ID_TRIDENT_9397      0x9397
#define PCI_DEVICE_ID_TRIDENT_939A      0x939A
#define PCI_DEVICE_ID_TRIDENT_9520      0x9520
#define PCI_DEVICE_ID_TRIDENT_9525      0x9525
#define PCI_DEVICE_ID_TRIDENT_9420      0x9420
#define PCI_DEVICE_ID_TRIDENT_9440      0x9440
#define PCI_DEVICE_ID_TRIDENT_9660      0x9660
#define PCI_DEVICE_ID_TRIDENT_9750      0x9750
#define PCI_DEVICE_ID_TRIDENT_9850      0x9850
#define PCI_DEVICE_ID_TRIDENT_9880      0x9880
#define PCI_DEVICE_ID_TRIDENT_8400      0x8400
#define PCI_DEVICE_ID_TRIDENT_8420      0x8420
#define PCI_DEVICE_ID_TRIDENT_8500      0x8500

#define PCI_VENDOR_ID_AI                0x1025
#define PCI_DEVICE_ID_AI_M1435          0x1435

#define PCI_VENDOR_ID_DELL              0x1028
#define PCI_DEVICE_ID_DELL_RACIII       0x0008
#define PCI_DEVICE_ID_DELL_RAC4         0x0012
#define PCI_DEVICE_ID_DELL_PERC5        0x0015

#define PCI_VENDOR_ID_MATROX            0x102B
#define PCI_DEVICE_ID_MATROX_MGA_2      0x0518
#define PCI_DEVICE_ID_MATROX_MIL        0x0519
#define PCI_DEVICE_ID_MATROX_MYS        0x051A
#define PCI_DEVICE_ID_MATROX_MIL_2      0x051b
#define PCI_DEVICE_ID_MATROX_MYS_AGP    0x051e
#define PCI_DEVICE_ID_MATROX_MIL_2_AGP  0x051f
#define PCI_DEVICE_ID_MATROX_MGA_IMP    0x0d10
#define PCI_DEVICE_ID_MATROX_G100_MM    0x1000
#define PCI_DEVICE_ID_MATROX_G100_AGP   0x1001
#define PCI_DEVICE_ID_MATROX_G200_PCI   0x0520
#define PCI_DEVICE_ID_MATROX_G200_AGP   0x0521
#define PCI_DEVICE_ID_MATROX_G400       0x0525
#define PCI_DEVICE_ID_MATROX_G200EV_PCI 0x0530
#define PCI_DEVICE_ID_MATROX_G550       0x2527
#define PCI_DEVICE_ID_MATROX_VIA        0x4536

#define PCI_VENDOR_ID_MOBILITY_ELECTRONICS      0x14f2

#define PCI_VENDOR_ID_CT                0x102c
#define PCI_DEVICE_ID_CT_69000          0x00c0
#define PCI_DEVICE_ID_CT_65545          0x00d8
#define PCI_DEVICE_ID_CT_65548          0x00dc
#define PCI_DEVICE_ID_CT_65550          0x00e0
#define PCI_DEVICE_ID_CT_65554          0x00e4
#define PCI_DEVICE_ID_CT_65555          0x00e5

#define PCI_VENDOR_ID_MIRO              0x1031
#define PCI_DEVICE_ID_MIRO_36050        0x5601
#define PCI_DEVICE_ID_MIRO_DC10PLUS     0x7efe
#define PCI_DEVICE_ID_MIRO_DC30PLUS     0xd801

#define PCI_VENDOR_ID_NEC               0x1033
#define PCI_DEVICE_ID_NEC_CBUS_1        0x0001 /* PCI-Cbus Bridge */
#define PCI_DEVICE_ID_NEC_LOCAL         0x0002 /* Local Bridge */
#define PCI_DEVICE_ID_NEC_ATM           0x0003 /* ATM LAN Controller */
#define PCI_DEVICE_ID_NEC_R4000         0x0004 /* R4000 Bridge */
#define PCI_DEVICE_ID_NEC_486           0x0005 /* 486 Like Peripheral Bus Bridge */
#define PCI_DEVICE_ID_NEC_ACCEL_1       0x0006 /* Graphic Accelerator */
#define PCI_DEVICE_ID_NEC_UXBUS         0x0007 /* UX-Bus Bridge */
#define PCI_DEVICE_ID_NEC_ACCEL_2       0x0008 /* Graphic Accelerator */
#define PCI_DEVICE_ID_NEC_GRAPH         0x0009 /* PCI-CoreGraph Bridge */
#define PCI_DEVICE_ID_NEC_VL            0x0016 /* PCI-VL Bridge */
#define PCI_DEVICE_ID_NEC_STARALPHA2    0x002c /* STAR ALPHA2 */
#define PCI_DEVICE_ID_NEC_CBUS_2        0x002d /* PCI-Cbus Bridge */
#define PCI_DEVICE_ID_NEC_USB           0x0035 /* PCI-USB Host */
#define PCI_DEVICE_ID_NEC_CBUS_3        0x003b
#define PCI_DEVICE_ID_NEC_NAPCCARD      0x003e
#define PCI_DEVICE_ID_NEC_PCX2          0x0046 /* PowerVR */
#define PCI_DEVICE_ID_NEC_VRC5476       0x009b
#define PCI_DEVICE_ID_NEC_VRC4173       0x00a5
#define PCI_DEVICE_ID_NEC_VRC5477_AC97  0x00a6
#define PCI_DEVICE_ID_NEC_PC9821CS01    0x800c /* PC-9821-CS01 */
#define PCI_DEVICE_ID_NEC_PC9821NRB06   0x800d /* PC-9821NR-B06 */

#define PCI_VENDOR_ID_FD                0x1036
#define PCI_DEVICE_ID_FD_36C70          0x0000

#define PCI_VENDOR_ID_SI                0x1039
#define PCI_DEVICE_ID_SI_5591_AGP       0x0001
#define PCI_DEVICE_ID_SI_6202           0x0002
#define PCI_DEVICE_ID_SI_503            0x0008
#define PCI_DEVICE_ID_SI_ACPI           0x0009
#define PCI_DEVICE_ID_SI_SMBUS          0x0016
#define PCI_DEVICE_ID_SI_LPC            0x0018
#define PCI_DEVICE_ID_SI_5597_VGA       0x0200
#define PCI_DEVICE_ID_SI_6205           0x0205
#define PCI_DEVICE_ID_SI_501            0x0406
#define PCI_DEVICE_ID_SI_496            0x0496
#define PCI_DEVICE_ID_SI_300            0x0300
#define PCI_DEVICE_ID_SI_315H           0x0310
#define PCI_DEVICE_ID_SI_315            0x0315
#define PCI_DEVICE_ID_SI_315PRO         0x0325
#define PCI_DEVICE_ID_SI_530            0x0530
#define PCI_DEVICE_ID_SI_540            0x0540
#define PCI_DEVICE_ID_SI_550            0x0550
#define PCI_DEVICE_ID_SI_540_VGA        0x5300
#define PCI_DEVICE_ID_SI_550_VGA        0x5315
#define PCI_DEVICE_ID_SI_620            0x0620
#define PCI_DEVICE_ID_SI_630            0x0630
#define PCI_DEVICE_ID_SI_633            0x0633
#define PCI_DEVICE_ID_SI_635            0x0635
#define PCI_DEVICE_ID_SI_640            0x0640
#define PCI_DEVICE_ID_SI_645            0x0645
#define PCI_DEVICE_ID_SI_646            0x0646
#define PCI_DEVICE_ID_SI_648            0x0648
#define PCI_DEVICE_ID_SI_650            0x0650
#define PCI_DEVICE_ID_SI_651            0x0651
#define PCI_DEVICE_ID_SI_655            0x0655
#define PCI_DEVICE_ID_SI_661            0x0661
#define PCI_DEVICE_ID_SI_730            0x0730
#define PCI_DEVICE_ID_SI_733            0x0733
#define PCI_DEVICE_ID_SI_630_VGA        0x6300
#define PCI_DEVICE_ID_SI_735            0x0735
#define PCI_DEVICE_ID_SI_740            0x0740
#define PCI_DEVICE_ID_SI_741            0x0741
#define PCI_DEVICE_ID_SI_745            0x0745
#define PCI_DEVICE_ID_SI_746            0x0746
#define PCI_DEVICE_ID_SI_755            0x0755
#define PCI_DEVICE_ID_SI_760            0x0760
#define PCI_DEVICE_ID_SI_900            0x0900
#define PCI_DEVICE_ID_SI_961            0x0961
#define PCI_DEVICE_ID_SI_962            0x0962
#define PCI_DEVICE_ID_SI_963            0x0963
#define PCI_DEVICE_ID_SI_965            0x0965
#define PCI_DEVICE_ID_SI_966            0x0966
#define PCI_DEVICE_ID_SI_968            0x0968
#define PCI_DEVICE_ID_SI_1180           0x1180
#define PCI_DEVICE_ID_SI_5511           0x5511
#define PCI_DEVICE_ID_SI_5513           0x5513
#define PCI_DEVICE_ID_SI_5517           0x5517
#define PCI_DEVICE_ID_SI_5518           0x5518
#define PCI_DEVICE_ID_SI_5571           0x5571
#define PCI_DEVICE_ID_SI_5581           0x5581
#define PCI_DEVICE_ID_SI_5582           0x5582
#define PCI_DEVICE_ID_SI_5591           0x5591
#define PCI_DEVICE_ID_SI_5596           0x5596
#define PCI_DEVICE_ID_SI_5597           0x5597
#define PCI_DEVICE_ID_SI_5598           0x5598
#define PCI_DEVICE_ID_SI_5600           0x5600
#define PCI_DEVICE_ID_SI_7012           0x7012
#define PCI_DEVICE_ID_SI_7013           0x7013
#define PCI_DEVICE_ID_SI_7016           0x7016
#define PCI_DEVICE_ID_SI_7018           0x7018

#define PCI_VENDOR_ID_HP                0x103c
#define PCI_DEVICE_ID_HP_VISUALIZE_EG   0x1005
#define PCI_DEVICE_ID_HP_VISUALIZE_FX6  0x1006
#define PCI_DEVICE_ID_HP_VISUALIZE_FX4  0x1008
#define PCI_DEVICE_ID_HP_VISUALIZE_FX2  0x100a
#define PCI_DEVICE_ID_HP_TACHYON        0x1028
#define PCI_DEVICE_ID_HP_TACHLITE       0x1029
#define PCI_DEVICE_ID_HP_J2585A         0x1030
#define PCI_DEVICE_ID_HP_J2585B         0x1031
#define PCI_DEVICE_ID_HP_J2973A         0x1040
#define PCI_DEVICE_ID_HP_J2970A         0x1042
#define PCI_DEVICE_ID_HP_DIVA           0x1048
#define PCI_DEVICE_ID_HP_DIVA_TOSCA1    0x1049
#define PCI_DEVICE_ID_HP_DIVA_TOSCA2    0x104A
#define PCI_DEVICE_ID_HP_DIVA_MAESTRO   0x104B
#define PCI_DEVICE_ID_HP_REO_IOC        0x10f1
#define PCI_DEVICE_ID_HP_VISUALIZE_FXE  0x108b
#define PCI_DEVICE_ID_HP_DIVA_HALFDOME  0x1223
#define PCI_DEVICE_ID_HP_DIVA_KEYSTONE  0x1226
#define PCI_DEVICE_ID_HP_DIVA_POWERBAR  0x1227
#define PCI_DEVICE_ID_HP_ZX1_IOC        0x122a
#define PCI_DEVICE_ID_HP_PCIX_LBA       0x122e
#define PCI_DEVICE_ID_HP_SX1000_IOC     0x127c
#define PCI_DEVICE_ID_HP_DIVA_EVEREST   0x1282
#define PCI_DEVICE_ID_HP_DIVA_AUX       0x1290
#define PCI_DEVICE_ID_HP_DIVA_RMP3      0x1301
#define PCI_DEVICE_ID_HP_DIVA_HURRICANE 0x132a
#define PCI_DEVICE_ID_HP_CISSA          0x3220
#define PCI_DEVICE_ID_HP_CISSC          0x3230
#define PCI_DEVICE_ID_HP_CISSD          0x3238
#define PCI_DEVICE_ID_HP_CISSE          0x323a
#define PCI_DEVICE_ID_HP_CISSF          0x323b
#define PCI_DEVICE_ID_HP_CISSH          0x323c
#define PCI_DEVICE_ID_HP_ZX2_IOC        0x4031

#define PCI_VENDOR_ID_PCTECH            0x1042
#define PCI_DEVICE_ID_PCTECH_RZ1000     0x1000
#define PCI_DEVICE_ID_PCTECH_RZ1001     0x1001
#define PCI_DEVICE_ID_PCTECH_SAMURAI_IDE 0x3020

#define PCI_VENDOR_ID_ASUSTEK           0x1043
#define PCI_DEVICE_ID_ASUSTEK_0675      0x0675

#define PCI_VENDOR_ID_DPT               0x1044
#define PCI_DEVICE_ID_DPT               0xa400

#define PCI_VENDOR_ID_OPTI              0x1045
#define PCI_DEVICE_ID_OPTI_82C558       0xc558
#define PCI_DEVICE_ID_OPTI_82C621       0xc621
#define PCI_DEVICE_ID_OPTI_82C700       0xc700
#define PCI_DEVICE_ID_OPTI_82C825       0xd568

#define PCI_VENDOR_ID_ELSA              0x1048
#define PCI_DEVICE_ID_ELSA_MICROLINK    0x1000
#define PCI_DEVICE_ID_ELSA_QS3000       0x3000

#define PCI_VENDOR_ID_STMICRO           0x104A
#define PCI_DEVICE_ID_STMICRO_USB_HOST  0xCC00
#define PCI_DEVICE_ID_STMICRO_USB_OHCI  0xCC01
#define PCI_DEVICE_ID_STMICRO_USB_OTG   0xCC02
#define PCI_DEVICE_ID_STMICRO_UART_HWFC 0xCC03
#define PCI_DEVICE_ID_STMICRO_UART_NO_HWFC      0xCC04
#define PCI_DEVICE_ID_STMICRO_SOC_DMA   0xCC05
#define PCI_DEVICE_ID_STMICRO_SATA      0xCC06
#define PCI_DEVICE_ID_STMICRO_I2C       0xCC07
#define PCI_DEVICE_ID_STMICRO_SPI_HS    0xCC08
#define PCI_DEVICE_ID_STMICRO_MAC       0xCC09
#define PCI_DEVICE_ID_STMICRO_SDIO_EMMC 0xCC0A
#define PCI_DEVICE_ID_STMICRO_SDIO      0xCC0B
#define PCI_DEVICE_ID_STMICRO_GPIO      0xCC0C
#define PCI_DEVICE_ID_STMICRO_VIP       0xCC0D
#define PCI_DEVICE_ID_STMICRO_AUDIO_ROUTER_DMA  0xCC0E
#define PCI_DEVICE_ID_STMICRO_AUDIO_ROUTER_SRCS 0xCC0F
#define PCI_DEVICE_ID_STMICRO_AUDIO_ROUTER_MSPS 0xCC10
#define PCI_DEVICE_ID_STMICRO_CAN       0xCC11
#define PCI_DEVICE_ID_STMICRO_MLB       0xCC12
#define PCI_DEVICE_ID_STMICRO_DBP       0xCC13
#define PCI_DEVICE_ID_STMICRO_SATA_PHY  0xCC14
#define PCI_DEVICE_ID_STMICRO_ESRAM     0xCC15
#define PCI_DEVICE_ID_STMICRO_VIC       0xCC16

#define PCI_VENDOR_ID_BUSLOGIC                0x104B
#define PCI_DEVICE_ID_BUSLOGIC_MULTIMASTER_NC 0x0140
#define PCI_DEVICE_ID_BUSLOGIC_MULTIMASTER    0x1040
#define PCI_DEVICE_ID_BUSLOGIC_FLASHPOINT     0x8130

#define PCI_VENDOR_ID_TI                0x104c
#define PCI_DEVICE_ID_TI_TVP4020        0x3d07
#define PCI_DEVICE_ID_TI_4450           0x8011
#define PCI_DEVICE_ID_TI_XX21_XX11      0x8031
#define PCI_DEVICE_ID_TI_XX21_XX11_FM   0x8033
#define PCI_DEVICE_ID_TI_XX21_XX11_SD   0x8034
#define PCI_DEVICE_ID_TI_X515           0x8036
#define PCI_DEVICE_ID_TI_XX12           0x8039
#define PCI_DEVICE_ID_TI_XX12_FM        0x803b
#define PCI_DEVICE_ID_TI_XIO2000A       0x8231
#define PCI_DEVICE_ID_TI_1130           0xac12
#define PCI_DEVICE_ID_TI_1031           0xac13
#define PCI_DEVICE_ID_TI_1131           0xac15
#define PCI_DEVICE_ID_TI_1250           0xac16
#define PCI_DEVICE_ID_TI_1220           0xac17
#define PCI_DEVICE_ID_TI_1221           0xac19
#define PCI_DEVICE_ID_TI_1210           0xac1a
#define PCI_DEVICE_ID_TI_1450           0xac1b
#define PCI_DEVICE_ID_TI_1225           0xac1c
#define PCI_DEVICE_ID_TI_1251A          0xac1d
#define PCI_DEVICE_ID_TI_1211           0xac1e
#define PCI_DEVICE_ID_TI_1251B          0xac1f
#define PCI_DEVICE_ID_TI_4410           0xac41
#define PCI_DEVICE_ID_TI_4451           0xac42
#define PCI_DEVICE_ID_TI_4510           0xac44
#define PCI_DEVICE_ID_TI_4520           0xac46
#define PCI_DEVICE_ID_TI_7510           0xac47
#define PCI_DEVICE_ID_TI_7610           0xac48
#define PCI_DEVICE_ID_TI_7410           0xac49
#define PCI_DEVICE_ID_TI_1410           0xac50
#define PCI_DEVICE_ID_TI_1420           0xac51
#define PCI_DEVICE_ID_TI_1451A          0xac52
#define PCI_DEVICE_ID_TI_1620           0xac54
#define PCI_DEVICE_ID_TI_1520           0xac55
#define PCI_DEVICE_ID_TI_1510           0xac56
#define PCI_DEVICE_ID_TI_X620           0xac8d
#define PCI_DEVICE_ID_TI_X420           0xac8e
#define PCI_DEVICE_ID_TI_XX20_FM        0xac8f

#define PCI_VENDOR_ID_SONY              0x104d

/* Winbond have two vendor IDs! See 0x10ad as well */
#define PCI_VENDOR_ID_WINBOND2          0x1050
#define PCI_DEVICE_ID_WINBOND2_89C940F  0x5a5a
#define PCI_DEVICE_ID_WINBOND2_6692     0x6692

#define PCI_VENDOR_ID_ANIGMA            0x1051
#define PCI_DEVICE_ID_ANIGMA_MC145575   0x0100

#define PCI_VENDOR_ID_EFAR              0x1055
#define PCI_DEVICE_ID_EFAR_SLC90E66_1   0x9130
#define PCI_DEVICE_ID_EFAR_SLC90E66_3   0x9463

#define PCI_VENDOR_ID_MOTOROLA          0x1057
#define PCI_DEVICE_ID_MOTOROLA_MPC105   0x0001
#define PCI_DEVICE_ID_MOTOROLA_MPC106   0x0002
#define PCI_DEVICE_ID_MOTOROLA_MPC107   0x0004
#define PCI_DEVICE_ID_MOTOROLA_RAVEN    0x4801
#define PCI_DEVICE_ID_MOTOROLA_FALCON   0x4802
#define PCI_DEVICE_ID_MOTOROLA_HAWK     0x4803
#define PCI_DEVICE_ID_MOTOROLA_HARRIER  0x480b
#define PCI_DEVICE_ID_MOTOROLA_MPC5200  0x5803
#define PCI_DEVICE_ID_MOTOROLA_MPC5200B 0x5809

#define PCI_VENDOR_ID_PROMISE           0x105a
#define PCI_DEVICE_ID_PROMISE_20265     0x0d30
#define PCI_DEVICE_ID_PROMISE_20267     0x4d30
#define PCI_DEVICE_ID_PROMISE_20246     0x4d33
#define PCI_DEVICE_ID_PROMISE_20262     0x4d38
#define PCI_DEVICE_ID_PROMISE_20263     0x0D38
#define PCI_DEVICE_ID_PROMISE_20268     0x4d68
#define PCI_DEVICE_ID_PROMISE_20269     0x4d69
#define PCI_DEVICE_ID_PROMISE_20270     0x6268
#define PCI_DEVICE_ID_PROMISE_20271     0x6269
#define PCI_DEVICE_ID_PROMISE_20275     0x1275
#define PCI_DEVICE_ID_PROMISE_20276     0x5275
#define PCI_DEVICE_ID_PROMISE_20277     0x7275

#define PCI_VENDOR_ID_FOXCONN           0x105b

#define PCI_VENDOR_ID_UMC               0x1060
#define PCI_DEVICE_ID_UMC_UM8673F       0x0101
#define PCI_DEVICE_ID_UMC_UM8886BF      0x673a
#define PCI_DEVICE_ID_UMC_UM8886A       0x886a

#define PCI_VENDOR_ID_PICOPOWER         0x1066
#define PCI_DEVICE_ID_PICOPOWER_PT86C523        0x0002
#define PCI_DEVICE_ID_PICOPOWER_PT86C523BBP     0x8002

#define PCI_VENDOR_ID_MYLEX             0x1069
#define PCI_DEVICE_ID_MYLEX_DAC960_P    0x0001
#define PCI_DEVICE_ID_MYLEX_DAC960_PD   0x0002
#define PCI_DEVICE_ID_MYLEX_DAC960_PG   0x0010
#define PCI_DEVICE_ID_MYLEX_DAC960_LA   0x0020
#define PCI_DEVICE_ID_MYLEX_DAC960_LP   0x0050
#define PCI_DEVICE_ID_MYLEX_DAC960_BA   0xBA56
#define PCI_DEVICE_ID_MYLEX_DAC960_GEM  0xB166

#define PCI_VENDOR_ID_APPLE             0x106b
#define PCI_DEVICE_ID_APPLE_BANDIT      0x0001
#define PCI_DEVICE_ID_APPLE_HYDRA       0x000e
#define PCI_DEVICE_ID_APPLE_UNI_N_FW    0x0018
#define PCI_DEVICE_ID_APPLE_UNI_N_AGP   0x0020
#define PCI_DEVICE_ID_APPLE_UNI_N_GMAC  0x0021
#define PCI_DEVICE_ID_APPLE_UNI_N_GMACP 0x0024
#define PCI_DEVICE_ID_APPLE_UNI_N_AGP_P 0x0027
#define PCI_DEVICE_ID_APPLE_UNI_N_AGP15 0x002d
#define PCI_DEVICE_ID_APPLE_UNI_N_PCI15 0x002e
#define PCI_DEVICE_ID_APPLE_UNI_N_GMAC2 0x0032
#define PCI_DEVICE_ID_APPLE_UNI_N_ATA   0x0033
#define PCI_DEVICE_ID_APPLE_UNI_N_AGP2  0x0034
#define PCI_DEVICE_ID_APPLE_IPID_ATA100 0x003b
#define PCI_DEVICE_ID_APPLE_K2_ATA100   0x0043
#define PCI_DEVICE_ID_APPLE_U3_AGP      0x004b
#define PCI_DEVICE_ID_APPLE_K2_GMAC     0x004c
#define PCI_DEVICE_ID_APPLE_SH_ATA      0x0050
#define PCI_DEVICE_ID_APPLE_SH_SUNGEM   0x0051
#define PCI_DEVICE_ID_APPLE_U3L_AGP     0x0058
#define PCI_DEVICE_ID_APPLE_U3H_AGP     0x0059
#define PCI_DEVICE_ID_APPLE_U4_PCIE     0x005b
#define PCI_DEVICE_ID_APPLE_IPID2_AGP   0x0066
#define PCI_DEVICE_ID_APPLE_IPID2_ATA   0x0069
#define PCI_DEVICE_ID_APPLE_IPID2_FW    0x006a
#define PCI_DEVICE_ID_APPLE_IPID2_GMAC  0x006b
#define PCI_DEVICE_ID_APPLE_TIGON3      0x1645

#define PCI_VENDOR_ID_YAMAHA            0x1073
#define PCI_DEVICE_ID_YAMAHA_724        0x0004
#define PCI_DEVICE_ID_YAMAHA_724F       0x000d
#define PCI_DEVICE_ID_YAMAHA_740        0x000a
#define PCI_DEVICE_ID_YAMAHA_740C       0x000c
#define PCI_DEVICE_ID_YAMAHA_744        0x0010
#define PCI_DEVICE_ID_YAMAHA_754        0x0012

#define PCI_VENDOR_ID_QLOGIC            0x1077
#define PCI_DEVICE_ID_QLOGIC_ISP10160   0x1016
#define PCI_DEVICE_ID_QLOGIC_ISP1020    0x1020
#define PCI_DEVICE_ID_QLOGIC_ISP1080    0x1080
#define PCI_DEVICE_ID_QLOGIC_ISP12160   0x1216
#define PCI_DEVICE_ID_QLOGIC_ISP1240    0x1240
#define PCI_DEVICE_ID_QLOGIC_ISP1280    0x1280
#define PCI_DEVICE_ID_QLOGIC_ISP2100    0x2100
#define PCI_DEVICE_ID_QLOGIC_ISP2200    0x2200
#define PCI_DEVICE_ID_QLOGIC_ISP2300    0x2300
#define PCI_DEVICE_ID_QLOGIC_ISP2312    0x2312
#define PCI_DEVICE_ID_QLOGIC_ISP2322    0x2322
#define PCI_DEVICE_ID_QLOGIC_ISP6312    0x6312
#define PCI_DEVICE_ID_QLOGIC_ISP6322    0x6322
#define PCI_DEVICE_ID_QLOGIC_ISP2422    0x2422
#define PCI_DEVICE_ID_QLOGIC_ISP2432    0x2432
#define PCI_DEVICE_ID_QLOGIC_ISP2512    0x2512
#define PCI_DEVICE_ID_QLOGIC_ISP2522    0x2522
#define PCI_DEVICE_ID_QLOGIC_ISP5422    0x5422
#define PCI_DEVICE_ID_QLOGIC_ISP5432    0x5432

#define PCI_VENDOR_ID_CYRIX             0x1078
#define PCI_DEVICE_ID_CYRIX_5510        0x0000
#define PCI_DEVICE_ID_CYRIX_PCI_MASTER  0x0001
#define PCI_DEVICE_ID_CYRIX_5520        0x0002
#define PCI_DEVICE_ID_CYRIX_5530_LEGACY 0x0100
#define PCI_DEVICE_ID_CYRIX_5530_IDE    0x0102
#define PCI_DEVICE_ID_CYRIX_5530_AUDIO  0x0103
#define PCI_DEVICE_ID_CYRIX_5530_VIDEO  0x0104

#define PCI_VENDOR_ID_CONTAQ            0x1080
#define PCI_DEVICE_ID_CONTAQ_82C693     0xc693

#define PCI_VENDOR_ID_OLICOM            0x108d
#define PCI_DEVICE_ID_OLICOM_OC2325     0x0012
#define PCI_DEVICE_ID_OLICOM_OC2183     0x0013
#define PCI_DEVICE_ID_OLICOM_OC2326     0x0014

#define PCI_VENDOR_ID_SUN               0x108e
#define PCI_DEVICE_ID_SUN_EBUS          0x1000
#define PCI_DEVICE_ID_SUN_HAPPYMEAL     0x1001
#define PCI_DEVICE_ID_SUN_RIO_EBUS      0x1100
#define PCI_DEVICE_ID_SUN_RIO_GEM       0x1101
#define PCI_DEVICE_ID_SUN_RIO_1394      0x1102
#define PCI_DEVICE_ID_SUN_RIO_USB       0x1103
#define PCI_DEVICE_ID_SUN_GEM           0x2bad
#define PCI_DEVICE_ID_SUN_SIMBA         0x5000
#define PCI_DEVICE_ID_SUN_PBM           0x8000
#define PCI_DEVICE_ID_SUN_SCHIZO        0x8001
#define PCI_DEVICE_ID_SUN_SABRE         0xa000
#define PCI_DEVICE_ID_SUN_HUMMINGBIRD   0xa001
#define PCI_DEVICE_ID_SUN_TOMATILLO     0xa801
#define PCI_DEVICE_ID_SUN_CASSINI       0xabba

#define PCI_VENDOR_ID_NI                0x1093
#define PCI_DEVICE_ID_NI_PCI2322        0xd130
#define PCI_DEVICE_ID_NI_PCI2324        0xd140
#define PCI_DEVICE_ID_NI_PCI2328        0xd150
#define PCI_DEVICE_ID_NI_PXI8422_2322   0xd190
#define PCI_DEVICE_ID_NI_PXI8422_2324   0xd1a0
#define PCI_DEVICE_ID_NI_PXI8420_2322   0xd1d0
#define PCI_DEVICE_ID_NI_PXI8420_2324   0xd1e0
#define PCI_DEVICE_ID_NI_PXI8420_2328   0xd1f0
#define PCI_DEVICE_ID_NI_PXI8420_23216  0xd1f1
#define PCI_DEVICE_ID_NI_PCI2322I       0xd250
#define PCI_DEVICE_ID_NI_PCI2324I       0xd270
#define PCI_DEVICE_ID_NI_PCI23216       0xd2b0
#define PCI_DEVICE_ID_NI_PXI8430_2322   0x7080
#define PCI_DEVICE_ID_NI_PCI8430_2322   0x70db
#define PCI_DEVICE_ID_NI_PXI8430_2324   0x70dd
#define PCI_DEVICE_ID_NI_PCI8430_2324   0x70df
#define PCI_DEVICE_ID_NI_PXI8430_2328   0x70e2
#define PCI_DEVICE_ID_NI_PCI8430_2328   0x70e4
#define PCI_DEVICE_ID_NI_PXI8430_23216  0x70e6
#define PCI_DEVICE_ID_NI_PCI8430_23216  0x70e7
#define PCI_DEVICE_ID_NI_PXI8432_2322   0x70e8
#define PCI_DEVICE_ID_NI_PCI8432_2322   0x70ea
#define PCI_DEVICE_ID_NI_PXI8432_2324   0x70ec
#define PCI_DEVICE_ID_NI_PCI8432_2324   0x70ee

#define PCI_VENDOR_ID_CMD               0x1095
#define PCI_DEVICE_ID_CMD_643           0x0643
#define PCI_DEVICE_ID_CMD_646           0x0646
#define PCI_DEVICE_ID_CMD_648           0x0648
#define PCI_DEVICE_ID_CMD_649           0x0649

#define PCI_DEVICE_ID_SII_680           0x0680
#define PCI_DEVICE_ID_SII_3112          0x3112
#define PCI_DEVICE_ID_SII_1210SA        0x0240

#define PCI_VENDOR_ID_BROOKTREE         0x109e
#define PCI_DEVICE_ID_BROOKTREE_878     0x0878
#define PCI_DEVICE_ID_BROOKTREE_879     0x0879

#define PCI_VENDOR_ID_SGI               0x10a9
#define PCI_DEVICE_ID_SGI_IOC3          0x0003
#define PCI_DEVICE_ID_SGI_LITHIUM       0x1002
#define PCI_DEVICE_ID_SGI_IOC4          0x100a

#define PCI_VENDOR_ID_WINBOND           0x10ad
#define PCI_DEVICE_ID_WINBOND_82C105    0x0105
#define PCI_DEVICE_ID_WINBOND_83C553    0x0565

#define PCI_VENDOR_ID_PLX               0x10b5
#define PCI_DEVICE_ID_PLX_R685          0x1030
#define PCI_DEVICE_ID_PLX_ROMULUS       0x106a
#define PCI_DEVICE_ID_PLX_SPCOM800      0x1076
#define PCI_DEVICE_ID_PLX_1077          0x1077
#define PCI_DEVICE_ID_PLX_SPCOM200      0x1103
#define PCI_DEVICE_ID_PLX_DJINN_ITOO    0x1151
#define PCI_DEVICE_ID_PLX_R753          0x1152
#define PCI_DEVICE_ID_PLX_OLITEC        0x1187
#define PCI_DEVICE_ID_PLX_PCI200SYN     0x3196
#define PCI_DEVICE_ID_PLX_9030          0x9030
#define PCI_DEVICE_ID_PLX_9050          0x9050
#define PCI_DEVICE_ID_PLX_9056          0x9056
#define PCI_DEVICE_ID_PLX_9080          0x9080
#define PCI_DEVICE_ID_PLX_GTEK_SERIAL2  0xa001

#define PCI_VENDOR_ID_MADGE             0x10b6
#define PCI_DEVICE_ID_MADGE_MK2         0x0002

#define PCI_VENDOR_ID_3COM              0x10b7
#define PCI_DEVICE_ID_3COM_3C985        0x0001
#define PCI_DEVICE_ID_3COM_3C940        0x1700
#define PCI_DEVICE_ID_3COM_3C339        0x3390
#define PCI_DEVICE_ID_3COM_3C359        0x3590
#define PCI_DEVICE_ID_3COM_3C940B       0x80eb
#define PCI_DEVICE_ID_3COM_3CR990       0x9900
#define PCI_DEVICE_ID_3COM_3CR990_TX_95 0x9902
#define PCI_DEVICE_ID_3COM_3CR990_TX_97 0x9903
#define PCI_DEVICE_ID_3COM_3CR990B      0x9904
#define PCI_DEVICE_ID_3COM_3CR990_FX    0x9905
#define PCI_DEVICE_ID_3COM_3CR990SVR95  0x9908
#define PCI_DEVICE_ID_3COM_3CR990SVR97  0x9909
#define PCI_DEVICE_ID_3COM_3CR990SVR    0x990a

#define PCI_VENDOR_ID_AL                0x10b9
#define PCI_DEVICE_ID_AL_M1533          0x1533
#define PCI_DEVICE_ID_AL_M1535          0x1535
#define PCI_DEVICE_ID_AL_M1541          0x1541
#define PCI_DEVICE_ID_AL_M1563          0x1563
#define PCI_DEVICE_ID_AL_M1621          0x1621
#define PCI_DEVICE_ID_AL_M1631          0x1631
#define PCI_DEVICE_ID_AL_M1632          0x1632
#define PCI_DEVICE_ID_AL_M1641          0x1641
#define PCI_DEVICE_ID_AL_M1644          0x1644
#define PCI_DEVICE_ID_AL_M1647          0x1647
#define PCI_DEVICE_ID_AL_M1651          0x1651
#define PCI_DEVICE_ID_AL_M1671          0x1671
#define PCI_DEVICE_ID_AL_M1681          0x1681
#define PCI_DEVICE_ID_AL_M1683          0x1683
#define PCI_DEVICE_ID_AL_M1689          0x1689
#define PCI_DEVICE_ID_AL_M5219          0x5219
#define PCI_DEVICE_ID_AL_M5228          0x5228
#define PCI_DEVICE_ID_AL_M5229          0x5229
#define PCI_DEVICE_ID_AL_M5451          0x5451
#define PCI_DEVICE_ID_AL_M7101          0x7101

#define PCI_VENDOR_ID_NEOMAGIC          0x10c8
#define PCI_DEVICE_ID_NEOMAGIC_NM256AV_AUDIO 0x8005
#define PCI_DEVICE_ID_NEOMAGIC_NM256ZX_AUDIO 0x8006
#define PCI_DEVICE_ID_NEOMAGIC_NM256XL_PLUS_AUDIO 0x8016

#define PCI_VENDOR_ID_TCONRAD           0x10da
#define PCI_DEVICE_ID_TCONRAD_TOKENRING 0x0508

#define PCI_VENDOR_ID_NVIDIA                    0x10de
#define PCI_DEVICE_ID_NVIDIA_TNT                0x0020
#define PCI_DEVICE_ID_NVIDIA_TNT2               0x0028
#define PCI_DEVICE_ID_NVIDIA_UTNT2              0x0029
#define PCI_DEVICE_ID_NVIDIA_TNT_UNKNOWN        0x002a
#define PCI_DEVICE_ID_NVIDIA_VTNT2              0x002C
#define PCI_DEVICE_ID_NVIDIA_UVTNT2             0x002D
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP04_SMBUS 0x0034
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP04_IDE   0x0035
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP04_SATA  0x0036
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP04_SATA2 0x003e
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_6800_ULTRA 0x0040
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_6800       0x0041
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_6800_LE    0x0042
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_6800_GT    0x0045
#define PCI_DEVICE_ID_NVIDIA_QUADRO_FX_4000     0x004E
#define PCI_DEVICE_ID_NVIDIA_NFORCE4_SMBUS      0x0052
#define PCI_DEVICE_ID_NVIDIA_NFORCE_CK804_IDE   0x0053
#define PCI_DEVICE_ID_NVIDIA_NFORCE_CK804_SATA  0x0054
#define PCI_DEVICE_ID_NVIDIA_NFORCE_CK804_SATA2 0x0055
#define PCI_DEVICE_ID_NVIDIA_CK804_AUDIO        0x0059
#define PCI_DEVICE_ID_NVIDIA_CK804_PCIE         0x005d
#define PCI_DEVICE_ID_NVIDIA_NFORCE2_SMBUS      0x0064
#define PCI_DEVICE_ID_NVIDIA_NFORCE2_IDE        0x0065
#define PCI_DEVICE_ID_NVIDIA_MCP2_MODEM         0x0069
#define PCI_DEVICE_ID_NVIDIA_MCP2_AUDIO         0x006a
#define PCI_DEVICE_ID_NVIDIA_NFORCE2S_SMBUS     0x0084
#define PCI_DEVICE_ID_NVIDIA_NFORCE2S_IDE       0x0085
#define PCI_DEVICE_ID_NVIDIA_MCP2S_MODEM        0x0089
#define PCI_DEVICE_ID_NVIDIA_CK8_AUDIO          0x008a
#define PCI_DEVICE_ID_NVIDIA_NFORCE2S_SATA      0x008e
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_7800_GT   0x0090
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_7800_GTX   0x0091
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_GO_7800   0x0098
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_GO_7800_GTX 0x0099
#define PCI_DEVICE_ID_NVIDIA_ITNT2              0x00A0
#define PCI_DEVICE_ID_GEFORCE_6800A             0x00c1
#define PCI_DEVICE_ID_GEFORCE_6800A_LE          0x00c2
#define PCI_DEVICE_ID_GEFORCE_GO_6800           0x00c8
#define PCI_DEVICE_ID_GEFORCE_GO_6800_ULTRA     0x00c9
#define PCI_DEVICE_ID_QUADRO_FX_GO1400          0x00cc
#define PCI_DEVICE_ID_QUADRO_FX_1400            0x00ce
#define PCI_DEVICE_ID_NVIDIA_NFORCE3            0x00d1
#define PCI_DEVICE_ID_NVIDIA_NFORCE3_SMBUS      0x00d4
#define PCI_DEVICE_ID_NVIDIA_NFORCE3_IDE        0x00d5
#define PCI_DEVICE_ID_NVIDIA_MCP3_MODEM         0x00d9
#define PCI_DEVICE_ID_NVIDIA_MCP3_AUDIO         0x00da
#define PCI_DEVICE_ID_NVIDIA_NFORCE3S           0x00e1
#define PCI_DEVICE_ID_NVIDIA_NFORCE3S_SATA      0x00e3
#define PCI_DEVICE_ID_NVIDIA_NFORCE3S_SMBUS     0x00e4
#define PCI_DEVICE_ID_NVIDIA_NFORCE3S_IDE       0x00e5
#define PCI_DEVICE_ID_NVIDIA_CK8S_AUDIO         0x00ea
#define PCI_DEVICE_ID_NVIDIA_NFORCE3S_SATA2     0x00ee
#define PCIE_DEVICE_ID_NVIDIA_GEFORCE_6800_ALT1 0x00f0
#define PCIE_DEVICE_ID_NVIDIA_GEFORCE_6600_ALT1 0x00f1
#define PCIE_DEVICE_ID_NVIDIA_GEFORCE_6600_ALT2 0x00f2
#define PCIE_DEVICE_ID_NVIDIA_GEFORCE_6200_ALT1 0x00f3
#define PCIE_DEVICE_ID_NVIDIA_GEFORCE_6800_GT   0x00f9
#define PCIE_DEVICE_ID_NVIDIA_QUADRO_NVS280     0x00fd
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_SDR        0x0100
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_DDR        0x0101
#define PCI_DEVICE_ID_NVIDIA_QUADRO             0x0103
#define PCI_DEVICE_ID_NVIDIA_GEFORCE2_MX        0x0110
#define PCI_DEVICE_ID_NVIDIA_GEFORCE2_MX2       0x0111
#define PCI_DEVICE_ID_NVIDIA_GEFORCE2_GO        0x0112
#define PCI_DEVICE_ID_NVIDIA_QUADRO2_MXR        0x0113
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_6600_GT    0x0140
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_6600       0x0141
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_6610_XL    0x0145
#define PCI_DEVICE_ID_NVIDIA_QUADRO_FX_540      0x014E
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_6200       0x014F
#define PCI_DEVICE_ID_NVIDIA_GEFORCE2_GTS       0x0150
#define PCI_DEVICE_ID_NVIDIA_GEFORCE2_GTS2      0x0151
#define PCI_DEVICE_ID_NVIDIA_GEFORCE2_ULTRA     0x0152
#define PCI_DEVICE_ID_NVIDIA_QUADRO2_PRO        0x0153
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_6200_TURBOCACHE 0x0161
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_GO_6200    0x0164
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_GO_6250    0x0166
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_GO_6200_1  0x0167
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_GO_6250_1  0x0168
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_MX_460    0x0170
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_MX_440    0x0171
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_MX_420    0x0172
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_MX_440_SE 0x0173
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_440_GO    0x0174
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_420_GO    0x0175
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_420_GO_M32 0x0176
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_460_GO    0x0177
#define PCI_DEVICE_ID_NVIDIA_QUADRO4_500XGL     0x0178
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_440_GO_M64 0x0179
#define PCI_DEVICE_ID_NVIDIA_QUADRO4_200        0x017A
#define PCI_DEVICE_ID_NVIDIA_QUADRO4_550XGL     0x017B
#define PCI_DEVICE_ID_NVIDIA_QUADRO4_500_GOGL   0x017C
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_410_GO_M16 0x017D
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_MX_440_8X 0x0181
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_MX_440SE_8X 0x0182
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_MX_420_8X 0x0183
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_MX_4000   0x0185
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_448_GO    0x0186
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_488_GO    0x0187
#define PCI_DEVICE_ID_NVIDIA_QUADRO4_580_XGL    0x0188
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_MX_MAC    0x0189
#define PCI_DEVICE_ID_NVIDIA_QUADRO4_280_NVS    0x018A
#define PCI_DEVICE_ID_NVIDIA_QUADRO4_380_XGL    0x018B
#define PCI_DEVICE_ID_NVIDIA_IGEFORCE2          0x01a0
#define PCI_DEVICE_ID_NVIDIA_NFORCE             0x01a4
#define PCI_DEVICE_ID_NVIDIA_MCP1_AUDIO         0x01b1
#define PCI_DEVICE_ID_NVIDIA_NFORCE_SMBUS       0x01b4
#define PCI_DEVICE_ID_NVIDIA_NFORCE_IDE         0x01bc
#define PCI_DEVICE_ID_NVIDIA_MCP1_MODEM         0x01c1
#define PCI_DEVICE_ID_NVIDIA_NFORCE2            0x01e0
#define PCI_DEVICE_ID_NVIDIA_GEFORCE3           0x0200
#define PCI_DEVICE_ID_NVIDIA_GEFORCE3_1         0x0201
#define PCI_DEVICE_ID_NVIDIA_GEFORCE3_2         0x0202
#define PCI_DEVICE_ID_NVIDIA_QUADRO_DDC         0x0203
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_6800B      0x0211
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_6800B_LE   0x0212
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_6800B_GT   0x0215
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_TI_4600   0x0250
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_TI_4400   0x0251
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_TI_4200   0x0253
#define PCI_DEVICE_ID_NVIDIA_QUADRO4_900XGL     0x0258
#define PCI_DEVICE_ID_NVIDIA_QUADRO4_750XGL     0x0259
#define PCI_DEVICE_ID_NVIDIA_QUADRO4_700XGL     0x025B
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP51_SMBUS 0x0264
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP51_IDE   0x0265
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP51_SATA  0x0266
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP51_SATA2 0x0267
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP55_SMBUS 0x0368
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP55_IDE   0x036E
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP55_SATA  0x037E
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP55_SATA2 0x037F
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_TI_4800   0x0280
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_TI_4800_8X    0x0281
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_TI_4800SE     0x0282
#define PCI_DEVICE_ID_NVIDIA_GEFORCE4_4200_GO       0x0286
#define PCI_DEVICE_ID_NVIDIA_QUADRO4_980_XGL        0x0288
#define PCI_DEVICE_ID_NVIDIA_QUADRO4_780_XGL        0x0289
#define PCI_DEVICE_ID_NVIDIA_QUADRO4_700_GOGL       0x028C
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5800_ULTRA  0x0301
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5800        0x0302
#define PCI_DEVICE_ID_NVIDIA_QUADRO_FX_2000         0x0308
#define PCI_DEVICE_ID_NVIDIA_QUADRO_FX_1000         0x0309
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5600_ULTRA  0x0311
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5600        0x0312
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5600SE      0x0314
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_GO5600      0x031A
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_GO5650      0x031B
#define PCI_DEVICE_ID_NVIDIA_QUADRO_FX_GO700        0x031C
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5200        0x0320
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5200_ULTRA  0x0321
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5200_1      0x0322
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5200SE      0x0323
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_GO5200      0x0324
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_GO5250      0x0325
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5500        0x0326
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5100        0x0327
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_GO5250_32   0x0328
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_GO_5200     0x0329
#define PCI_DEVICE_ID_NVIDIA_QUADRO_NVS_280_PCI     0x032A
#define PCI_DEVICE_ID_NVIDIA_QUADRO_FX_500          0x032B
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_GO5300      0x032C
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_GO5100      0x032D
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5900_ULTRA  0x0330
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5900        0x0331
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5900XT      0x0332
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5950_ULTRA  0x0333
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5900ZT      0x0334
#define PCI_DEVICE_ID_NVIDIA_QUADRO_FX_3000         0x0338
#define PCI_DEVICE_ID_NVIDIA_QUADRO_FX_700          0x033F
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5700_ULTRA  0x0341
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5700        0x0342
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5700LE      0x0343
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_5700VE      0x0344
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_GO5700_1    0x0347
#define PCI_DEVICE_ID_NVIDIA_GEFORCE_FX_GO5700_2    0x0348
#define PCI_DEVICE_ID_NVIDIA_QUADRO_FX_GO1000       0x034C
#define PCI_DEVICE_ID_NVIDIA_QUADRO_FX_1100         0x034E
#define PCI_DEVICE_ID_NVIDIA_MCP55_BRIDGE_V0        0x0360
#define PCI_DEVICE_ID_NVIDIA_MCP55_BRIDGE_V4        0x0364
#define PCI_DEVICE_ID_NVIDIA_NVENET_15              0x0373
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP61_SATA      0x03E7
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP61_SMBUS     0x03EB
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP61_IDE       0x03EC
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP61_SATA2     0x03F6
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP61_SATA3     0x03F7
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP65_SMBUS     0x0446
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP65_IDE       0x0448
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP67_SMBUS     0x0542
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP67_IDE       0x0560
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP73_IDE       0x056C
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP78S_SMBUS    0x0752
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP77_IDE       0x0759
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP73_SMBUS     0x07D8
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP79_SMBUS     0x0AA2
#define PCI_DEVICE_ID_NVIDIA_NFORCE_MCP89_SATA      0x0D85

#define PCI_VENDOR_ID_IMS               0x10e0
#define PCI_DEVICE_ID_IMS_TT128         0x9128
#define PCI_DEVICE_ID_IMS_TT3D          0x9135

#define PCI_VENDOR_ID_INTERG            0x10ea
#define PCI_DEVICE_ID_INTERG_1682       0x1682
#define PCI_DEVICE_ID_INTERG_2000       0x2000
#define PCI_DEVICE_ID_INTERG_2010       0x2010
#define PCI_DEVICE_ID_INTERG_5000       0x5000
#define PCI_DEVICE_ID_INTERG_5050       0x5050

#define PCI_VENDOR_ID_REALTEK           0x10ec
#define PCI_DEVICE_ID_REALTEK_8139      0x8139

#define PCI_VENDOR_ID_XILINX            0x10ee
#define PCI_DEVICE_ID_RME_DIGI96        0x3fc0
#define PCI_DEVICE_ID_RME_DIGI96_8      0x3fc1
#define PCI_DEVICE_ID_RME_DIGI96_8_PRO  0x3fc2
#define PCI_DEVICE_ID_RME_DIGI96_8_PAD_OR_PST 0x3fc3
#define PCI_DEVICE_ID_XILINX_HAMMERFALL_DSP 0x3fc5
#define PCI_DEVICE_ID_XILINX_HAMMERFALL_DSP_MADI 0x3fc6

#define PCI_VENDOR_ID_INIT              0x1101

#define PCI_VENDOR_ID_CREATIVE          0x1102 /* duplicate: ECTIVA */
#define PCI_DEVICE_ID_CREATIVE_EMU10K1  0x0002
#define PCI_DEVICE_ID_CREATIVE_20K1     0x0005
#define PCI_DEVICE_ID_CREATIVE_20K2     0x000b
#define PCI_SUBDEVICE_ID_CREATIVE_SB0760        0x0024
#define PCI_SUBDEVICE_ID_CREATIVE_SB08801       0x0041
#define PCI_SUBDEVICE_ID_CREATIVE_SB08802       0x0042
#define PCI_SUBDEVICE_ID_CREATIVE_SB08803       0x0043
#define PCI_SUBDEVICE_ID_CREATIVE_SB1270        0x0062
#define PCI_SUBDEVICE_ID_CREATIVE_HENDRIX       0x6000

#define PCI_VENDOR_ID_ECTIVA            0x1102 /* duplicate: CREATIVE */
#define PCI_DEVICE_ID_ECTIVA_EV1938     0x8938

#define PCI_VENDOR_ID_TTI               0x1103
#define PCI_DEVICE_ID_TTI_HPT343        0x0003
#define PCI_DEVICE_ID_TTI_HPT366        0x0004
#define PCI_DEVICE_ID_TTI_HPT372        0x0005
#define PCI_DEVICE_ID_TTI_HPT302        0x0006
#define PCI_DEVICE_ID_TTI_HPT371        0x0007
#define PCI_DEVICE_ID_TTI_HPT374        0x0008
#define PCI_DEVICE_ID_TTI_HPT372N       0x0009  /* apparently a 372N variant? */

#define PCI_VENDOR_ID_VIA               0x1106
#define PCI_DEVICE_ID_VIA_8763_0        0x0198
#define PCI_DEVICE_ID_VIA_8380_0        0x0204
#define PCI_DEVICE_ID_VIA_3238_0        0x0238
#define PCI_DEVICE_ID_VIA_PT880         0x0258
#define PCI_DEVICE_ID_VIA_PT880ULTRA    0x0308
#define PCI_DEVICE_ID_VIA_PX8X0_0       0x0259
#define PCI_DEVICE_ID_VIA_3269_0        0x0269
#define PCI_DEVICE_ID_VIA_K8T800PRO_0   0x0282
#define PCI_DEVICE_ID_VIA_3296_0        0x0296
#define PCI_DEVICE_ID_VIA_8363_0        0x0305
#define PCI_DEVICE_ID_VIA_P4M800CE      0x0314
#define PCI_DEVICE_ID_VIA_P4M890        0x0327
#define PCI_DEVICE_ID_VIA_VT3324        0x0324
#define PCI_DEVICE_ID_VIA_VT3336        0x0336
#define PCI_DEVICE_ID_VIA_VT3351        0x0351
#define PCI_DEVICE_ID_VIA_VT3364        0x0364
#define PCI_DEVICE_ID_VIA_8371_0        0x0391
#define PCI_DEVICE_ID_VIA_6415          0x0415
#define PCI_DEVICE_ID_VIA_8501_0        0x0501
#define PCI_DEVICE_ID_VIA_82C561        0x0561
#define PCI_DEVICE_ID_VIA_82C586_1      0x0571
#define PCI_DEVICE_ID_VIA_82C576        0x0576
#define PCI_DEVICE_ID_VIA_82C586_0      0x0586
#define PCI_DEVICE_ID_VIA_82C596        0x0596
#define PCI_DEVICE_ID_VIA_82C597_0      0x0597
#define PCI_DEVICE_ID_VIA_82C598_0      0x0598
#define PCI_DEVICE_ID_VIA_8601_0        0x0601
#define PCI_DEVICE_ID_VIA_8605_0        0x0605
#define PCI_DEVICE_ID_VIA_82C686        0x0686
#define PCI_DEVICE_ID_VIA_82C691_0      0x0691
#define PCI_DEVICE_ID_VIA_82C576_1      0x1571
#define PCI_DEVICE_ID_VIA_82C586_2      0x3038
#define PCI_DEVICE_ID_VIA_82C586_3      0x3040
#define PCI_DEVICE_ID_VIA_82C596_3      0x3050
#define PCI_DEVICE_ID_VIA_82C596B_3     0x3051
#define PCI_DEVICE_ID_VIA_82C686_4      0x3057
#define PCI_DEVICE_ID_VIA_82C686_5      0x3058
#define PCI_DEVICE_ID_VIA_8233_5        0x3059
#define PCI_DEVICE_ID_VIA_8233_0        0x3074
#define PCI_DEVICE_ID_VIA_8633_0        0x3091
#define PCI_DEVICE_ID_VIA_8367_0        0x3099
#define PCI_DEVICE_ID_VIA_8653_0        0x3101
#define PCI_DEVICE_ID_VIA_8622          0x3102
#define PCI_DEVICE_ID_VIA_8235_USB_2    0x3104
#define PCI_DEVICE_ID_VIA_8233C_0       0x3109
#define PCI_DEVICE_ID_VIA_8361          0x3112
#define PCI_DEVICE_ID_VIA_XM266         0x3116
#define PCI_DEVICE_ID_VIA_612X          0x3119
#define PCI_DEVICE_ID_VIA_862X_0        0x3123
#define PCI_DEVICE_ID_VIA_8753_0        0x3128
#define PCI_DEVICE_ID_VIA_8233A         0x3147
#define PCI_DEVICE_ID_VIA_8703_51_0     0x3148
#define PCI_DEVICE_ID_VIA_8237_SATA     0x3149
#define PCI_DEVICE_ID_VIA_XN266         0x3156
#define PCI_DEVICE_ID_VIA_6410          0x3164
#define PCI_DEVICE_ID_VIA_8754C_0       0x3168
#define PCI_DEVICE_ID_VIA_8235          0x3177
#define PCI_DEVICE_ID_VIA_8385_0        0x3188
#define PCI_DEVICE_ID_VIA_8377_0        0x3189
#define PCI_DEVICE_ID_VIA_8378_0        0x3205
#define PCI_DEVICE_ID_VIA_8783_0        0x3208
#define PCI_DEVICE_ID_VIA_8237          0x3227
#define PCI_DEVICE_ID_VIA_8251          0x3287
#define PCI_DEVICE_ID_VIA_8261          0x3402
#define PCI_DEVICE_ID_VIA_8237A         0x3337
#define PCI_DEVICE_ID_VIA_8237S         0x3372
#define PCI_DEVICE_ID_VIA_SATA_EIDE     0x5324
#define PCI_DEVICE_ID_VIA_8231          0x8231
#define PCI_DEVICE_ID_VIA_8231_4        0x8235
#define PCI_DEVICE_ID_VIA_8365_1        0x8305
#define PCI_DEVICE_ID_VIA_CX700         0x8324
#define PCI_DEVICE_ID_VIA_CX700_IDE     0x0581
#define PCI_DEVICE_ID_VIA_VX800         0x8353
#define PCI_DEVICE_ID_VIA_VX855         0x8409
#define PCI_DEVICE_ID_VIA_VX900         0x8410
#define PCI_DEVICE_ID_VIA_8371_1        0x8391
#define PCI_DEVICE_ID_VIA_82C598_1      0x8598
#define PCI_DEVICE_ID_VIA_838X_1        0xB188
#define PCI_DEVICE_ID_VIA_83_87XX_1     0xB198
#define PCI_DEVICE_ID_VIA_VX855_IDE     0xC409
#define PCI_DEVICE_ID_VIA_ANON          0xFFFF

#define PCI_VENDOR_ID_SIEMENS           0x110A
#define PCI_DEVICE_ID_SIEMENS_DSCC4     0x2102

#define PCI_VENDOR_ID_VORTEX            0x1119
#define PCI_DEVICE_ID_VORTEX_GDT60x0    0x0000
#define PCI_DEVICE_ID_VORTEX_GDT6000B   0x0001
#define PCI_DEVICE_ID_VORTEX_GDT6x10    0x0002
#define PCI_DEVICE_ID_VORTEX_GDT6x20    0x0003
#define PCI_DEVICE_ID_VORTEX_GDT6530    0x0004
#define PCI_DEVICE_ID_VORTEX_GDT6550    0x0005
#define PCI_DEVICE_ID_VORTEX_GDT6x17    0x0006
#define PCI_DEVICE_ID_VORTEX_GDT6x27    0x0007
#define PCI_DEVICE_ID_VORTEX_GDT6537    0x0008
#define PCI_DEVICE_ID_VORTEX_GDT6557    0x0009
#define PCI_DEVICE_ID_VORTEX_GDT6x15    0x000a
#define PCI_DEVICE_ID_VORTEX_GDT6x25    0x000b
#define PCI_DEVICE_ID_VORTEX_GDT6535    0x000c
#define PCI_DEVICE_ID_VORTEX_GDT6555    0x000d
#define PCI_DEVICE_ID_VORTEX_GDT6x17RP  0x0100
#define PCI_DEVICE_ID_VORTEX_GDT6x27RP  0x0101
#define PCI_DEVICE_ID_VORTEX_GDT6537RP  0x0102
#define PCI_DEVICE_ID_VORTEX_GDT6557RP  0x0103
#define PCI_DEVICE_ID_VORTEX_GDT6x11RP  0x0104
#define PCI_DEVICE_ID_VORTEX_GDT6x21RP  0x0105

#define PCI_VENDOR_ID_EF                0x111a
#define PCI_DEVICE_ID_EF_ATM_FPGA       0x0000
#define PCI_DEVICE_ID_EF_ATM_ASIC       0x0002
#define PCI_DEVICE_ID_EF_ATM_LANAI2     0x0003
#define PCI_DEVICE_ID_EF_ATM_LANAIHB    0x0005

#define PCI_VENDOR_ID_IDT               0x111d
#define PCI_DEVICE_ID_IDT_IDT77201      0x0001

#define PCI_VENDOR_ID_FORE              0x1127
#define PCI_DEVICE_ID_FORE_PCA200E      0x0300

#define PCI_VENDOR_ID_PHILIPS           0x1131
#define PCI_DEVICE_ID_PHILIPS_SAA7146   0x7146
#define PCI_DEVICE_ID_PHILIPS_SAA9730   0x9730

#define PCI_VENDOR_ID_EICON             0x1133
#define PCI_DEVICE_ID_EICON_DIVA20      0xe002
#define PCI_DEVICE_ID_EICON_DIVA20_U    0xe004
#define PCI_DEVICE_ID_EICON_DIVA201     0xe005
#define PCI_DEVICE_ID_EICON_DIVA202     0xe00b
#define PCI_DEVICE_ID_EICON_MAESTRA     0xe010
#define PCI_DEVICE_ID_EICON_MAESTRAQ    0xe012
#define PCI_DEVICE_ID_EICON_MAESTRAQ_U  0xe013
#define PCI_DEVICE_ID_EICON_MAESTRAP    0xe014

#define PCI_VENDOR_ID_CISCO             0x1137

#define PCI_VENDOR_ID_ZIATECH           0x1138
#define PCI_DEVICE_ID_ZIATECH_5550_HC   0x5550

#define PCI_VENDOR_ID_SYSKONNECT        0x1148
#define PCI_DEVICE_ID_SYSKONNECT_TR     0x4200
#define PCI_DEVICE_ID_SYSKONNECT_GE     0x4300
#define PCI_DEVICE_ID_SYSKONNECT_YU     0x4320
#define PCI_DEVICE_ID_SYSKONNECT_9DXX   0x4400
#define PCI_DEVICE_ID_SYSKONNECT_9MXX   0x4500

#define PCI_VENDOR_ID_DIGI              0x114f
#define PCI_DEVICE_ID_DIGI_DF_M_IOM2_E  0x0070
#define PCI_DEVICE_ID_DIGI_DF_M_E       0x0071
#define PCI_DEVICE_ID_DIGI_DF_M_IOM2_A  0x0072
#define PCI_DEVICE_ID_DIGI_DF_M_A       0x0073
#define PCI_DEVICE_ID_DIGI_NEO_8        0x00B1
#define PCI_DEVICE_ID_NEO_2DB9          0x00C8
#define PCI_DEVICE_ID_NEO_2DB9PRI       0x00C9
#define PCI_DEVICE_ID_NEO_2RJ45         0x00CA
#define PCI_DEVICE_ID_NEO_2RJ45PRI      0x00CB
#define PCIE_DEVICE_ID_NEO_4_IBM        0x00F4

#define PCI_VENDOR_ID_XIRCOM            0x115d
#define PCI_DEVICE_ID_XIRCOM_RBM56G     0x0101
#define PCI_DEVICE_ID_XIRCOM_X3201_MDM  0x0103

#define PCI_VENDOR_ID_SERVERWORKS         0x1166
#define PCI_DEVICE_ID_SERVERWORKS_HE      0x0008
#define PCI_DEVICE_ID_SERVERWORKS_LE      0x0009
#define PCI_DEVICE_ID_SERVERWORKS_GCNB_LE 0x0017
#define PCI_DEVICE_ID_SERVERWORKS_HT1000_PXB    0x0036
#define PCI_DEVICE_ID_SERVERWORKS_EPB     0x0103
#define PCI_DEVICE_ID_SERVERWORKS_HT2000_PCIE   0x0132
#define PCI_DEVICE_ID_SERVERWORKS_OSB4    0x0200
#define PCI_DEVICE_ID_SERVERWORKS_CSB5    0x0201
#define PCI_DEVICE_ID_SERVERWORKS_CSB6    0x0203
#define PCI_DEVICE_ID_SERVERWORKS_HT1000SB 0x0205
#define PCI_DEVICE_ID_SERVERWORKS_OSB4IDE 0x0211
#define PCI_DEVICE_ID_SERVERWORKS_CSB5IDE 0x0212
#define PCI_DEVICE_ID_SERVERWORKS_CSB6IDE 0x0213
#define PCI_DEVICE_ID_SERVERWORKS_HT1000IDE 0x0214
#define PCI_DEVICE_ID_SERVERWORKS_CSB6IDE2 0x0217
#define PCI_DEVICE_ID_SERVERWORKS_CSB6LPC 0x0227
#define PCI_DEVICE_ID_SERVERWORKS_HT1100LD 0x0408

#define PCI_VENDOR_ID_SBE               0x1176
#define PCI_DEVICE_ID_SBE_WANXL100      0x0301
#define PCI_DEVICE_ID_SBE_WANXL200      0x0302
#define PCI_DEVICE_ID_SBE_WANXL400      0x0104
#define PCI_SUBDEVICE_ID_SBE_T3E3       0x0009
#define PCI_SUBDEVICE_ID_SBE_2T3E3_P0   0x0901
#define PCI_SUBDEVICE_ID_SBE_2T3E3_P1   0x0902

#define PCI_VENDOR_ID_TOSHIBA           0x1179
#define PCI_DEVICE_ID_TOSHIBA_PICCOLO_1 0x0101
#define PCI_DEVICE_ID_TOSHIBA_PICCOLO_2 0x0102
#define PCI_DEVICE_ID_TOSHIBA_PICCOLO_3 0x0103
#define PCI_DEVICE_ID_TOSHIBA_PICCOLO_5 0x0105
#define PCI_DEVICE_ID_TOSHIBA_TOPIC95   0x060a
#define PCI_DEVICE_ID_TOSHIBA_TOPIC97   0x060f
#define PCI_DEVICE_ID_TOSHIBA_TOPIC100  0x0617

#define PCI_VENDOR_ID_TOSHIBA_2         0x102f
#define PCI_DEVICE_ID_TOSHIBA_TC35815CF 0x0030
#define PCI_DEVICE_ID_TOSHIBA_TC35815_NWU       0x0031
#define PCI_DEVICE_ID_TOSHIBA_TC35815_TX4939    0x0032
#define PCI_DEVICE_ID_TOSHIBA_TC86C001_IDE      0x0105
#define PCI_DEVICE_ID_TOSHIBA_TC86C001_MISC     0x0108
#define PCI_DEVICE_ID_TOSHIBA_SPIDER_NET 0x01b3

#define PCI_VENDOR_ID_ATTO              0x117c

#define PCI_VENDOR_ID_RICOH             0x1180
#define PCI_DEVICE_ID_RICOH_RL5C465     0x0465
#define PCI_DEVICE_ID_RICOH_RL5C466     0x0466
#define PCI_DEVICE_ID_RICOH_RL5C475     0x0475
#define PCI_DEVICE_ID_RICOH_RL5C476     0x0476
#define PCI_DEVICE_ID_RICOH_RL5C478     0x0478
#define PCI_DEVICE_ID_RICOH_R5C822      0x0822
#define PCI_DEVICE_ID_RICOH_R5CE822     0xe822
#define PCI_DEVICE_ID_RICOH_R5CE823     0xe823
#define PCI_DEVICE_ID_RICOH_R5C832      0x0832
#define PCI_DEVICE_ID_RICOH_R5C843      0x0843

#define PCI_VENDOR_ID_DLINK             0x1186
#define PCI_DEVICE_ID_DLINK_DGE510T     0x4c00

#define PCI_VENDOR_ID_ARTOP             0x1191
#define PCI_DEVICE_ID_ARTOP_ATP850UF    0x0005
#define PCI_DEVICE_ID_ARTOP_ATP860      0x0006
#define PCI_DEVICE_ID_ARTOP_ATP860R     0x0007
#define PCI_DEVICE_ID_ARTOP_ATP865      0x0008
#define PCI_DEVICE_ID_ARTOP_ATP865R     0x0009
#define PCI_DEVICE_ID_ARTOP_ATP867A     0x000A
#define PCI_DEVICE_ID_ARTOP_ATP867B     0x000B
#define PCI_DEVICE_ID_ARTOP_AEC7610     0x8002
#define PCI_DEVICE_ID_ARTOP_AEC7612UW   0x8010
#define PCI_DEVICE_ID_ARTOP_AEC7612U    0x8020
#define PCI_DEVICE_ID_ARTOP_AEC7612S    0x8030
#define PCI_DEVICE_ID_ARTOP_AEC7612D    0x8040
#define PCI_DEVICE_ID_ARTOP_AEC7612SUW  0x8050
#define PCI_DEVICE_ID_ARTOP_8060        0x8060

#define PCI_VENDOR_ID_ZEITNET           0x1193
#define PCI_DEVICE_ID_ZEITNET_1221      0x0001
#define PCI_DEVICE_ID_ZEITNET_1225      0x0002

#define PCI_VENDOR_ID_FUJITSU_ME        0x119e
#define PCI_DEVICE_ID_FUJITSU_FS155     0x0001
#define PCI_DEVICE_ID_FUJITSU_FS50      0x0003

#define PCI_SUBVENDOR_ID_KEYSPAN        0x11a9
#define PCI_SUBDEVICE_ID_KEYSPAN_SX2    0x5334

#define PCI_VENDOR_ID_MARVELL           0x11ab
#define PCI_VENDOR_ID_MARVELL_EXT       0x1b4b
#define PCI_DEVICE_ID_MARVELL_GT64111   0x4146
#define PCI_DEVICE_ID_MARVELL_GT64260   0x6430
#define PCI_DEVICE_ID_MARVELL_MV64360   0x6460
#define PCI_DEVICE_ID_MARVELL_MV64460   0x6480
#define PCI_DEVICE_ID_MARVELL_88ALP01_NAND      0x4100
#define PCI_DEVICE_ID_MARVELL_88ALP01_SD        0x4101
#define PCI_DEVICE_ID_MARVELL_88ALP01_CCIC      0x4102

#define PCI_VENDOR_ID_V3                0x11b0
#define PCI_DEVICE_ID_V3_V960           0x0001
#define PCI_DEVICE_ID_V3_V351           0x0002

#define PCI_VENDOR_ID_ATT               0x11c1
#define PCI_DEVICE_ID_ATT_VENUS_MODEM   0x480

#define PCI_VENDOR_ID_SPECIALIX         0x11cb
#define PCI_DEVICE_ID_SPECIALIX_IO8     0x2000
#define PCI_DEVICE_ID_SPECIALIX_RIO     0x8000
#define PCI_SUBDEVICE_ID_SPECIALIX_SPEED4 0xa004

#define PCI_VENDOR_ID_ANALOG_DEVICES    0x11d4
#define PCI_DEVICE_ID_AD1889JS          0x1889

#define PCI_DEVICE_ID_SEGA_BBA          0x1234

#define PCI_VENDOR_ID_ZORAN             0x11de
#define PCI_DEVICE_ID_ZORAN_36057       0x6057
#define PCI_DEVICE_ID_ZORAN_36120       0x6120

#define PCI_VENDOR_ID_COMPEX            0x11f6
#define PCI_DEVICE_ID_COMPEX_ENET100VG4 0x0112

#define PCI_VENDOR_ID_PMC_Sierra        0x11f8

#define PCI_VENDOR_ID_RP                0x11fe
#define PCI_DEVICE_ID_RP32INTF          0x0001
#define PCI_DEVICE_ID_RP8INTF           0x0002
#define PCI_DEVICE_ID_RP16INTF          0x0003
#define PCI_DEVICE_ID_RP4QUAD           0x0004
#define PCI_DEVICE_ID_RP8OCTA           0x0005
#define PCI_DEVICE_ID_RP8J              0x0006
#define PCI_DEVICE_ID_RP4J              0x0007
#define PCI_DEVICE_ID_RP8SNI            0x0008
#define PCI_DEVICE_ID_RP16SNI           0x0009
#define PCI_DEVICE_ID_RPP4              0x000A
#define PCI_DEVICE_ID_RPP8              0x000B
#define PCI_DEVICE_ID_RP4M              0x000D
#define PCI_DEVICE_ID_RP2_232           0x000E
#define PCI_DEVICE_ID_RP2_422           0x000F
#define PCI_DEVICE_ID_URP32INTF         0x0801
#define PCI_DEVICE_ID_URP8INTF          0x0802
#define PCI_DEVICE_ID_URP16INTF         0x0803
#define PCI_DEVICE_ID_URP8OCTA          0x0805
#define PCI_DEVICE_ID_UPCI_RM3_8PORT    0x080C
#define PCI_DEVICE_ID_UPCI_RM3_4PORT    0x080D
#define PCI_DEVICE_ID_CRP16INTF         0x0903

#define PCI_VENDOR_ID_CYCLADES          0x120e
#define PCI_DEVICE_ID_CYCLOM_Y_Lo       0x0100
#define PCI_DEVICE_ID_CYCLOM_Y_Hi       0x0101
#define PCI_DEVICE_ID_CYCLOM_4Y_Lo      0x0102
#define PCI_DEVICE_ID_CYCLOM_4Y_Hi      0x0103
#define PCI_DEVICE_ID_CYCLOM_8Y_Lo      0x0104
#define PCI_DEVICE_ID_CYCLOM_8Y_Hi      0x0105
#define PCI_DEVICE_ID_CYCLOM_Z_Lo       0x0200
#define PCI_DEVICE_ID_CYCLOM_Z_Hi       0x0201
#define PCI_DEVICE_ID_PC300_RX_2        0x0300
#define PCI_DEVICE_ID_PC300_RX_1        0x0301
#define PCI_DEVICE_ID_PC300_TE_2        0x0310
#define PCI_DEVICE_ID_PC300_TE_1        0x0311
#define PCI_DEVICE_ID_PC300_TE_M_2      0x0320
#define PCI_DEVICE_ID_PC300_TE_M_1      0x0321

#define PCI_VENDOR_ID_ESSENTIAL         0x120f
#define PCI_DEVICE_ID_ESSENTIAL_ROADRUNNER      0x0001

#define PCI_VENDOR_ID_O2                0x1217
#define PCI_DEVICE_ID_O2_6729           0x6729
#define PCI_DEVICE_ID_O2_6730           0x673a
#define PCI_DEVICE_ID_O2_6832           0x6832
#define PCI_DEVICE_ID_O2_6836           0x6836
#define PCI_DEVICE_ID_O2_6812           0x6872
#define PCI_DEVICE_ID_O2_6933           0x6933
#define PCI_DEVICE_ID_O2_8120           0x8120
#define PCI_DEVICE_ID_O2_8220           0x8220
#define PCI_DEVICE_ID_O2_8221           0x8221
#define PCI_DEVICE_ID_O2_8320           0x8320
#define PCI_DEVICE_ID_O2_8321           0x8321

#define PCI_VENDOR_ID_3DFX              0x121a
#define PCI_DEVICE_ID_3DFX_VOODOO       0x0001
#define PCI_DEVICE_ID_3DFX_VOODOO2      0x0002
#define PCI_DEVICE_ID_3DFX_BANSHEE      0x0003
#define PCI_DEVICE_ID_3DFX_VOODOO3      0x0005
#define PCI_DEVICE_ID_3DFX_VOODOO5      0x0009

#define PCI_VENDOR_ID_AVM               0x1244
#define PCI_DEVICE_ID_AVM_B1            0x0700
#define PCI_DEVICE_ID_AVM_C4            0x0800
#define PCI_DEVICE_ID_AVM_A1            0x0a00
#define PCI_DEVICE_ID_AVM_A1_V2         0x0e00
#define PCI_DEVICE_ID_AVM_C2            0x1100
#define PCI_DEVICE_ID_AVM_T1            0x1200

#define PCI_VENDOR_ID_STALLION          0x124d

/* Allied Telesyn */
#define PCI_VENDOR_ID_AT                0x1259
#define PCI_SUBDEVICE_ID_AT_2700FX      0x2701
#define PCI_SUBDEVICE_ID_AT_2701FX      0x2703

#define PCI_VENDOR_ID_ESS               0x125d
#define PCI_DEVICE_ID_ESS_ESS1968       0x1968
#define PCI_DEVICE_ID_ESS_ESS1978       0x1978
#define PCI_DEVICE_ID_ESS_ALLEGRO_1     0x1988
#define PCI_DEVICE_ID_ESS_ALLEGRO       0x1989
#define PCI_DEVICE_ID_ESS_CANYON3D_2LE  0x1990
#define PCI_DEVICE_ID_ESS_CANYON3D_2    0x1992
#define PCI_DEVICE_ID_ESS_MAESTRO3      0x1998
#define PCI_DEVICE_ID_ESS_MAESTRO3_1    0x1999
#define PCI_DEVICE_ID_ESS_MAESTRO3_HW   0x199a
#define PCI_DEVICE_ID_ESS_MAESTRO3_2    0x199b

#define PCI_VENDOR_ID_SATSAGEM          0x1267
#define PCI_DEVICE_ID_SATSAGEM_NICCY    0x1016

#define PCI_VENDOR_ID_ENSONIQ           0x1274
#define PCI_DEVICE_ID_ENSONIQ_CT5880    0x5880
#define PCI_DEVICE_ID_ENSONIQ_ES1370    0x5000
#define PCI_DEVICE_ID_ENSONIQ_ES1371    0x1371

#define PCI_VENDOR_ID_TRANSMETA         0x1279
#define PCI_DEVICE_ID_EFFICEON          0x0060

#define PCI_VENDOR_ID_ROCKWELL          0x127A

#define PCI_VENDOR_ID_ITE               0x1283
#define PCI_DEVICE_ID_ITE_8172          0x8172
#define PCI_DEVICE_ID_ITE_8211          0x8211
#define PCI_DEVICE_ID_ITE_8212          0x8212
#define PCI_DEVICE_ID_ITE_8213          0x8213
#define PCI_DEVICE_ID_ITE_8152          0x8152
#define PCI_DEVICE_ID_ITE_8872          0x8872
#define PCI_DEVICE_ID_ITE_IT8330G_0     0xe886

/* formerly Platform Tech */
#define PCI_DEVICE_ID_ESS_ESS0100       0x0100

#define PCI_VENDOR_ID_ALTEON            0x12ae

#define PCI_SUBVENDOR_ID_CONNECT_TECH                   0x12c4
#define PCI_SUBDEVICE_ID_CONNECT_TECH_BH8_232           0x0001
#define PCI_SUBDEVICE_ID_CONNECT_TECH_BH4_232           0x0002
#define PCI_SUBDEVICE_ID_CONNECT_TECH_BH2_232           0x0003
#define PCI_SUBDEVICE_ID_CONNECT_TECH_BH8_485           0x0004
#define PCI_SUBDEVICE_ID_CONNECT_TECH_BH8_485_4_4       0x0005
#define PCI_SUBDEVICE_ID_CONNECT_TECH_BH4_485           0x0006
#define PCI_SUBDEVICE_ID_CONNECT_TECH_BH4_485_2_2       0x0007
#define PCI_SUBDEVICE_ID_CONNECT_TECH_BH2_485           0x0008
#define PCI_SUBDEVICE_ID_CONNECT_TECH_BH8_485_2_6       0x0009
#define PCI_SUBDEVICE_ID_CONNECT_TECH_BH081101V1        0x000A
#define PCI_SUBDEVICE_ID_CONNECT_TECH_BH041101V1        0x000B
#define PCI_SUBDEVICE_ID_CONNECT_TECH_BH2_20MHZ         0x000C
#define PCI_SUBDEVICE_ID_CONNECT_TECH_BH2_PTM           0x000D
#define PCI_SUBDEVICE_ID_CONNECT_TECH_NT960PCI          0x0100
#define PCI_SUBDEVICE_ID_CONNECT_TECH_TITAN_2           0x0201
#define PCI_SUBDEVICE_ID_CONNECT_TECH_TITAN_4           0x0202
#define PCI_SUBDEVICE_ID_CONNECT_TECH_PCI_UART_2_232    0x0300
#define PCI_SUBDEVICE_ID_CONNECT_TECH_PCI_UART_4_232    0x0301
#define PCI_SUBDEVICE_ID_CONNECT_TECH_PCI_UART_8_232    0x0302
#define PCI_SUBDEVICE_ID_CONNECT_TECH_PCI_UART_1_1      0x0310
#define PCI_SUBDEVICE_ID_CONNECT_TECH_PCI_UART_2_2      0x0311
#define PCI_SUBDEVICE_ID_CONNECT_TECH_PCI_UART_4_4      0x0312
#define PCI_SUBDEVICE_ID_CONNECT_TECH_PCI_UART_2        0x0320
#define PCI_SUBDEVICE_ID_CONNECT_TECH_PCI_UART_4        0x0321
#define PCI_SUBDEVICE_ID_CONNECT_TECH_PCI_UART_8        0x0322
#define PCI_SUBDEVICE_ID_CONNECT_TECH_PCI_UART_2_485    0x0330
#define PCI_SUBDEVICE_ID_CONNECT_TECH_PCI_UART_4_485    0x0331
#define PCI_SUBDEVICE_ID_CONNECT_TECH_PCI_UART_8_485    0x0332

#define PCI_VENDOR_ID_NVIDIA_SGS        0x12d2
#define PCI_DEVICE_ID_NVIDIA_SGS_RIVA128 0x0018

#define PCI_SUBVENDOR_ID_CHASE_PCIFAST          0x12E0
#define PCI_SUBDEVICE_ID_CHASE_PCIFAST4         0x0031
#define PCI_SUBDEVICE_ID_CHASE_PCIFAST8         0x0021
#define PCI_SUBDEVICE_ID_CHASE_PCIFAST16        0x0011
#define PCI_SUBDEVICE_ID_CHASE_PCIFAST16FMC     0x0041
#define PCI_SUBVENDOR_ID_CHASE_PCIRAS           0x124D
#define PCI_SUBDEVICE_ID_CHASE_PCIRAS4          0xF001
#define PCI_SUBDEVICE_ID_CHASE_PCIRAS8          0xF010

#define PCI_VENDOR_ID_AUREAL            0x12eb
#define PCI_DEVICE_ID_AUREAL_VORTEX_1   0x0001
#define PCI_DEVICE_ID_AUREAL_VORTEX_2   0x0002
#define PCI_DEVICE_ID_AUREAL_ADVANTAGE  0x0003

#define PCI_VENDOR_ID_ELECTRONICDESIGNGMBH 0x12f8
#define PCI_DEVICE_ID_LML_33R10         0x8a02

#define PCI_VENDOR_ID_ESDGMBH           0x12fe
#define PCI_DEVICE_ID_ESDGMBH_CPCIASIO4 0x0111

#define PCI_VENDOR_ID_CB                0x1307  /* Measurement Computing */

#define PCI_VENDOR_ID_SIIG              0x131f
#define PCI_SUBVENDOR_ID_SIIG           0x131f
#define PCI_DEVICE_ID_SIIG_1S_10x_550   0x1000
#define PCI_DEVICE_ID_SIIG_1S_10x_650   0x1001
#define PCI_DEVICE_ID_SIIG_1S_10x_850   0x1002
#define PCI_DEVICE_ID_SIIG_1S1P_10x_550 0x1010
#define PCI_DEVICE_ID_SIIG_1S1P_10x_650 0x1011
#define PCI_DEVICE_ID_SIIG_1S1P_10x_850 0x1012
#define PCI_DEVICE_ID_SIIG_1P_10x       0x1020
#define PCI_DEVICE_ID_SIIG_2P_10x       0x1021
#define PCI_DEVICE_ID_SIIG_2S_10x_550   0x1030
#define PCI_DEVICE_ID_SIIG_2S_10x_650   0x1031
#define PCI_DEVICE_ID_SIIG_2S_10x_850   0x1032
#define PCI_DEVICE_ID_SIIG_2S1P_10x_550 0x1034
#define PCI_DEVICE_ID_SIIG_2S1P_10x_650 0x1035
#define PCI_DEVICE_ID_SIIG_2S1P_10x_850 0x1036
#define PCI_DEVICE_ID_SIIG_4S_10x_550   0x1050
#define PCI_DEVICE_ID_SIIG_4S_10x_650   0x1051
#define PCI_DEVICE_ID_SIIG_4S_10x_850   0x1052
#define PCI_DEVICE_ID_SIIG_1S_20x_550   0x2000
#define PCI_DEVICE_ID_SIIG_1S_20x_650   0x2001
#define PCI_DEVICE_ID_SIIG_1S_20x_850   0x2002
#define PCI_DEVICE_ID_SIIG_1P_20x       0x2020
#define PCI_DEVICE_ID_SIIG_2P_20x       0x2021
#define PCI_DEVICE_ID_SIIG_2S_20x_550   0x2030
#define PCI_DEVICE_ID_SIIG_2S_20x_650   0x2031
#define PCI_DEVICE_ID_SIIG_2S_20x_850   0x2032
#define PCI_DEVICE_ID_SIIG_2P1S_20x_550 0x2040
#define PCI_DEVICE_ID_SIIG_2P1S_20x_650 0x2041
#define PCI_DEVICE_ID_SIIG_2P1S_20x_850 0x2042
#define PCI_DEVICE_ID_SIIG_1S1P_20x_550 0x2010
#define PCI_DEVICE_ID_SIIG_1S1P_20x_650 0x2011
#define PCI_DEVICE_ID_SIIG_1S1P_20x_850 0x2012
#define PCI_DEVICE_ID_SIIG_4S_20x_550   0x2050
#define PCI_DEVICE_ID_SIIG_4S_20x_650   0x2051
#define PCI_DEVICE_ID_SIIG_4S_20x_850   0x2052
#define PCI_DEVICE_ID_SIIG_2S1P_20x_550 0x2060
#define PCI_DEVICE_ID_SIIG_2S1P_20x_650 0x2061
#define PCI_DEVICE_ID_SIIG_2S1P_20x_850 0x2062
#define PCI_DEVICE_ID_SIIG_8S_20x_550   0x2080
#define PCI_DEVICE_ID_SIIG_8S_20x_650   0x2081
#define PCI_DEVICE_ID_SIIG_8S_20x_850   0x2082
#define PCI_SUBDEVICE_ID_SIIG_QUARTET_SERIAL    0x2050

#define PCI_VENDOR_ID_RADISYS           0x1331

#define PCI_VENDOR_ID_MICRO_MEMORY              0x1332
#define PCI_DEVICE_ID_MICRO_MEMORY_5415CN       0x5415
#define PCI_DEVICE_ID_MICRO_MEMORY_5425CN       0x5425
#define PCI_DEVICE_ID_MICRO_MEMORY_6155         0x6155

#define PCI_VENDOR_ID_DOMEX             0x134a
#define PCI_DEVICE_ID_DOMEX_DMX3191D    0x0001

#define PCI_VENDOR_ID_INTASHIELD        0x135a
#define PCI_DEVICE_ID_INTASHIELD_IS200  0x0d80
#define PCI_DEVICE_ID_INTASHIELD_IS400  0x0dc0

#define PCI_VENDOR_ID_QUATECH           0x135C
#define PCI_DEVICE_ID_QUATECH_QSC100    0x0010
#define PCI_DEVICE_ID_QUATECH_DSC100    0x0020
#define PCI_DEVICE_ID_QUATECH_DSC200    0x0030
#define PCI_DEVICE_ID_QUATECH_QSC200    0x0040
#define PCI_DEVICE_ID_QUATECH_ESC100D   0x0050
#define PCI_DEVICE_ID_QUATECH_ESC100M   0x0060
#define PCI_DEVICE_ID_QUATECH_QSCP100   0x0120
#define PCI_DEVICE_ID_QUATECH_DSCP100   0x0130
#define PCI_DEVICE_ID_QUATECH_QSCP200   0x0140
#define PCI_DEVICE_ID_QUATECH_DSCP200   0x0150
#define PCI_DEVICE_ID_QUATECH_QSCLP100  0x0170
#define PCI_DEVICE_ID_QUATECH_DSCLP100  0x0180
#define PCI_DEVICE_ID_QUATECH_DSC100E   0x0181
#define PCI_DEVICE_ID_QUATECH_SSCLP100  0x0190
#define PCI_DEVICE_ID_QUATECH_QSCLP200  0x01A0
#define PCI_DEVICE_ID_QUATECH_DSCLP200  0x01B0
#define PCI_DEVICE_ID_QUATECH_DSC200E   0x01B1
#define PCI_DEVICE_ID_QUATECH_SSCLP200  0x01C0
#define PCI_DEVICE_ID_QUATECH_ESCLP100  0x01E0
#define PCI_DEVICE_ID_QUATECH_SPPXP_100 0x0278

#define PCI_VENDOR_ID_SEALEVEL          0x135e
#define PCI_DEVICE_ID_SEALEVEL_U530     0x7101
#define PCI_DEVICE_ID_SEALEVEL_UCOMM2   0x7201
#define PCI_DEVICE_ID_SEALEVEL_UCOMM422 0x7402
#define PCI_DEVICE_ID_SEALEVEL_UCOMM232 0x7202
#define PCI_DEVICE_ID_SEALEVEL_COMM4    0x7401
#define PCI_DEVICE_ID_SEALEVEL_COMM8    0x7801
#define PCI_DEVICE_ID_SEALEVEL_7803     0x7803
#define PCI_DEVICE_ID_SEALEVEL_UCOMM8   0x7804

#define PCI_VENDOR_ID_HYPERCOPE         0x1365
#define PCI_DEVICE_ID_HYPERCOPE_PLX     0x9050
#define PCI_SUBDEVICE_ID_HYPERCOPE_OLD_ERGO     0x0104
#define PCI_SUBDEVICE_ID_HYPERCOPE_ERGO         0x0106
#define PCI_SUBDEVICE_ID_HYPERCOPE_METRO        0x0107
#define PCI_SUBDEVICE_ID_HYPERCOPE_CHAMP2       0x0108

#define PCI_VENDOR_ID_DIGIGRAM          0x1369
#define PCI_SUBDEVICE_ID_DIGIGRAM_LX6464ES_SERIAL_SUBSYSTEM     0xc001
#define PCI_SUBDEVICE_ID_DIGIGRAM_LX6464ES_CAE_SERIAL_SUBSYSTEM 0xc002

#define PCI_VENDOR_ID_KAWASAKI          0x136b
#define PCI_DEVICE_ID_MCHIP_KL5A72002   0xff01

#define PCI_VENDOR_ID_CNET              0x1371
#define PCI_DEVICE_ID_CNET_GIGACARD     0x434e

#define PCI_VENDOR_ID_LMC               0x1376
#define PCI_DEVICE_ID_LMC_HSSI          0x0003
#define PCI_DEVICE_ID_LMC_DS3           0x0004
#define PCI_DEVICE_ID_LMC_SSI           0x0005
#define PCI_DEVICE_ID_LMC_T1            0x0006

#define PCI_VENDOR_ID_NETGEAR           0x1385
#define PCI_DEVICE_ID_NETGEAR_GA620     0x620a

#define PCI_VENDOR_ID_APPLICOM          0x1389
#define PCI_DEVICE_ID_APPLICOM_PCIGENERIC 0x0001
#define PCI_DEVICE_ID_APPLICOM_PCI2000IBS_CAN 0x0002
#define PCI_DEVICE_ID_APPLICOM_PCI2000PFB 0x0003

#define PCI_VENDOR_ID_MOXA              0x1393
#define PCI_DEVICE_ID_MOXA_RC7000       0x0001
#define PCI_DEVICE_ID_MOXA_CP102        0x1020
#define PCI_DEVICE_ID_MOXA_CP102UL      0x1021
#define PCI_DEVICE_ID_MOXA_CP102U       0x1022
#define PCI_DEVICE_ID_MOXA_C104         0x1040
#define PCI_DEVICE_ID_MOXA_CP104U       0x1041
#define PCI_DEVICE_ID_MOXA_CP104JU      0x1042
#define PCI_DEVICE_ID_MOXA_CP104EL      0x1043
#define PCI_DEVICE_ID_MOXA_CT114        0x1140
#define PCI_DEVICE_ID_MOXA_CP114        0x1141
#define PCI_DEVICE_ID_MOXA_CP118U       0x1180
#define PCI_DEVICE_ID_MOXA_CP118EL      0x1181
#define PCI_DEVICE_ID_MOXA_CP132        0x1320
#define PCI_DEVICE_ID_MOXA_CP132U       0x1321
#define PCI_DEVICE_ID_MOXA_CP134U       0x1340
#define PCI_DEVICE_ID_MOXA_C168         0x1680
#define PCI_DEVICE_ID_MOXA_CP168U       0x1681
#define PCI_DEVICE_ID_MOXA_CP168EL      0x1682
#define PCI_DEVICE_ID_MOXA_CP204J       0x2040
#define PCI_DEVICE_ID_MOXA_C218         0x2180
#define PCI_DEVICE_ID_MOXA_C320         0x3200

#define PCI_VENDOR_ID_CCD               0x1397
#define PCI_DEVICE_ID_CCD_HFC4S         0x08B4
#define PCI_SUBDEVICE_ID_CCD_PMX2S      0x1234
#define PCI_DEVICE_ID_CCD_HFC8S         0x16B8
#define PCI_DEVICE_ID_CCD_2BD0          0x2bd0
#define PCI_DEVICE_ID_CCD_HFCE1         0x30B1
#define PCI_SUBDEVICE_ID_CCD_SPD4S      0x3136
#define PCI_SUBDEVICE_ID_CCD_SPDE1      0x3137
#define PCI_DEVICE_ID_CCD_B000          0xb000
#define PCI_DEVICE_ID_CCD_B006          0xb006
#define PCI_DEVICE_ID_CCD_B007          0xb007
#define PCI_DEVICE_ID_CCD_B008          0xb008
#define PCI_DEVICE_ID_CCD_B009          0xb009
#define PCI_DEVICE_ID_CCD_B00A          0xb00a
#define PCI_DEVICE_ID_CCD_B00B          0xb00b
#define PCI_DEVICE_ID_CCD_B00C          0xb00c
#define PCI_DEVICE_ID_CCD_B100          0xb100
#define PCI_SUBDEVICE_ID_CCD_IOB4ST     0xB520
#define PCI_SUBDEVICE_ID_CCD_IOB8STR    0xB521
#define PCI_SUBDEVICE_ID_CCD_IOB8ST     0xB522
#define PCI_SUBDEVICE_ID_CCD_IOB1E1     0xB523
#define PCI_SUBDEVICE_ID_CCD_SWYX4S     0xB540
#define PCI_SUBDEVICE_ID_CCD_JH4S20     0xB550
#define PCI_SUBDEVICE_ID_CCD_IOB8ST_1   0xB552
#define PCI_SUBDEVICE_ID_CCD_JHSE1      0xB553
#define PCI_SUBDEVICE_ID_CCD_JH8S       0xB55B
#define PCI_SUBDEVICE_ID_CCD_BN4S       0xB560
#define PCI_SUBDEVICE_ID_CCD_BN8S       0xB562
#define PCI_SUBDEVICE_ID_CCD_BNE1       0xB563
#define PCI_SUBDEVICE_ID_CCD_BNE1D      0xB564
#define PCI_SUBDEVICE_ID_CCD_BNE1DP     0xB565
#define PCI_SUBDEVICE_ID_CCD_BN2S       0xB566
#define PCI_SUBDEVICE_ID_CCD_BN1SM      0xB567
#define PCI_SUBDEVICE_ID_CCD_BN4SM      0xB568
#define PCI_SUBDEVICE_ID_CCD_BN2SM      0xB569
#define PCI_SUBDEVICE_ID_CCD_BNE1M      0xB56A
#define PCI_SUBDEVICE_ID_CCD_BN8SP      0xB56B
#define PCI_SUBDEVICE_ID_CCD_HFC4S      0xB620
#define PCI_SUBDEVICE_ID_CCD_HFC8S      0xB622
#define PCI_DEVICE_ID_CCD_B700          0xb700
#define PCI_DEVICE_ID_CCD_B701          0xb701
#define PCI_SUBDEVICE_ID_CCD_HFCE1      0xC523
#define PCI_SUBDEVICE_ID_CCD_OV2S       0xE884
#define PCI_SUBDEVICE_ID_CCD_OV4S       0xE888
#define PCI_SUBDEVICE_ID_CCD_OV8S       0xE998

#define PCI_VENDOR_ID_EXAR              0x13a8
#define PCI_DEVICE_ID_EXAR_XR17C152     0x0152
#define PCI_DEVICE_ID_EXAR_XR17C154     0x0154
#define PCI_DEVICE_ID_EXAR_XR17C158     0x0158
#define PCI_DEVICE_ID_EXAR_XR17V352     0x0352
#define PCI_DEVICE_ID_EXAR_XR17V354     0x0354
#define PCI_DEVICE_ID_EXAR_XR17V358     0x0358

#define PCI_VENDOR_ID_MICROGATE         0x13c0
#define PCI_DEVICE_ID_MICROGATE_USC     0x0010
#define PCI_DEVICE_ID_MICROGATE_SCA     0x0030

#define PCI_VENDOR_ID_3WARE             0x13C1
#define PCI_DEVICE_ID_3WARE_1000        0x1000
#define PCI_DEVICE_ID_3WARE_7000        0x1001
#define PCI_DEVICE_ID_3WARE_9000        0x1002

#define PCI_VENDOR_ID_IOMEGA            0x13ca
#define PCI_DEVICE_ID_IOMEGA_BUZ        0x4231

#define PCI_VENDOR_ID_ABOCOM            0x13D1
#define PCI_DEVICE_ID_ABOCOM_2BD1       0x2BD1

#define PCI_VENDOR_ID_SUNDANCE          0x13f0

#define PCI_VENDOR_ID_CMEDIA            0x13f6
#define PCI_DEVICE_ID_CMEDIA_CM8338A    0x0100
#define PCI_DEVICE_ID_CMEDIA_CM8338B    0x0101
#define PCI_DEVICE_ID_CMEDIA_CM8738     0x0111
#define PCI_DEVICE_ID_CMEDIA_CM8738B    0x0112

#define PCI_VENDOR_ID_ADVANTECH         0x13fe

#define PCI_VENDOR_ID_MEILHAUS          0x1402

#define PCI_VENDOR_ID_LAVA              0x1407
#define PCI_DEVICE_ID_LAVA_DSERIAL      0x0100 /* 2x 16550 */
#define PCI_DEVICE_ID_LAVA_QUATRO_A     0x0101 /* 2x 16550, half of 4 port */
#define PCI_DEVICE_ID_LAVA_QUATRO_B     0x0102 /* 2x 16550, half of 4 port */
#define PCI_DEVICE_ID_LAVA_QUATTRO_A    0x0120 /* 2x 16550A, half of 4 port */
#define PCI_DEVICE_ID_LAVA_QUATTRO_B    0x0121 /* 2x 16550A, half of 4 port */
#define PCI_DEVICE_ID_LAVA_OCTO_A       0x0180 /* 4x 16550A, half of 8 port */
#define PCI_DEVICE_ID_LAVA_OCTO_B       0x0181 /* 4x 16550A, half of 8 port */
#define PCI_DEVICE_ID_LAVA_PORT_PLUS    0x0200 /* 2x 16650 */
#define PCI_DEVICE_ID_LAVA_QUAD_A       0x0201 /* 2x 16650, half of 4 port */
#define PCI_DEVICE_ID_LAVA_QUAD_B       0x0202 /* 2x 16650, half of 4 port */
#define PCI_DEVICE_ID_LAVA_SSERIAL      0x0500 /* 1x 16550 */
#define PCI_DEVICE_ID_LAVA_PORT_650     0x0600 /* 1x 16650 */
#define PCI_DEVICE_ID_LAVA_PARALLEL     0x8000
#define PCI_DEVICE_ID_LAVA_DUAL_PAR_A   0x8002 /* The Lava Dual Parallel is */
#define PCI_DEVICE_ID_LAVA_DUAL_PAR_B   0x8003 /* two PCI devices on a card */
#define PCI_DEVICE_ID_LAVA_BOCA_IOPPAR  0x8800

#define PCI_VENDOR_ID_TIMEDIA           0x1409
#define PCI_DEVICE_ID_TIMEDIA_1889      0x7168

#define PCI_VENDOR_ID_ICE               0x1412
#define PCI_DEVICE_ID_ICE_1712          0x1712
#define PCI_DEVICE_ID_VT1724            0x1724

#define PCI_VENDOR_ID_OXSEMI            0x1415
#define PCI_DEVICE_ID_OXSEMI_12PCI840   0x8403
#define PCI_DEVICE_ID_OXSEMI_PCIe840            0xC000
#define PCI_DEVICE_ID_OXSEMI_PCIe840_G          0xC004
#define PCI_DEVICE_ID_OXSEMI_PCIe952_0          0xC100
#define PCI_DEVICE_ID_OXSEMI_PCIe952_0_G        0xC104
#define PCI_DEVICE_ID_OXSEMI_PCIe952_1          0xC110
#define PCI_DEVICE_ID_OXSEMI_PCIe952_1_G        0xC114
#define PCI_DEVICE_ID_OXSEMI_PCIe952_1_U        0xC118
#define PCI_DEVICE_ID_OXSEMI_PCIe952_1_GU       0xC11C
#define PCI_DEVICE_ID_OXSEMI_16PCI954   0x9501
#define PCI_DEVICE_ID_OXSEMI_C950       0x950B
#define PCI_DEVICE_ID_OXSEMI_16PCI95N   0x9511
#define PCI_DEVICE_ID_OXSEMI_16PCI954PP 0x9513
#define PCI_DEVICE_ID_OXSEMI_16PCI952   0x9521
#define PCI_DEVICE_ID_OXSEMI_16PCI952PP 0x9523
#define PCI_SUBDEVICE_ID_OXSEMI_C950    0x0001

#define PCI_VENDOR_ID_CHELSIO           0x1425

#define PCI_VENDOR_ID_ADLINK            0x144a

#define PCI_VENDOR_ID_SAMSUNG           0x144d

#define PCI_VENDOR_ID_GIGABYTE          0x1458

#define PCI_VENDOR_ID_AMBIT             0x1468

#define PCI_VENDOR_ID_MYRICOM           0x14c1

#define PCI_VENDOR_ID_TITAN             0x14D2
#define PCI_DEVICE_ID_TITAN_010L        0x8001
#define PCI_DEVICE_ID_TITAN_100L        0x8010
#define PCI_DEVICE_ID_TITAN_110L        0x8011
#define PCI_DEVICE_ID_TITAN_200L        0x8020
#define PCI_DEVICE_ID_TITAN_210L        0x8021
#define PCI_DEVICE_ID_TITAN_400L        0x8040
#define PCI_DEVICE_ID_TITAN_800L        0x8080
#define PCI_DEVICE_ID_TITAN_100         0xA001
#define PCI_DEVICE_ID_TITAN_200         0xA005
#define PCI_DEVICE_ID_TITAN_400         0xA003
#define PCI_DEVICE_ID_TITAN_800B        0xA004

#define PCI_VENDOR_ID_PANACOM           0x14d4
#define PCI_DEVICE_ID_PANACOM_QUADMODEM 0x0400
#define PCI_DEVICE_ID_PANACOM_DUALMODEM 0x0402

#define PCI_VENDOR_ID_SIPACKETS         0x14d9
#define PCI_DEVICE_ID_SP1011            0x0010

#define PCI_VENDOR_ID_AFAVLAB           0x14db
#define PCI_DEVICE_ID_AFAVLAB_P028      0x2180
#define PCI_DEVICE_ID_AFAVLAB_P030      0x2182
#define PCI_SUBDEVICE_ID_AFAVLAB_P061           0x2150

#define PCI_VENDOR_ID_AMPLICON          0x14dc

#define PCI_VENDOR_ID_BCM_GVC          0x14a4
#define PCI_VENDOR_ID_BROADCOM          0x14e4
#define PCI_DEVICE_ID_TIGON3_5752       0x1600
#define PCI_DEVICE_ID_TIGON3_5752M      0x1601
#define PCI_DEVICE_ID_NX2_5709          0x1639
#define PCI_DEVICE_ID_NX2_5709S         0x163a
#define PCI_DEVICE_ID_TIGON3_5700       0x1644
#define PCI_DEVICE_ID_TIGON3_5701       0x1645
#define PCI_DEVICE_ID_TIGON3_5702       0x1646
#define PCI_DEVICE_ID_TIGON3_5703       0x1647
#define PCI_DEVICE_ID_TIGON3_5704       0x1648
#define PCI_DEVICE_ID_TIGON3_5704S_2    0x1649
#define PCI_DEVICE_ID_NX2_5706          0x164a
#define PCI_DEVICE_ID_NX2_5708          0x164c
#define PCI_DEVICE_ID_TIGON3_5702FE     0x164d
#define PCI_DEVICE_ID_NX2_57710         0x164e
#define PCI_DEVICE_ID_NX2_57711         0x164f
#define PCI_DEVICE_ID_NX2_57711E        0x1650
#define PCI_DEVICE_ID_TIGON3_5705       0x1653
#define PCI_DEVICE_ID_TIGON3_5705_2     0x1654
#define PCI_DEVICE_ID_TIGON3_5719       0x1657
#define PCI_DEVICE_ID_TIGON3_5721       0x1659
#define PCI_DEVICE_ID_TIGON3_5722       0x165a
#define PCI_DEVICE_ID_TIGON3_5723       0x165b
#define PCI_DEVICE_ID_TIGON3_5705M      0x165d
#define PCI_DEVICE_ID_TIGON3_5705M_2    0x165e
#define PCI_DEVICE_ID_NX2_57712         0x1662
#define PCI_DEVICE_ID_NX2_57712E        0x1663
#define PCI_DEVICE_ID_NX2_57712_MF      0x1663
#define PCI_DEVICE_ID_TIGON3_5714       0x1668
#define PCI_DEVICE_ID_TIGON3_5714S      0x1669
#define PCI_DEVICE_ID_TIGON3_5780       0x166a
#define PCI_DEVICE_ID_TIGON3_5780S      0x166b
#define PCI_DEVICE_ID_TIGON3_5705F      0x166e
#define PCI_DEVICE_ID_NX2_57712_VF      0x166f
#define PCI_DEVICE_ID_TIGON3_5754M      0x1672
#define PCI_DEVICE_ID_TIGON3_5755M      0x1673
#define PCI_DEVICE_ID_TIGON3_5756       0x1674
#define PCI_DEVICE_ID_TIGON3_5750       0x1676
#define PCI_DEVICE_ID_TIGON3_5751       0x1677
#define PCI_DEVICE_ID_TIGON3_5715       0x1678
#define PCI_DEVICE_ID_TIGON3_5715S      0x1679
#define PCI_DEVICE_ID_TIGON3_5754       0x167a
#define PCI_DEVICE_ID_TIGON3_5755       0x167b
#define PCI_DEVICE_ID_TIGON3_5751M      0x167d
#define PCI_DEVICE_ID_TIGON3_5751F      0x167e
#define PCI_DEVICE_ID_TIGON3_5787F      0x167f
#define PCI_DEVICE_ID_TIGON3_5761E      0x1680
#define PCI_DEVICE_ID_TIGON3_5761       0x1681
#define PCI_DEVICE_ID_TIGON3_5764       0x1684
#define PCI_DEVICE_ID_NX2_57800         0x168a
#define PCI_DEVICE_ID_NX2_57840         0x168d
#define PCI_DEVICE_ID_NX2_57810         0x168e
#define PCI_DEVICE_ID_TIGON3_5787M      0x1693
#define PCI_DEVICE_ID_TIGON3_5782       0x1696
#define PCI_DEVICE_ID_TIGON3_5784       0x1698
#define PCI_DEVICE_ID_TIGON3_5786       0x169a
#define PCI_DEVICE_ID_TIGON3_5787       0x169b
#define PCI_DEVICE_ID_TIGON3_5788       0x169c
#define PCI_DEVICE_ID_TIGON3_5789       0x169d
#define PCI_DEVICE_ID_NX2_57840_4_10    0x16a1
#define PCI_DEVICE_ID_NX2_57840_2_20    0x16a2
#define PCI_DEVICE_ID_NX2_57840_MF      0x16a4
#define PCI_DEVICE_ID_NX2_57800_MF      0x16a5
#define PCI_DEVICE_ID_TIGON3_5702X      0x16a6
#define PCI_DEVICE_ID_TIGON3_5703X      0x16a7
#define PCI_DEVICE_ID_TIGON3_5704S      0x16a8
#define PCI_DEVICE_ID_NX2_57800_VF      0x16a9
#define PCI_DEVICE_ID_NX2_5706S         0x16aa
#define PCI_DEVICE_ID_NX2_5708S         0x16ac
#define PCI_DEVICE_ID_NX2_57840_VF      0x16ad
#define PCI_DEVICE_ID_NX2_57810_MF      0x16ae
#define PCI_DEVICE_ID_NX2_57810_VF      0x16af
#define PCI_DEVICE_ID_TIGON3_5702A3     0x16c6
#define PCI_DEVICE_ID_TIGON3_5703A3     0x16c7
#define PCI_DEVICE_ID_TIGON3_5781       0x16dd
#define PCI_DEVICE_ID_TIGON3_5753       0x16f7
#define PCI_DEVICE_ID_TIGON3_5753M      0x16fd
#define PCI_DEVICE_ID_TIGON3_5753F      0x16fe
#define PCI_DEVICE_ID_TIGON3_5901       0x170d
#define PCI_DEVICE_ID_BCM4401B1         0x170c
#define PCI_DEVICE_ID_TIGON3_5901_2     0x170e
#define PCI_DEVICE_ID_TIGON3_5906       0x1712
#define PCI_DEVICE_ID_TIGON3_5906M      0x1713
#define PCI_DEVICE_ID_BCM4401           0x4401
#define PCI_DEVICE_ID_BCM4401B0         0x4402

#define PCI_VENDOR_ID_TOPIC             0x151f
#define PCI_DEVICE_ID_TOPIC_TP560       0x0000

#define PCI_VENDOR_ID_MAINPINE          0x1522
#define PCI_DEVICE_ID_MAINPINE_PBRIDGE  0x0100
#define PCI_VENDOR_ID_ENE               0x1524
#define PCI_DEVICE_ID_ENE_CB710_FLASH   0x0510
#define PCI_DEVICE_ID_ENE_CB712_SD      0x0550
#define PCI_DEVICE_ID_ENE_CB712_SD_2    0x0551
#define PCI_DEVICE_ID_ENE_CB714_SD      0x0750
#define PCI_DEVICE_ID_ENE_CB714_SD_2    0x0751
#define PCI_DEVICE_ID_ENE_1211          0x1211
#define PCI_DEVICE_ID_ENE_1225          0x1225
#define PCI_DEVICE_ID_ENE_1410          0x1410
#define PCI_DEVICE_ID_ENE_710           0x1411
#define PCI_DEVICE_ID_ENE_712           0x1412
#define PCI_DEVICE_ID_ENE_1420          0x1420
#define PCI_DEVICE_ID_ENE_720           0x1421
#define PCI_DEVICE_ID_ENE_722           0x1422

#define PCI_SUBVENDOR_ID_PERLE          0x155f
#define PCI_SUBDEVICE_ID_PCI_RAS4       0xf001
#define PCI_SUBDEVICE_ID_PCI_RAS8       0xf010

#define PCI_VENDOR_ID_SYBA              0x1592
#define PCI_DEVICE_ID_SYBA_2P_EPP       0x0782
#define PCI_DEVICE_ID_SYBA_1P_ECP       0x0783

#define PCI_VENDOR_ID_MORETON           0x15aa
#define PCI_DEVICE_ID_RASTEL_2PORT      0x2000

#define PCI_VENDOR_ID_ZOLTRIX           0x15b0
#define PCI_DEVICE_ID_ZOLTRIX_2BD0      0x2bd0

#define PCI_VENDOR_ID_MELLANOX          0x15b3
#define PCI_DEVICE_ID_MELLANOX_TAVOR    0x5a44
#define PCI_DEVICE_ID_MELLANOX_TAVOR_BRIDGE     0x5a46
#define PCI_DEVICE_ID_MELLANOX_ARBEL_COMPAT 0x6278
#define PCI_DEVICE_ID_MELLANOX_ARBEL    0x6282
#define PCI_DEVICE_ID_MELLANOX_SINAI_OLD 0x5e8c
#define PCI_DEVICE_ID_MELLANOX_SINAI    0x6274

#define PCI_VENDOR_ID_DFI               0x15bd

#define PCI_VENDOR_ID_QUICKNET          0x15e2
#define PCI_DEVICE_ID_QUICKNET_XJ       0x0500

/*
* ADDI-DATA GmbH communication cards <info@addi-data.com>
*/
#define PCI_VENDOR_ID_ADDIDATA_OLD             0x10E8
#define PCI_VENDOR_ID_ADDIDATA                 0x15B8
#define PCI_DEVICE_ID_ADDIDATA_APCI7500        0x7000
#define PCI_DEVICE_ID_ADDIDATA_APCI7420        0x7001
#define PCI_DEVICE_ID_ADDIDATA_APCI7300        0x7002
#define PCI_DEVICE_ID_ADDIDATA_APCI7800        0x818E
#define PCI_DEVICE_ID_ADDIDATA_APCI7500_2      0x7009
#define PCI_DEVICE_ID_ADDIDATA_APCI7420_2      0x700A
#define PCI_DEVICE_ID_ADDIDATA_APCI7300_2      0x700B
#define PCI_DEVICE_ID_ADDIDATA_APCI7500_3      0x700C
#define PCI_DEVICE_ID_ADDIDATA_APCI7420_3      0x700D
#define PCI_DEVICE_ID_ADDIDATA_APCI7300_3      0x700E
#define PCI_DEVICE_ID_ADDIDATA_APCI7800_3      0x700F
#define PCI_DEVICE_ID_ADDIDATA_APCIe7300       0x7010
#define PCI_DEVICE_ID_ADDIDATA_APCIe7420       0x7011
#define PCI_DEVICE_ID_ADDIDATA_APCIe7500       0x7012
#define PCI_DEVICE_ID_ADDIDATA_APCIe7800       0x7013

#define PCI_VENDOR_ID_PDC               0x15e9

#define PCI_VENDOR_ID_FARSITE           0x1619
#define PCI_DEVICE_ID_FARSITE_T2P       0x0400
#define PCI_DEVICE_ID_FARSITE_T4P       0x0440
#define PCI_DEVICE_ID_FARSITE_T1U       0x0610
#define PCI_DEVICE_ID_FARSITE_T2U       0x0620
#define PCI_DEVICE_ID_FARSITE_T4U       0x0640
#define PCI_DEVICE_ID_FARSITE_TE1       0x1610
#define PCI_DEVICE_ID_FARSITE_TE1C      0x1612

#define PCI_VENDOR_ID_ARIMA             0x161f

#define PCI_VENDOR_ID_BROCADE           0x1657
#define PCI_DEVICE_ID_BROCADE_CT        0x0014
#define PCI_DEVICE_ID_BROCADE_FC_8G1P   0x0017
#define PCI_DEVICE_ID_BROCADE_CT_FC     0x0021

#define PCI_VENDOR_ID_SIBYTE            0x166d
#define PCI_DEVICE_ID_BCM1250_PCI       0x0001
#define PCI_DEVICE_ID_BCM1250_HT        0x0002

#define PCI_VENDOR_ID_ATHEROS           0x168c

#define PCI_VENDOR_ID_NETCELL           0x169c
#define PCI_DEVICE_ID_REVOLUTION        0x0044

#define PCI_VENDOR_ID_CENATEK           0x16CA
#define PCI_DEVICE_ID_CENATEK_IDE       0x0001

#define PCI_VENDOR_ID_VITESSE           0x1725
#define PCI_DEVICE_ID_VITESSE_VSC7174   0x7174

#define PCI_VENDOR_ID_LINKSYS           0x1737
#define PCI_DEVICE_ID_LINKSYS_EG1064    0x1064

#define PCI_VENDOR_ID_ALTIMA            0x173b
#define PCI_DEVICE_ID_ALTIMA_AC1000     0x03e8
#define PCI_DEVICE_ID_ALTIMA_AC1001     0x03e9
#define PCI_DEVICE_ID_ALTIMA_AC9100     0x03ea
#define PCI_DEVICE_ID_ALTIMA_AC1003     0x03eb

#define PCI_VENDOR_ID_BELKIN            0x1799
#define PCI_DEVICE_ID_BELKIN_F5D7010V7  0x701f

#define PCI_VENDOR_ID_RDC               0x17f3
#define PCI_DEVICE_ID_RDC_R6020         0x6020
#define PCI_DEVICE_ID_RDC_R6030         0x6030
#define PCI_DEVICE_ID_RDC_R6040         0x6040
#define PCI_DEVICE_ID_RDC_R6060         0x6060
#define PCI_DEVICE_ID_RDC_R6061         0x6061
#define PCI_DEVICE_ID_RDC_D1010         0x1010

#define PCI_VENDOR_ID_LENOVO            0x17aa

#define PCI_VENDOR_ID_ARECA             0x17d3
#define PCI_DEVICE_ID_ARECA_1110        0x1110
#define PCI_DEVICE_ID_ARECA_1120        0x1120
#define PCI_DEVICE_ID_ARECA_1130        0x1130
#define PCI_DEVICE_ID_ARECA_1160        0x1160
#define PCI_DEVICE_ID_ARECA_1170        0x1170
#define PCI_DEVICE_ID_ARECA_1200        0x1200
#define PCI_DEVICE_ID_ARECA_1201        0x1201
#define PCI_DEVICE_ID_ARECA_1202        0x1202
#define PCI_DEVICE_ID_ARECA_1210        0x1210
#define PCI_DEVICE_ID_ARECA_1220        0x1220
#define PCI_DEVICE_ID_ARECA_1230        0x1230
#define PCI_DEVICE_ID_ARECA_1260        0x1260
#define PCI_DEVICE_ID_ARECA_1270        0x1270
#define PCI_DEVICE_ID_ARECA_1280        0x1280
#define PCI_DEVICE_ID_ARECA_1380        0x1380
#define PCI_DEVICE_ID_ARECA_1381        0x1381
#define PCI_DEVICE_ID_ARECA_1680        0x1680
#define PCI_DEVICE_ID_ARECA_1681        0x1681

#define PCI_VENDOR_ID_S2IO              0x17d5
#define PCI_DEVICE_ID_S2IO_WIN          0x5731
#define PCI_DEVICE_ID_S2IO_UNI          0x5831
#define PCI_DEVICE_ID_HERC_WIN          0x5732
#define PCI_DEVICE_ID_HERC_UNI          0x5832

#define PCI_VENDOR_ID_SITECOM           0x182d
#define PCI_DEVICE_ID_SITECOM_DC105V2   0x3069

#define PCI_VENDOR_ID_TOPSPIN           0x1867

#define PCI_VENDOR_ID_COMMTECH          0x18f7

#define PCI_VENDOR_ID_SILAN             0x1904

#define PCI_VENDOR_ID_RENESAS           0x1912
#define PCI_DEVICE_ID_RENESAS_SH7781    0x0001
#define PCI_DEVICE_ID_RENESAS_SH7780    0x0002
#define PCI_DEVICE_ID_RENESAS_SH7763    0x0004
#define PCI_DEVICE_ID_RENESAS_SH7785    0x0007
#define PCI_DEVICE_ID_RENESAS_SH7786    0x0010

#define PCI_VENDOR_ID_SOLARFLARE        0x1924
#define PCI_DEVICE_ID_SOLARFLARE_SFC4000A_0     0x0703
#define PCI_DEVICE_ID_SOLARFLARE_SFC4000A_1     0x6703
#define PCI_DEVICE_ID_SOLARFLARE_SFC4000B       0x0710

#define PCI_VENDOR_ID_TDI               0x192E
#define PCI_DEVICE_ID_TDI_EHCI          0x0101

#define PCI_VENDOR_ID_FREESCALE         0x1957
#define PCI_DEVICE_ID_MPC8308           0xc006
#define PCI_DEVICE_ID_MPC8315E          0x00b4
#define PCI_DEVICE_ID_MPC8315           0x00b5
#define PCI_DEVICE_ID_MPC8314E          0x00b6
#define PCI_DEVICE_ID_MPC8314           0x00b7
#define PCI_DEVICE_ID_MPC8378E          0x00c4
#define PCI_DEVICE_ID_MPC8378           0x00c5
#define PCI_DEVICE_ID_MPC8377E          0x00c6
#define PCI_DEVICE_ID_MPC8377           0x00c7
#define PCI_DEVICE_ID_MPC8548E          0x0012
#define PCI_DEVICE_ID_MPC8548           0x0013
#define PCI_DEVICE_ID_MPC8543E          0x0014
#define PCI_DEVICE_ID_MPC8543           0x0015
#define PCI_DEVICE_ID_MPC8547E          0x0018
#define PCI_DEVICE_ID_MPC8545E          0x0019
#define PCI_DEVICE_ID_MPC8545           0x001a
#define PCI_DEVICE_ID_MPC8569E          0x0061
#define PCI_DEVICE_ID_MPC8569           0x0060
#define PCI_DEVICE_ID_MPC8568E          0x0020
#define PCI_DEVICE_ID_MPC8568           0x0021
#define PCI_DEVICE_ID_MPC8567E          0x0022
#define PCI_DEVICE_ID_MPC8567           0x0023
#define PCI_DEVICE_ID_MPC8533E          0x0030
#define PCI_DEVICE_ID_MPC8533           0x0031
#define PCI_DEVICE_ID_MPC8544E          0x0032
#define PCI_DEVICE_ID_MPC8544           0x0033
#define PCI_DEVICE_ID_MPC8572E          0x0040
#define PCI_DEVICE_ID_MPC8572           0x0041
#define PCI_DEVICE_ID_MPC8536E          0x0050
#define PCI_DEVICE_ID_MPC8536           0x0051
#define PCI_DEVICE_ID_P2020E            0x0070
#define PCI_DEVICE_ID_P2020             0x0071
#define PCI_DEVICE_ID_P2010E            0x0078
#define PCI_DEVICE_ID_P2010             0x0079
#define PCI_DEVICE_ID_P1020E            0x0100
#define PCI_DEVICE_ID_P1020             0x0101
#define PCI_DEVICE_ID_P1021E            0x0102
#define PCI_DEVICE_ID_P1021             0x0103
#define PCI_DEVICE_ID_P1011E            0x0108
#define PCI_DEVICE_ID_P1011             0x0109
#define PCI_DEVICE_ID_P1022E            0x0110
#define PCI_DEVICE_ID_P1022             0x0111
#define PCI_DEVICE_ID_P1013E            0x0118
#define PCI_DEVICE_ID_P1013             0x0119
#define PCI_DEVICE_ID_P4080E            0x0400
#define PCI_DEVICE_ID_P4080             0x0401
#define PCI_DEVICE_ID_P4040E            0x0408
#define PCI_DEVICE_ID_P4040             0x0409
#define PCI_DEVICE_ID_P2040E            0x0410
#define PCI_DEVICE_ID_P2040             0x0411
#define PCI_DEVICE_ID_P3041E            0x041E
#define PCI_DEVICE_ID_P3041             0x041F
#define PCI_DEVICE_ID_P5020E            0x0420
#define PCI_DEVICE_ID_P5020             0x0421
#define PCI_DEVICE_ID_P5010E            0x0428
#define PCI_DEVICE_ID_P5010             0x0429
#define PCI_DEVICE_ID_MPC8641           0x7010
#define PCI_DEVICE_ID_MPC8641D          0x7011
#define PCI_DEVICE_ID_MPC8610           0x7018

#define PCI_VENDOR_ID_PASEMI            0x1959

#define PCI_VENDOR_ID_ATTANSIC          0x1969
#define PCI_DEVICE_ID_ATTANSIC_L1       0x1048
#define PCI_DEVICE_ID_ATTANSIC_L2       0x2048

#define PCI_VENDOR_ID_JMICRON           0x197B
#define PCI_DEVICE_ID_JMICRON_JMB360    0x2360
#define PCI_DEVICE_ID_JMICRON_JMB361    0x2361
#define PCI_DEVICE_ID_JMICRON_JMB362    0x2362
#define PCI_DEVICE_ID_JMICRON_JMB363    0x2363
#define PCI_DEVICE_ID_JMICRON_JMB364    0x2364
#define PCI_DEVICE_ID_JMICRON_JMB365    0x2365
#define PCI_DEVICE_ID_JMICRON_JMB366    0x2366
#define PCI_DEVICE_ID_JMICRON_JMB368    0x2368
#define PCI_DEVICE_ID_JMICRON_JMB369    0x2369
#define PCI_DEVICE_ID_JMICRON_JMB38X_SD 0x2381
#define PCI_DEVICE_ID_JMICRON_JMB38X_MMC 0x2382
#define PCI_DEVICE_ID_JMICRON_JMB38X_MS 0x2383
#define PCI_DEVICE_ID_JMICRON_JMB385_MS 0x2388
#define PCI_DEVICE_ID_JMICRON_JMB388_SD 0x2391
#define PCI_DEVICE_ID_JMICRON_JMB388_ESD 0x2392
#define PCI_DEVICE_ID_JMICRON_JMB390_MS 0x2393

#define PCI_VENDOR_ID_KORENIX           0x1982
#define PCI_DEVICE_ID_KORENIX_JETCARDF0 0x1600
#define PCI_DEVICE_ID_KORENIX_JETCARDF1 0x16ff
#define PCI_DEVICE_ID_KORENIX_JETCARDF2 0x1700
#define PCI_DEVICE_ID_KORENIX_JETCARDF3 0x17ff

#define PCI_VENDOR_ID_QMI               0x1a32

#define PCI_VENDOR_ID_AZWAVE            0x1a3b

#define PCI_VENDOR_ID_ASMEDIA           0x1b21

#define PCI_VENDOR_ID_CIRCUITCO         0x1cc8
#define PCI_SUBSYSTEM_ID_CIRCUITCO_MINNOWBOARD  0x0001

#define PCI_VENDOR_ID_TEKRAM            0x1de1
#define PCI_DEVICE_ID_TEKRAM_DC290      0xdc29

#define PCI_VENDOR_ID_TEHUTI            0x1fc9
#define PCI_DEVICE_ID_TEHUTI_3009       0x3009
#define PCI_DEVICE_ID_TEHUTI_3010       0x3010
#define PCI_DEVICE_ID_TEHUTI_3014       0x3014

#define PCI_VENDOR_ID_HINT             0x3388
#define PCI_DEVICE_ID_HINT_VXPROII_IDE 0x8013

#define PCI_VENDOR_ID_3DLABS            0x3d3d
#define PCI_DEVICE_ID_3DLABS_PERMEDIA2  0x0007
#define PCI_DEVICE_ID_3DLABS_PERMEDIA2V 0x0009

#define PCI_VENDOR_ID_NETXEN            0x4040
#define PCI_DEVICE_ID_NX2031_10GXSR     0x0001
#define PCI_DEVICE_ID_NX2031_10GCX4     0x0002
#define PCI_DEVICE_ID_NX2031_4GCU       0x0003
#define PCI_DEVICE_ID_NX2031_IMEZ       0x0004
#define PCI_DEVICE_ID_NX2031_HMEZ       0x0005
#define PCI_DEVICE_ID_NX2031_XG_MGMT    0x0024
#define PCI_DEVICE_ID_NX2031_XG_MGMT2   0x0025
#define PCI_DEVICE_ID_NX3031            0x0100

#define PCI_VENDOR_ID_AKS               0x416c
#define PCI_DEVICE_ID_AKS_ALADDINCARD   0x0100

#define PCI_VENDOR_ID_ACCESSIO          0x494f
#define PCI_DEVICE_ID_ACCESSIO_WDG_CSM  0x22c0

#define PCI_VENDOR_ID_S3                0x5333
#define PCI_DEVICE_ID_S3_TRIO           0x8811
#define PCI_DEVICE_ID_S3_868            0x8880
#define PCI_DEVICE_ID_S3_968            0x88f0
#define PCI_DEVICE_ID_S3_SAVAGE4        0x8a25
#define PCI_DEVICE_ID_S3_PROSAVAGE8     0x8d04
#define PCI_DEVICE_ID_S3_SONICVIBES     0xca00

#define PCI_VENDOR_ID_DUNORD            0x5544
#define PCI_DEVICE_ID_DUNORD_I3000      0x0001

#define PCI_VENDOR_ID_DCI               0x6666
#define PCI_DEVICE_ID_DCI_PCCOM4        0x0001
#define PCI_DEVICE_ID_DCI_PCCOM8        0x0002
#define PCI_DEVICE_ID_DCI_PCCOM2        0x0004

#define PCI_VENDOR_ID_INTEL             0x8086
#define PCI_DEVICE_ID_INTEL_EESSC       0x0008
#define PCI_DEVICE_ID_INTEL_PXHD_0      0x0320
#define PCI_DEVICE_ID_INTEL_PXHD_1      0x0321
#define PCI_DEVICE_ID_INTEL_PXH_0       0x0329
#define PCI_DEVICE_ID_INTEL_PXH_1       0x032A
#define PCI_DEVICE_ID_INTEL_PXHV        0x032C
#define PCI_DEVICE_ID_INTEL_80332_0     0x0330
#define PCI_DEVICE_ID_INTEL_80332_1     0x0332
#define PCI_DEVICE_ID_INTEL_80333_0     0x0370
#define PCI_DEVICE_ID_INTEL_80333_1     0x0372
#define PCI_DEVICE_ID_INTEL_82375       0x0482
#define PCI_DEVICE_ID_INTEL_82424       0x0483
#define PCI_DEVICE_ID_INTEL_82378       0x0484
#define PCI_DEVICE_ID_INTEL_MRST_SD0    0x0807
#define PCI_DEVICE_ID_INTEL_MRST_SD1    0x0808
#define PCI_DEVICE_ID_INTEL_MFD_SD      0x0820
#define PCI_DEVICE_ID_INTEL_MFD_SDIO1   0x0821
#define PCI_DEVICE_ID_INTEL_MFD_SDIO2   0x0822
#define PCI_DEVICE_ID_INTEL_MFD_EMMC0   0x0823
#define PCI_DEVICE_ID_INTEL_MFD_EMMC1   0x0824
#define PCI_DEVICE_ID_INTEL_MRST_SD2    0x084F
#define PCI_DEVICE_ID_INTEL_I960        0x0960
#define PCI_DEVICE_ID_INTEL_I960RM      0x0962
#define PCI_DEVICE_ID_INTEL_CENTERTON_ILB       0x0c60
#define PCI_DEVICE_ID_INTEL_8257X_SOL   0x1062
#define PCI_DEVICE_ID_INTEL_82573E_SOL  0x1085
#define PCI_DEVICE_ID_INTEL_82573L_SOL  0x108F
#define PCI_DEVICE_ID_INTEL_82815_MC    0x1130
#define PCI_DEVICE_ID_INTEL_82815_CGC   0x1132
#define PCI_DEVICE_ID_INTEL_82092AA_0   0x1221
#define PCI_DEVICE_ID_INTEL_7505_0      0x2550
#define PCI_DEVICE_ID_INTEL_7205_0      0x255d
#define PCI_DEVICE_ID_INTEL_82437       0x122d
#define PCI_DEVICE_ID_INTEL_82371FB_0   0x122e
#define PCI_DEVICE_ID_INTEL_82371FB_1   0x1230
#define PCI_DEVICE_ID_INTEL_82371MX     0x1234
#define PCI_DEVICE_ID_INTEL_82441       0x1237
#define PCI_DEVICE_ID_INTEL_82380FB     0x124b
#define PCI_DEVICE_ID_INTEL_82439       0x1250
#define PCI_DEVICE_ID_INTEL_80960_RP    0x1960
#define PCI_DEVICE_ID_INTEL_82840_HB    0x1a21
#define PCI_DEVICE_ID_INTEL_82845_HB    0x1a30
#define PCI_DEVICE_ID_INTEL_IOAT        0x1a38
#define PCI_DEVICE_ID_INTEL_COUGARPOINT_LPC_MIN 0x1c41
#define PCI_DEVICE_ID_INTEL_COUGARPOINT_LPC_MAX 0x1c5f
#define PCI_DEVICE_ID_INTEL_PATSBURG_LPC_0      0x1d40
#define PCI_DEVICE_ID_INTEL_PATSBURG_LPC_1      0x1d41
#define PCI_DEVICE_ID_INTEL_PANTHERPOINT_XHCI   0x1e31
#define PCI_DEVICE_ID_INTEL_PANTHERPOINT_LPC_MIN        0x1e40
#define PCI_DEVICE_ID_INTEL_PANTHERPOINT_LPC_MAX        0x1e5f
#define PCI_DEVICE_ID_INTEL_DH89XXCC_LPC_MIN    0x2310
#define PCI_DEVICE_ID_INTEL_DH89XXCC_LPC_MAX    0x231f
#define PCI_DEVICE_ID_INTEL_82801AA_0   0x2410
#define PCI_DEVICE_ID_INTEL_82801AA_1   0x2411
#define PCI_DEVICE_ID_INTEL_82801AA_3   0x2413
#define PCI_DEVICE_ID_INTEL_82801AA_5   0x2415
#define PCI_DEVICE_ID_INTEL_82801AA_6   0x2416
#define PCI_DEVICE_ID_INTEL_82801AA_8   0x2418
#define PCI_DEVICE_ID_INTEL_82801AB_0   0x2420
#define PCI_DEVICE_ID_INTEL_82801AB_1   0x2421
#define PCI_DEVICE_ID_INTEL_82801AB_3   0x2423
#define PCI_DEVICE_ID_INTEL_82801AB_5   0x2425
#define PCI_DEVICE_ID_INTEL_82801AB_6   0x2426
#define PCI_DEVICE_ID_INTEL_82801AB_8   0x2428
#define PCI_DEVICE_ID_INTEL_82801BA_0   0x2440
#define PCI_DEVICE_ID_INTEL_82801BA_2   0x2443
#define PCI_DEVICE_ID_INTEL_82801BA_4   0x2445
#define PCI_DEVICE_ID_INTEL_82801BA_6   0x2448
#define PCI_DEVICE_ID_INTEL_82801BA_8   0x244a
#define PCI_DEVICE_ID_INTEL_82801BA_9   0x244b
#define PCI_DEVICE_ID_INTEL_82801BA_10  0x244c
#define PCI_DEVICE_ID_INTEL_82801BA_11  0x244e
#define PCI_DEVICE_ID_INTEL_82801E_0    0x2450
#define PCI_DEVICE_ID_INTEL_82801E_11   0x245b
#define PCI_DEVICE_ID_INTEL_82801CA_0   0x2480
#define PCI_DEVICE_ID_INTEL_82801CA_3   0x2483
#define PCI_DEVICE_ID_INTEL_82801CA_5   0x2485
#define PCI_DEVICE_ID_INTEL_82801CA_6   0x2486
#define PCI_DEVICE_ID_INTEL_82801CA_10  0x248a
#define PCI_DEVICE_ID_INTEL_82801CA_11  0x248b
#define PCI_DEVICE_ID_INTEL_82801CA_12  0x248c
#define PCI_DEVICE_ID_INTEL_82801DB_0   0x24c0
#define PCI_DEVICE_ID_INTEL_82801DB_1   0x24c1
#define PCI_DEVICE_ID_INTEL_82801DB_2   0x24c2
#define PCI_DEVICE_ID_INTEL_82801DB_3   0x24c3
#define PCI_DEVICE_ID_INTEL_82801DB_5   0x24c5
#define PCI_DEVICE_ID_INTEL_82801DB_6   0x24c6
#define PCI_DEVICE_ID_INTEL_82801DB_9   0x24c9
#define PCI_DEVICE_ID_INTEL_82801DB_10  0x24ca
#define PCI_DEVICE_ID_INTEL_82801DB_11  0x24cb
#define PCI_DEVICE_ID_INTEL_82801DB_12  0x24cc
#define PCI_DEVICE_ID_INTEL_82801EB_0   0x24d0
#define PCI_DEVICE_ID_INTEL_82801EB_1   0x24d1
#define PCI_DEVICE_ID_INTEL_82801EB_3   0x24d3
#define PCI_DEVICE_ID_INTEL_82801EB_5   0x24d5
#define PCI_DEVICE_ID_INTEL_82801EB_6   0x24d6
#define PCI_DEVICE_ID_INTEL_82801EB_11  0x24db
#define PCI_DEVICE_ID_INTEL_82801EB_12  0x24dc
#define PCI_DEVICE_ID_INTEL_82801EB_13  0x24dd
#define PCI_DEVICE_ID_INTEL_ESB_1       0x25a1
#define PCI_DEVICE_ID_INTEL_ESB_2       0x25a2
#define PCI_DEVICE_ID_INTEL_ESB_4       0x25a4
#define PCI_DEVICE_ID_INTEL_ESB_5       0x25a6
#define PCI_DEVICE_ID_INTEL_ESB_9       0x25ab
#define PCI_DEVICE_ID_INTEL_ESB_10      0x25ac
#define PCI_DEVICE_ID_INTEL_82820_HB    0x2500
#define PCI_DEVICE_ID_INTEL_82820_UP_HB 0x2501
#define PCI_DEVICE_ID_INTEL_82850_HB    0x2530
#define PCI_DEVICE_ID_INTEL_82860_HB    0x2531
#define PCI_DEVICE_ID_INTEL_E7501_MCH   0x254c
#define PCI_DEVICE_ID_INTEL_82845G_HB   0x2560
#define PCI_DEVICE_ID_INTEL_82845G_IG   0x2562
#define PCI_DEVICE_ID_INTEL_82865_HB    0x2570
#define PCI_DEVICE_ID_INTEL_82865_IG    0x2572
#define PCI_DEVICE_ID_INTEL_82875_HB    0x2578
#define PCI_DEVICE_ID_INTEL_82915G_HB   0x2580
#define PCI_DEVICE_ID_INTEL_82915G_IG   0x2582
#define PCI_DEVICE_ID_INTEL_82915GM_HB  0x2590
#define PCI_DEVICE_ID_INTEL_82915GM_IG  0x2592
#define PCI_DEVICE_ID_INTEL_5000_ERR    0x25F0
#define PCI_DEVICE_ID_INTEL_5000_FBD0   0x25F5
#define PCI_DEVICE_ID_INTEL_5000_FBD1   0x25F6
#define PCI_DEVICE_ID_INTEL_82945G_HB   0x2770
#define PCI_DEVICE_ID_INTEL_82945G_IG   0x2772
#define PCI_DEVICE_ID_INTEL_3000_HB     0x2778
#define PCI_DEVICE_ID_INTEL_82945GM_HB  0x27A0
#define PCI_DEVICE_ID_INTEL_82945GM_IG  0x27A2
#define PCI_DEVICE_ID_INTEL_ICH6_0      0x2640
#define PCI_DEVICE_ID_INTEL_ICH6_1      0x2641
#define PCI_DEVICE_ID_INTEL_ICH6_2      0x2642
#define PCI_DEVICE_ID_INTEL_ICH6_16     0x266a
#define PCI_DEVICE_ID_INTEL_ICH6_17     0x266d
#define PCI_DEVICE_ID_INTEL_ICH6_18     0x266e
#define PCI_DEVICE_ID_INTEL_ICH6_19     0x266f
#define PCI_DEVICE_ID_INTEL_ESB2_0      0x2670
#define PCI_DEVICE_ID_INTEL_ESB2_14     0x2698
#define PCI_DEVICE_ID_INTEL_ESB2_17     0x269b
#define PCI_DEVICE_ID_INTEL_ESB2_18     0x269e
#define PCI_DEVICE_ID_INTEL_ICH7_0      0x27b8
#define PCI_DEVICE_ID_INTEL_ICH7_1      0x27b9
#define PCI_DEVICE_ID_INTEL_ICH7_30     0x27b0
#define PCI_DEVICE_ID_INTEL_TGP_LPC     0x27bc
#define PCI_DEVICE_ID_INTEL_ICH7_31     0x27bd
#define PCI_DEVICE_ID_INTEL_ICH7_17     0x27da
#define PCI_DEVICE_ID_INTEL_ICH7_19     0x27dd
#define PCI_DEVICE_ID_INTEL_ICH7_20     0x27de
#define PCI_DEVICE_ID_INTEL_ICH7_21     0x27df
#define PCI_DEVICE_ID_INTEL_ICH8_0      0x2810
#define PCI_DEVICE_ID_INTEL_ICH8_1      0x2811
#define PCI_DEVICE_ID_INTEL_ICH8_2      0x2812
#define PCI_DEVICE_ID_INTEL_ICH8_3      0x2814
#define PCI_DEVICE_ID_INTEL_ICH8_4      0x2815
#define PCI_DEVICE_ID_INTEL_ICH8_5      0x283e
#define PCI_DEVICE_ID_INTEL_ICH8_6      0x2850
#define PCI_DEVICE_ID_INTEL_ICH9_0      0x2910
#define PCI_DEVICE_ID_INTEL_ICH9_1      0x2917
#define PCI_DEVICE_ID_INTEL_ICH9_2      0x2912
#define PCI_DEVICE_ID_INTEL_ICH9_3      0x2913
#define PCI_DEVICE_ID_INTEL_ICH9_4      0x2914
#define PCI_DEVICE_ID_INTEL_ICH9_5      0x2919
#define PCI_DEVICE_ID_INTEL_ICH9_6      0x2930
#define PCI_DEVICE_ID_INTEL_ICH9_7      0x2916
#define PCI_DEVICE_ID_INTEL_ICH9_8      0x2918
#define PCI_DEVICE_ID_INTEL_I7_MCR      0x2c18
#define PCI_DEVICE_ID_INTEL_I7_MC_TAD   0x2c19
#define PCI_DEVICE_ID_INTEL_I7_MC_RAS   0x2c1a
#define PCI_DEVICE_ID_INTEL_I7_MC_TEST  0x2c1c
#define PCI_DEVICE_ID_INTEL_I7_MC_CH0_CTRL  0x2c20
#define PCI_DEVICE_ID_INTEL_I7_MC_CH0_ADDR  0x2c21
#define PCI_DEVICE_ID_INTEL_I7_MC_CH0_RANK  0x2c22
#define PCI_DEVICE_ID_INTEL_I7_MC_CH0_TC    0x2c23
#define PCI_DEVICE_ID_INTEL_I7_MC_CH1_CTRL  0x2c28
#define PCI_DEVICE_ID_INTEL_I7_MC_CH1_ADDR  0x2c29
#define PCI_DEVICE_ID_INTEL_I7_MC_CH1_RANK  0x2c2a
#define PCI_DEVICE_ID_INTEL_I7_MC_CH1_TC    0x2c2b
#define PCI_DEVICE_ID_INTEL_I7_MC_CH2_CTRL  0x2c30
#define PCI_DEVICE_ID_INTEL_I7_MC_CH2_ADDR  0x2c31
#define PCI_DEVICE_ID_INTEL_I7_MC_CH2_RANK  0x2c32
#define PCI_DEVICE_ID_INTEL_I7_MC_CH2_TC    0x2c33
#define PCI_DEVICE_ID_INTEL_I7_NONCORE  0x2c41
#define PCI_DEVICE_ID_INTEL_I7_NONCORE_ALT 0x2c40
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_NONCORE     0x2c50
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_NONCORE_ALT 0x2c51
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_NONCORE_REV2 0x2c70
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_SAD         0x2c81
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_QPI_LINK0   0x2c90
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_QPI_PHY0    0x2c91
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MCR         0x2c98
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_TAD      0x2c99
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_TEST     0x2c9C
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH0_CTRL 0x2ca0
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH0_ADDR 0x2ca1
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH0_RANK 0x2ca2
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH0_TC   0x2ca3
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH1_CTRL 0x2ca8
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH1_ADDR 0x2ca9
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH1_RANK 0x2caa
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH1_TC   0x2cab
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MCR_REV2          0x2d98
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_TAD_REV2       0x2d99
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_RAS_REV2       0x2d9a
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_TEST_REV2      0x2d9c
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH0_CTRL_REV2  0x2da0
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH0_ADDR_REV2  0x2da1
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH0_RANK_REV2  0x2da2
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH0_TC_REV2    0x2da3
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH1_CTRL_REV2  0x2da8
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH1_ADDR_REV2  0x2da9
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH1_RANK_REV2  0x2daa
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH1_TC_REV2    0x2dab
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH2_CTRL_REV2  0x2db0
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH2_ADDR_REV2  0x2db1
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH2_RANK_REV2  0x2db2
#define PCI_DEVICE_ID_INTEL_LYNNFIELD_MC_CH2_TC_REV2    0x2db3
#define PCI_DEVICE_ID_INTEL_82855PM_HB  0x3340
#define PCI_DEVICE_ID_INTEL_IOAT_TBG4   0x3429
#define PCI_DEVICE_ID_INTEL_IOAT_TBG5   0x342a
#define PCI_DEVICE_ID_INTEL_IOAT_TBG6   0x342b
#define PCI_DEVICE_ID_INTEL_IOAT_TBG7   0x342c
#define PCI_DEVICE_ID_INTEL_X58_HUB_MGMT 0x342e
#define PCI_DEVICE_ID_INTEL_IOAT_TBG0   0x3430
#define PCI_DEVICE_ID_INTEL_IOAT_TBG1   0x3431
#define PCI_DEVICE_ID_INTEL_IOAT_TBG2   0x3432
#define PCI_DEVICE_ID_INTEL_IOAT_TBG3   0x3433
#define PCI_DEVICE_ID_INTEL_82830_HB    0x3575
#define PCI_DEVICE_ID_INTEL_82830_CGC   0x3577
#define PCI_DEVICE_ID_INTEL_82854_HB    0x358c
#define PCI_DEVICE_ID_INTEL_82854_IG    0x358e
#define PCI_DEVICE_ID_INTEL_82855GM_HB  0x3580
#define PCI_DEVICE_ID_INTEL_82855GM_IG  0x3582
#define PCI_DEVICE_ID_INTEL_E7520_MCH   0x3590
#define PCI_DEVICE_ID_INTEL_E7320_MCH   0x3592
#define PCI_DEVICE_ID_INTEL_MCH_PA      0x3595
#define PCI_DEVICE_ID_INTEL_MCH_PA1     0x3596
#define PCI_DEVICE_ID_INTEL_MCH_PB      0x3597
#define PCI_DEVICE_ID_INTEL_MCH_PB1     0x3598
#define PCI_DEVICE_ID_INTEL_MCH_PC      0x3599
#define PCI_DEVICE_ID_INTEL_MCH_PC1     0x359a
#define PCI_DEVICE_ID_INTEL_E7525_MCH   0x359e
#define PCI_DEVICE_ID_INTEL_I7300_MCH_ERR 0x360c
#define PCI_DEVICE_ID_INTEL_I7300_MCH_FB0 0x360f
#define PCI_DEVICE_ID_INTEL_I7300_MCH_FB1 0x3610
#define PCI_DEVICE_ID_INTEL_IOAT_CNB    0x360b
#define PCI_DEVICE_ID_INTEL_FBD_CNB     0x360c
#define PCI_DEVICE_ID_INTEL_IOAT_JSF0   0x3710
#define PCI_DEVICE_ID_INTEL_IOAT_JSF1   0x3711
#define PCI_DEVICE_ID_INTEL_IOAT_JSF2   0x3712
#define PCI_DEVICE_ID_INTEL_IOAT_JSF3   0x3713
#define PCI_DEVICE_ID_INTEL_IOAT_JSF4   0x3714
#define PCI_DEVICE_ID_INTEL_IOAT_JSF5   0x3715
#define PCI_DEVICE_ID_INTEL_IOAT_JSF6   0x3716
#define PCI_DEVICE_ID_INTEL_IOAT_JSF7   0x3717
#define PCI_DEVICE_ID_INTEL_IOAT_JSF8   0x3718
#define PCI_DEVICE_ID_INTEL_IOAT_JSF9   0x3719
#define PCI_DEVICE_ID_INTEL_ICH10_0     0x3a14
#define PCI_DEVICE_ID_INTEL_ICH10_1     0x3a16
#define PCI_DEVICE_ID_INTEL_ICH10_2     0x3a18
#define PCI_DEVICE_ID_INTEL_ICH10_3     0x3a1a
#define PCI_DEVICE_ID_INTEL_ICH10_4     0x3a30
#define PCI_DEVICE_ID_INTEL_ICH10_5     0x3a60
#define PCI_DEVICE_ID_INTEL_5_3400_SERIES_LPC_MIN       0x3b00
#define PCI_DEVICE_ID_INTEL_5_3400_SERIES_LPC_MAX       0x3b1f
#define PCI_DEVICE_ID_INTEL_IOAT_SNB0   0x3c20
#define PCI_DEVICE_ID_INTEL_IOAT_SNB1   0x3c21
#define PCI_DEVICE_ID_INTEL_IOAT_SNB2   0x3c22
#define PCI_DEVICE_ID_INTEL_IOAT_SNB3   0x3c23
#define PCI_DEVICE_ID_INTEL_IOAT_SNB4   0x3c24
#define PCI_DEVICE_ID_INTEL_IOAT_SNB5   0x3c25
#define PCI_DEVICE_ID_INTEL_IOAT_SNB6   0x3c26
#define PCI_DEVICE_ID_INTEL_IOAT_SNB7   0x3c27
#define PCI_DEVICE_ID_INTEL_IOAT_SNB8   0x3c2e
#define PCI_DEVICE_ID_INTEL_IOAT_SNB9   0x3c2f
#define PCI_DEVICE_ID_INTEL_UNC_HA      0x3c46
#define PCI_DEVICE_ID_INTEL_UNC_IMC0    0x3cb0
#define PCI_DEVICE_ID_INTEL_UNC_IMC1    0x3cb1
#define PCI_DEVICE_ID_INTEL_UNC_IMC2    0x3cb4
#define PCI_DEVICE_ID_INTEL_UNC_IMC3    0x3cb5
#define PCI_DEVICE_ID_INTEL_UNC_QPI0    0x3c41
#define PCI_DEVICE_ID_INTEL_UNC_QPI1    0x3c42
#define PCI_DEVICE_ID_INTEL_UNC_R2PCIE  0x3c43
#define PCI_DEVICE_ID_INTEL_UNC_R3QPI0  0x3c44
#define PCI_DEVICE_ID_INTEL_UNC_R3QPI1  0x3c45
#define PCI_DEVICE_ID_INTEL_JAKETOWN_UBOX       0x3ce0
#define PCI_DEVICE_ID_INTEL_IOAT_SNB    0x402f
#define PCI_DEVICE_ID_INTEL_5100_16     0x65f0
#define PCI_DEVICE_ID_INTEL_5100_19     0x65f3
#define PCI_DEVICE_ID_INTEL_5100_21     0x65f5
#define PCI_DEVICE_ID_INTEL_5100_22     0x65f6
#define PCI_DEVICE_ID_INTEL_5400_ERR    0x4030
#define PCI_DEVICE_ID_INTEL_5400_FBD0   0x4035
#define PCI_DEVICE_ID_INTEL_5400_FBD1   0x4036
#define PCI_DEVICE_ID_INTEL_IOAT_SCNB   0x65ff
#define PCI_DEVICE_ID_INTEL_EP80579_0   0x5031
#define PCI_DEVICE_ID_INTEL_EP80579_1   0x5032
#define PCI_DEVICE_ID_INTEL_82371SB_0   0x7000
#define PCI_DEVICE_ID_INTEL_82371SB_1   0x7010
#define PCI_DEVICE_ID_INTEL_82371SB_2   0x7020
#define PCI_DEVICE_ID_INTEL_82437VX     0x7030
#define PCI_DEVICE_ID_INTEL_82439TX     0x7100
#define PCI_DEVICE_ID_INTEL_82371AB_0   0x7110
#define PCI_DEVICE_ID_INTEL_82371AB     0x7111
#define PCI_DEVICE_ID_INTEL_82371AB_2   0x7112
#define PCI_DEVICE_ID_INTEL_82371AB_3   0x7113
#define PCI_DEVICE_ID_INTEL_82810_MC1   0x7120
#define PCI_DEVICE_ID_INTEL_82810_IG1   0x7121
#define PCI_DEVICE_ID_INTEL_82810_MC3   0x7122
#define PCI_DEVICE_ID_INTEL_82810_IG3   0x7123
#define PCI_DEVICE_ID_INTEL_82810E_MC   0x7124
#define PCI_DEVICE_ID_INTEL_82810E_IG   0x7125
#define PCI_DEVICE_ID_INTEL_82443LX_0   0x7180
#define PCI_DEVICE_ID_INTEL_82443LX_1   0x7181
#define PCI_DEVICE_ID_INTEL_82443BX_0   0x7190
#define PCI_DEVICE_ID_INTEL_82443BX_1   0x7191
#define PCI_DEVICE_ID_INTEL_82443BX_2   0x7192
#define PCI_DEVICE_ID_INTEL_440MX       0x7195
#define PCI_DEVICE_ID_INTEL_440MX_6     0x7196
#define PCI_DEVICE_ID_INTEL_82443MX_0   0x7198
#define PCI_DEVICE_ID_INTEL_82443MX_1   0x7199
#define PCI_DEVICE_ID_INTEL_82443MX_3   0x719b
#define PCI_DEVICE_ID_INTEL_82443GX_0   0x71a0
#define PCI_DEVICE_ID_INTEL_82443GX_2   0x71a2
#define PCI_DEVICE_ID_INTEL_82372FB_1   0x7601
#define PCI_DEVICE_ID_INTEL_SCH_LPC     0x8119
#define PCI_DEVICE_ID_INTEL_SCH_IDE     0x811a
#define PCI_DEVICE_ID_INTEL_ITC_LPC     0x8186
#define PCI_DEVICE_ID_INTEL_82454GX     0x84c4
#define PCI_DEVICE_ID_INTEL_82450GX     0x84c5
#define PCI_DEVICE_ID_INTEL_82451NX     0x84ca
#define PCI_DEVICE_ID_INTEL_82454NX     0x84cb
#define PCI_DEVICE_ID_INTEL_84460GX     0x84ea
#define PCI_DEVICE_ID_INTEL_IXP4XX      0x8500
#define PCI_DEVICE_ID_INTEL_IXP2800     0x9004
#define PCI_DEVICE_ID_INTEL_S21152BB    0xb152

#define PCI_VENDOR_ID_SCALEMP           0x8686
#define PCI_DEVICE_ID_SCALEMP_VSMP_CTL  0x1010

#define PCI_VENDOR_ID_COMPUTONE         0x8e0e
#define PCI_DEVICE_ID_COMPUTONE_IP2EX   0x0291
#define PCI_DEVICE_ID_COMPUTONE_PG      0x0302
#define PCI_SUBVENDOR_ID_COMPUTONE      0x8e0e
#define PCI_SUBDEVICE_ID_COMPUTONE_PG4  0x0001
#define PCI_SUBDEVICE_ID_COMPUTONE_PG8  0x0002
#define PCI_SUBDEVICE_ID_COMPUTONE_PG6  0x0003

#define PCI_VENDOR_ID_KTI               0x8e2e

#define PCI_VENDOR_ID_ADAPTEC           0x9004
#define PCI_DEVICE_ID_ADAPTEC_7810      0x1078
#define PCI_DEVICE_ID_ADAPTEC_7821      0x2178
#define PCI_DEVICE_ID_ADAPTEC_38602     0x3860
#define PCI_DEVICE_ID_ADAPTEC_7850      0x5078
#define PCI_DEVICE_ID_ADAPTEC_7855      0x5578
#define PCI_DEVICE_ID_ADAPTEC_3860      0x6038
#define PCI_DEVICE_ID_ADAPTEC_1480A     0x6075
#define PCI_DEVICE_ID_ADAPTEC_7860      0x6078
#define PCI_DEVICE_ID_ADAPTEC_7861      0x6178
#define PCI_DEVICE_ID_ADAPTEC_7870      0x7078
#define PCI_DEVICE_ID_ADAPTEC_7871      0x7178
#define PCI_DEVICE_ID_ADAPTEC_7872      0x7278
#define PCI_DEVICE_ID_ADAPTEC_7873      0x7378
#define PCI_DEVICE_ID_ADAPTEC_7874      0x7478
#define PCI_DEVICE_ID_ADAPTEC_7895      0x7895
#define PCI_DEVICE_ID_ADAPTEC_7880      0x8078
#define PCI_DEVICE_ID_ADAPTEC_7881      0x8178
#define PCI_DEVICE_ID_ADAPTEC_7882      0x8278
#define PCI_DEVICE_ID_ADAPTEC_7883      0x8378
#define PCI_DEVICE_ID_ADAPTEC_7884      0x8478
#define PCI_DEVICE_ID_ADAPTEC_7885      0x8578
#define PCI_DEVICE_ID_ADAPTEC_7886      0x8678
#define PCI_DEVICE_ID_ADAPTEC_7887      0x8778
#define PCI_DEVICE_ID_ADAPTEC_7888      0x8878

#define PCI_VENDOR_ID_ADAPTEC2          0x9005
#define PCI_DEVICE_ID_ADAPTEC2_2940U2   0x0010
#define PCI_DEVICE_ID_ADAPTEC2_2930U2   0x0011
#define PCI_DEVICE_ID_ADAPTEC2_7890B    0x0013
#define PCI_DEVICE_ID_ADAPTEC2_7890     0x001f
#define PCI_DEVICE_ID_ADAPTEC2_3940U2   0x0050
#define PCI_DEVICE_ID_ADAPTEC2_3950U2D  0x0051
#define PCI_DEVICE_ID_ADAPTEC2_7896     0x005f
#define PCI_DEVICE_ID_ADAPTEC2_7892A    0x0080
#define PCI_DEVICE_ID_ADAPTEC2_7892B    0x0081
#define PCI_DEVICE_ID_ADAPTEC2_7892D    0x0083
#define PCI_DEVICE_ID_ADAPTEC2_7892P    0x008f
#define PCI_DEVICE_ID_ADAPTEC2_7899A    0x00c0
#define PCI_DEVICE_ID_ADAPTEC2_7899B    0x00c1
#define PCI_DEVICE_ID_ADAPTEC2_7899D    0x00c3
#define PCI_DEVICE_ID_ADAPTEC2_7899P    0x00cf
#define PCI_DEVICE_ID_ADAPTEC2_OBSIDIAN   0x0500
#define PCI_DEVICE_ID_ADAPTEC2_SCAMP    0x0503

#define PCI_VENDOR_ID_HOLTEK            0x9412
#define PCI_DEVICE_ID_HOLTEK_6565       0x6565

#define PCI_VENDOR_ID_NETMOS            0x9710
#define PCI_DEVICE_ID_NETMOS_9705       0x9705
#define PCI_DEVICE_ID_NETMOS_9715       0x9715
#define PCI_DEVICE_ID_NETMOS_9735       0x9735
#define PCI_DEVICE_ID_NETMOS_9745       0x9745
#define PCI_DEVICE_ID_NETMOS_9755       0x9755
#define PCI_DEVICE_ID_NETMOS_9805       0x9805
#define PCI_DEVICE_ID_NETMOS_9815       0x9815
#define PCI_DEVICE_ID_NETMOS_9835       0x9835
#define PCI_DEVICE_ID_NETMOS_9845       0x9845
#define PCI_DEVICE_ID_NETMOS_9855       0x9855
#define PCI_DEVICE_ID_NETMOS_9865       0x9865
#define PCI_DEVICE_ID_NETMOS_9900       0x9900
#define PCI_DEVICE_ID_NETMOS_9901       0x9901
#define PCI_DEVICE_ID_NETMOS_9904       0x9904
#define PCI_DEVICE_ID_NETMOS_9912       0x9912
#define PCI_DEVICE_ID_NETMOS_9922       0x9922

#define PCI_VENDOR_ID_3COM_2            0xa727

#define PCI_VENDOR_ID_DIGIUM            0xd161
#define PCI_DEVICE_ID_DIGIUM_HFC4S      0xb410

#define PCI_SUBVENDOR_ID_EXSYS          0xd84d
#define PCI_SUBDEVICE_ID_EXSYS_4014     0x4014
#define PCI_SUBDEVICE_ID_EXSYS_4055     0x4055

#define PCI_VENDOR_ID_TIGERJET          0xe159
#define PCI_DEVICE_ID_TIGERJET_300      0x0001
#define PCI_DEVICE_ID_TIGERJET_100      0x0002

#define PCI_VENDOR_ID_XILINX_RME        0xea60
#define PCI_DEVICE_ID_RME_DIGI32        0x9896
#define PCI_DEVICE_ID_RME_DIGI32_PRO    0x9897
#define PCI_DEVICE_ID_RME_DIGI32_8      0x9898

#define PCI_VENDOR_ID_XEN               0x5853
#define PCI_DEVICE_ID_XEN_PLATFORM      0x0001

#define PCI_VENDOR_ID_OCZ               0x1b85

//--------------------------------------------------------------------------------------------------

#define PCI_VENDOR_ID_VMWARE            0x15ad
#define PCI_DEVICE_ID_VMWARE_PCI_BRIDGE 0x0790
#define PCI_DEVICE_ID_VMWARE_PCI_EXPRES 0x07a0
#define PCI_DEVICE_ID_VMWARE_PCI_TO_USB 0x0770
#define PCI_DEVICE_ID_VMWARE_USB_TO_PLL 0x0778
#define PCI_DEVICE_ID_VMWARE_VHDA_AUDIO   0x1977
#define PCI_DEVICE_ID_VMWARE_VHDA_CODEC   0x1975
#define PCI_DEVICE_ID_VMWARE_VMCI_BUS   0x0740
#define PCI_DEVICE_ID_VMWARE_VMEMCTRL   0x0801
#define PCI_DEVICE_ID_VMWARE_VMXNET     0x0720
#define PCI_DEVICE_ID_VMWARE_VMXNET3    0x07b0
#define PCI_DEVICE_ID_VMWARE_VSVGA      0x0710
#define PCI_DEVICE_ID_VMWARE_VSVGA2     0x0405

#define PCI_VENDOR_ID_INVALID   0xffff

char *libpci_vendorID_str(int vid);
char *libpci_deviceID_str(int vid, int did);
