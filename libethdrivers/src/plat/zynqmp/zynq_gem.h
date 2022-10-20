/*
 * (C) Copyright 2011 Michal Simek
 * Copyright 2017, DornerWorks, Ltd.
 *
 * Ported to seL4
 *
 * Michal SIMEK <monstr@monstr.eu>
 *
 * Based on Xilinx gmac driver:
 * (C) Copyright 2011 Xilinx
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <platsupport/io.h>

/* Bit/mask specification */
#define ZYNQ_GEM_PHYMNTNC_OP_MASK   0x40020000 /* operation mask bits */
#define ZYNQ_GEM_PHYMNTNC_OP_R_MASK 0x20000000 /* read operation */
#define ZYNQ_GEM_PHYMNTNC_OP_W_MASK 0x10000000 /* write operation */
#define ZYNQ_GEM_PHYMNTNC_PHYAD_SHIFT_MASK  23 /* Shift bits for PHYAD */
#define ZYNQ_GEM_PHYMNTNC_PHREG_SHIFT_MASK  18 /* Shift bits for PHREG */

#define ZYNQ_GEM_RXBUF_EOF_MASK     0x00008000 /* End of frame. */
#define ZYNQ_GEM_RXBUF_SOF_MASK     0x00004000 /* Start of frame. */
#define ZYNQ_GEM_RXBUF_LEN_MASK     0x00003FFF /* Mask for length field */

#define ZYNQ_GEM_RXBUF_WRAP_MASK    0x00000002 /* Wrap bit, last BD */
#define ZYNQ_GEM_RXBUF_NEW_MASK     0x00000001 /* Used bit.. */
#define ZYNQ_GEM_RXBUF_ADD_MASK     0xFFFFFFFC /* Mask for address */

/* Wrap bit, last descriptor */
#define ZYNQ_GEM_TXBUF_WRAP_MASK    0x40000000
#define ZYNQ_GEM_TXBUF_LAST_MASK    0x00008000 /* Last buffer */
#define ZYNQ_GEM_TXBUF_USED_MASK    0x80000000 /* Used by Hw */

#define ZYNQ_GEM_NWCTRL_TXEN_MASK   0x00000008 /* Enable transmit */
#define ZYNQ_GEM_NWCTRL_RXEN_MASK   0x00000004 /* Enable receive */
#define ZYNQ_GEM_NWCTRL_MDEN_MASK   0x00000010 /* Enable MDIO port */
#define ZYNQ_GEM_NWCTRL_STARTTX_MASK    0x00000200 /* Start tx (tx_go) */

#define ZYNQ_GEM_NWCFG_SPEED100     0x000000001 /* 100 Mbps operation */
#define ZYNQ_GEM_NWCFG_SPEED1000    0x000000400 /* 1Gbps operation */
#define ZYNQ_GEM_NWCFG_FDEN     0x000000002 /* Full Duplex mode */
#define ZYNQ_GEM_NWCFG_FSREM        0x000020000 /* FCS removal */
#define ZYNQ_GEM_NWCFG_MDCCLKDIV    0x0000c0000 /* Div pclk by 48, max 120MHz */
#define ZYNQ_GEM_NWCFG_COPY_ALL     0x000000010 /* Promiscuous Mode */

#ifdef CONFIG_ARM64
# define ZYNQ_GEM_DBUS_WIDTH    (1 << 21) /* 64 bit bus */
#else
# define ZYNQ_GEM_DBUS_WIDTH    (0 << 21) /* 32 bit bus */
#endif

#define ZYNQ_GEM_NWCFG_INIT     (ZYNQ_GEM_DBUS_WIDTH | \
                    ZYNQ_GEM_NWCFG_FDEN | \
                    ZYNQ_GEM_NWCFG_FSREM | \
                    ZYNQ_GEM_NWCFG_MDCCLKDIV)

#define ZYNQ_GEM_NWSR_MDIOIDLE_MASK 0x00000004 /* PHY management idle */

#define ZYNQ_GEM_DMACR_BLENGTH      0x00000004 /* INCR4 AHB bursts */
/* Use full configured addressable space (8 Kb) */
#define ZYNQ_GEM_DMACR_RXSIZE       0x00000300
/* Use full configured addressable space (4 Kb) */
#define ZYNQ_GEM_DMACR_TXSIZE       0x00000400
/* Set with binary 00011000 to use 1536 byte(1*max length frame/buffer) */
#define ZYNQ_GEM_DMACR_RXBUF        0x00180000

#define ZYNQ_GEM_DMACR_INIT     (ZYNQ_GEM_DMACR_BLENGTH | \
                    ZYNQ_GEM_DMACR_RXSIZE | \
                    ZYNQ_GEM_DMACR_TXSIZE | \
                    ZYNQ_GEM_DMACR_RXBUF)

#define ZYNQ_GEM_TSR_DONE       0x00000020 /* Tx done mask */

/* Use MII register 1 (MII status register) to detect PHY */
#define PHY_DETECT_REG  1

/* Mask used to verify certain PHY features (or register contents)
 * in the register above:
 *  0x1000: 10Mbps full duplex support
 *  0x0800: 10Mbps half duplex support
 *  0x0008: Auto-negotiation support
 */
#define PHY_DETECT_MASK 0x1808

/* TX BD status masks */
#define ZYNQ_GEM_TXBUF_FRMLEN_MASK  0x000007ff
#define ZYNQ_GEM_TXBUF_EXHAUSTED    0x08000000
#define ZYNQ_GEM_TXBUF_UNDERRUN     0x10000000

/* Clock frequencies for different speeds */
#define ZYNQ_GEM_FREQUENCY_10   2500000UL
#define ZYNQ_GEM_FREQUENCY_100  25000000UL
#define ZYNQ_GEM_FREQUENCY_1000 125000000UL

#define ZYNQ_GEM_IXR_FRAMERX (1 << 1)
#define ZYNQ_GEM_IXR_TXCOMPLETE (1 << 7)

#define ZYNQ_GEM_TXSR_TXGO (1 << 3)

/* Device registers */
struct zynq_gem_regs {
    u32 nwctrl; /* 0x0 - Network Control reg */
    u32 nwcfg; /* 0x4 - Network Config reg */
    u32 nwsr; /* 0x8 - Network Status reg */
    u32 reserved1;
    u32 dmacr; /* 0x10 - DMA Control reg */
    u32 txsr; /* 0x14 - TX Status reg */
    u32 rxqbase; /* 0x18 - RX Q Base address reg */
    u32 txqbase; /* 0x1c - TX Q Base address reg */
    u32 rxsr; /* 0x20 - RX Status reg */
    u32 isr;
    u32 ier;
    u32 idr; /* 0x2c - Interrupt Disable reg */
    u32 reserved3;
    u32 phymntnc; /* 0x34 - Phy Maintaince reg */
    u32 reserved4[18];
    u32 hashl; /* 0x80 - Hash Low address reg */
    u32 hashh; /* 0x84 - Hash High address reg */
#define LADDR_LOW   0
#define LADDR_HIGH  1
    u32 laddr[4][LADDR_HIGH + 1]; /* 0x8c - Specific1 addr low/high reg */
    u32 match[4]; /* 0xa8 - Type ID1 Match reg */
    u32 reserved6[18];
#define STAT_SIZE   44
    u32 stat[STAT_SIZE]; /* 0x100 - Octects transmitted Low reg */
    u32 reserved7[164];
    u32 transmit_q1_ptr; /* 0x440 - Transmit priority queue 1 */
    u32 reserved8[15];
    u32 receive_q1_ptr; /* 0x480 - Receive priority queue 1 */
};

/* BD descriptors */
struct emac_bd {
    u32 addr; /* Next descriptor pointer */
    u32 status;
};

#define RX_BUF 32
/* Page table entries are set to 1MB, or multiples of 1MB
 * (not < 1MB). driver uses less bd's so use 1MB bdspace.
 */
#define BD_SPACE    0x100000
/* BD separation space */
#define BD_SEPRN_SPACE  (RX_BUF * sizeof(struct emac_bd))

/* Setup the first free TX descriptor */
#define TX_FREE_DESC    2

struct eth_device *zynq_gem_initialize(phys_addr_t base_addr,
                                       int phy_addr, u32 emio);
int zynq_gem_init(struct eth_device *dev);
int zynq_gem_setup_mac(struct eth_device *dev);
int zynq_gem_start_send(struct eth_device *dev);
int zynq_gem_recv_enabled(struct eth_device *dev);
void zynq_gem_recv_enable(struct eth_device *dev);
void zynq_gem_halt(struct eth_device *dev);
void zynq_set_gem_ioops(ps_io_ops_t *io_ops);
void zynq_gem_prom_enable(struct eth_device *dev);
void zynq_gem_prom_disable(struct eth_device *dev);
