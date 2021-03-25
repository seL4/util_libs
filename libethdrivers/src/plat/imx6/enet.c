/*
 * Copyright 2017, NXP
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "enet.h"
#include <stdint.h>
#include <assert.h>
#include "io.h"
#include <platsupport/clock.h>
#include "unimplemented.h"
#include "../../debug.h"
#include <stdlib.h>

#ifdef CONFIG_PLAT_IMX6
#define IMX6_ENET_PADDR 0x02188000
#define IMX6_ENET_SIZE  0x00004000
#endif
#ifdef CONFIG_PLAT_IMX8MQ_EVK
#define IMX6_ENET_PADDR 0x30be0000
#define IMX6_ENET_SIZE  0x10000

#define CCM_PADDR 0x30380000
#define CCM_SIZE 0x10000
#endif

#define ENET_FREQ  125000000UL
#define MDC_FREQ    20000000UL /* must be less than 2.5MHz */

struct mib_regs {
    /* NOTE: Counter not implemented because it is not applicable (read 0 always).*/
    uint32_t rmon_t_drop;        /* 00 Register Count of frames not counted correctly */
    uint32_t rmon_t_packets;     /* 04 RMON Tx packet count */
    uint32_t rmon_t_bc_pkt;      /* 08 RMON Tx Broadcast Packets */
    uint32_t rmon_t_mc_pkt;      /* 0C RMON Tx Multicast Packets */
    uint32_t rmon_t_crc_align;   /* 10 RMON Tx Packets w CRC/Align error */
    uint32_t rmon_t_undersize;   /* 14 RMON Tx Packets < 64 bytes, good CRC */
    uint32_t rmon_t_oversize;    /* 18 RMON Tx Packets > MAX_FL bytes, good CRC */
    uint32_t rmon_t_frag;        /* 1C RMON Tx Packets < 64 bytes, bad CRC */
    uint32_t rmon_t_jab;         /* 20 RMON Tx Packets > MAX_FL bytes, bad CRC*/
    uint32_t rmon_t_col;         /* 24 RMON Tx collision count */
    uint32_t rmon_t_p64;         /* 28 RMON Tx 64 byte packets */
    uint32_t rmon_t_p65to127n;   /* 2C RMON Tx 65 to 127 byte packets */
    uint32_t rmon_t_p128to255n;  /* 30 RMON Tx 128 to 255 byte packets */
    uint32_t rmon_t_p256to511;   /* 34 RMON Tx 256 to 511 byte packets */
    uint32_t rmon_t_p512to1023;  /* 38 RMON Tx 512 to 1023 byte packets */
    uint32_t rmon_t_p1024to2047; /* 3C RMON Tx 1024 to 2047 byte packets */
    uint32_t rmon_t_p_gte2048;   /* 40 RMON Tx packets w > 2048 bytes */
    uint32_t rmon_t_octets;      /* 44 RMON Tx Octets */
    /* NOTE: Counter not implemented because it is not applicable (read 0 always). */
    uint32_t ieee_t_drop;        /* 48 Count of frames not counted correctly */
    uint32_t ieee_t_frame_ok;    /* 4C Frames Transmitted OK */
    uint32_t ieee_t_1col;        /* 50 Frames Transmitted with Single Collision */
    uint32_t ieee_t_mcol;        /* 54 Frames Transmitted with Multiple Collisions */
    uint32_t ieee_t_def;         /* 58 Frames Transmitted after Deferral Delay */
    uint32_t ieee_t_lcol;        /* 5C Frames Transmitted with Late Collision */
    uint32_t ieee_t_excol;       /* 60 Frames Transmitted with Excessive Collisions */
    uint32_t ieee_t_macerr;      /* 64 Frames Transmitted with Tx FIFO Underrun */
    uint32_t ieee_t_cserr;       /* 68 Frames Transmitted with Carrier Sense Error */
    /* NOTE: Counter not implemented because there is no SQE information available (read 0 always). */
    uint32_t ieee_t_sqe;         /* 6C Frames Transmitted with SQE Error */
    uint32_t ieee_t_fdxfc;       /* 70 Flow Control Pause frames transmitted */
    /* NOTE: Counts total octets (includes header and FCS fields). */
    uint32_t ieee_t_octets_ok;   /* 74 Octet count for Frames Transmitted w/o Error */
    uint32_t res0[3];
    uint32_t rmon_r_packets;     /* 84 RMON Rx packet count */
    uint32_t rmon_r_bc_pkt;      /* 88 RMON Rx Broadcast Packets */
    uint32_t rmon_r_mc_pkt;      /* 8C RMON Rx Multicast Packets */
    uint32_t rmon_r_crc_align;   /* 90 RMON Rx Packets w CRC/Align error */
    uint32_t rmon_r_undersize;   /* 94 RMON Rx Packets < 64 bytes, good CRC */
    uint32_t rmon_r_oversize;    /* 98 RMON Rx Packets > MAX_FL, good CRC */
    uint32_t rmon_r_frag;        /* 9C RMON Rx Packets < 64 bytes, bad CRC */
    uint32_t rmon_r_jab;         /* A0 RMON Rx Packets > MAX_FL bytes, bad CRC  */
    uint32_t rmon_r_resvd_0;     /* A4 Reserved */
    uint32_t rmon_r_p64;         /* A8 RMON Rx 64 byte packets */
    uint32_t rmon_r_p65to127;    /* AC RMON Rx 65 to 127 byte packets */
    uint32_t rmon_r_p128to255;   /* B0 RMON Rx 128 to 255 byte packets */
    uint32_t rmon_r_p256to511;   /* B4 RMON Rx 256 to 511 byte packets */
    uint32_t rmon_r_p512to1023;  /* B8 RMON Rx 512 to 1023 byte packets */
    uint32_t rmon_r_p1024to2047; /* BC RMON Rx 1024 to 2047 byte packets */
    uint32_t rmon_r_p_gte2048;   /* C0 RMON Rx packets w > 2048 bytes */
    uint32_t rmon_r_octets;      /* C4 RMON Rx Octets */
    /* NOTE: Counter increments if a frame with invalid/missing SFD character is
     * detected and has been dropped. None of the other counters increments if
     * this counter increments. */
    uint32_t ieee_r_drop;        /* C8 Count of frames not counted correctly */
    uint32_t ieee_r_frame_ok;    /* CC Frames Received OK */
    uint32_t ieee_r_crc;         /* D0 Frames Received with CRC Error */
    uint32_t ieee_r_align;       /* D4 Frames Received with Alignment Error */
    /* Assume they mean D8... */
    uint32_t ieee_r_macerr;      /* D7 Receive FIFO Overflow count */
    uint32_t ieee_r_fdxfc;       /* DC Flow Control Pause frames received */
    /* NOTE: Counts total octets (includes header and FCS fields ) */
    uint32_t ieee_r_octets_ok;   /* E0 Octet count for Frames Rcvd w/o Error */
    uint32_t res1[7];
};

struct enet_regs {
    /* Configuration */
    uint32_t res0[1];
    uint32_t eir;    /* 004 Interrupt Event Register */
    uint32_t eimr;   /* 008 Interrupt Mask Register */
    uint32_t res1[1];
    uint32_t rdar;   /* 010 Receive Descriptor Active Register */
    uint32_t tdar;   /* 014 Transmit Descriptor Active Register */
    uint32_t res2[3];
    uint32_t ecr;    /* 024 Ethernet Control Register */
    uint32_t res3[6];
    uint32_t mmfr;   /* 040 MII Management Frame Register */
    uint32_t mscr;   /* 044 MII Speed Control Register */
    uint32_t res4[7];
    uint32_t mibc;   /* 064 MIB Control Register */
    uint32_t res5[7];
    uint32_t rcr;    /* 084 Receive Control Register */
    uint32_t res6[15];
    uint32_t tcr;    /* 0C4 Transmit Control Register */
    uint32_t res7[7];
    uint32_t palr;   /* 0E4 Physical Address Lower Register */
    uint32_t paur;   /* 0E8 Physical Address Upper Register */
    uint32_t opd;    /* 0EC Opcode/Pause Duration Register */
    uint32_t res8[10];
    uint32_t iaur;   /* 118 Descriptor Individual Upper Address Register */
    uint32_t ialr;   /* 11C Descriptor Individual Lower Address Register */
    uint32_t gaur;   /* 120 Descriptor Group Upper Address Register */
    uint32_t galr;   /* 124 Descriptor Group Lower Address Register */
    uint32_t res9[7];
    uint32_t tfwr;   /* 144 Transmit FIFO Watermark Register */
    uint32_t res10[14];
    uint32_t rdsr;   /* 180 Receive Descriptor Ring Start Register */
    uint32_t tdsr;   /* 184 Transmit Buffer Descriptor Ring Start Register */
    uint32_t mrbr;   /* 188 Maximum Receive Buffer Size Register */
    uint32_t res12[1];
    uint32_t rsfl;   /* 190 Receive FIFO Section Full Threshold */
    uint32_t rsem;   /* 194 Receive FIFO Section Empty Threshold */
    uint32_t raem;   /* 198 Receive FIFO Almost Empty Threshold */
    uint32_t rafl;   /* 19C Receive FIFO Almost Full Threshold */
    uint32_t tsem;   /* 1A0 Transmit FIFO Section Empty Threshold */
    uint32_t taem;   /* 1A4 Transmit FIFO Almost Empty Threshold */
    uint32_t tafl;   /* 1A8 Transmit FIFO Almost Full Threshold */
    uint32_t tipg;   /* 1AC Transmit Inter-Packet Gap */
    uint32_t ftrl;   /* 1B0 Frame Truncation Length */
    uint32_t res13[3];
    uint32_t tacc;   /* 1C0 Transmit Accelerator Function Configuration */
    uint32_t racc;   /* 1C4 Receive Accelerator Function Configuration */
    uint32_t res14[14];
    /* 0x200: Statistics counters MIB block RFC 2819 */
    struct mib_regs mib;
    uint32_t res15[64];
    /* 0x400: 1588 adjustable timer (TSM) and 1588 frame control */
    uint32_t atcr;   /* 400 Timer Control Register */
    uint32_t atvr;   /* 404 Timer Value Register */
    uint32_t atoff;  /* 408 Timer Offset Register */
    uint32_t atper;  /* 40C Timer Period Register */
    uint32_t atcor;  /* 410 Timer Correction Register */
    uint32_t atinc;  /* 414 Time-Stamping Clock Period Register */
    uint32_t atstmp; /* 418 Timestamp of Last Transmitted Frame */
    uint32_t res16[121];

    /* 0x600: Capture/compare block */
    uint32_t res17[1];
    uint32_t tgsr;   /* 604 Timer Global Status Register */
    uint32_t tcsr0;  /* 608 Timer Control Status Register */
    uint32_t tccr0;  /* 60C Timer Compare Capture Register */
    uint32_t tcsr1;  /* 610 Timer Control Status Register */
    uint32_t tccr1;  /* 614 Timer Compare Capture Register */
    uint32_t tcsr2;  /* 618 Timer Control Status Register */
    uint32_t tccr2;  /* 61C Timer Compare Capture Register */
    uint32_t tcsr3;  /* 620 Timer Control Status Register */
    uint32_t tccr3;  /* 624 Timer Compare Capture Register */
};

struct enet {
    void *dummy;
};

typedef volatile struct enet_regs enet_regs_t;

static inline enet_regs_t *enet_get_regs(struct enet *enet)
{
    return (enet_regs_t *)enet;
}

/* Ethernet control register */
#define ECR_DBSWP   BIT(8) /* descriptor byte swapping enable */
#define ECR_STOPEN  BIT(7) /* Disable ENET clock in doze mode */
#define ECR_DBGEN   BIT(6) /* Harware freeze when in debug mode */
#define ECR_SPEED   BIT(5) /* Enable 1000Mbps */
#define ECR_EN1588  BIT(4) /* Enhanced descriptors */
#define ECR_SLEEP   BIT(3) /* Enter sleep mode */
#define ECR_MAGICEN BIT(2) /* Magic packet detection enable */
#define ECR_ETHEREN BIT(1) /* Enable */
#define ECR_RESET   BIT(0) /* Reset */

/* Receive control register */
#define RCR_GRS       BIT(31) /* Graceful Receive Stopped */
#define RCR_NLC       BIT(30) /* No payload Length Check */
#define RCR_MAX_FL(x) (((x) & 0x3fff) << 16) /* Maximum Frame Length */
#define RCR_CFEN      BIT(15) /* MAC Control Frame Enable */
#define RCR_CRCSTRIP  BIT(14) /* Do not Forward Received CRC */
#define RCR_PAUFWD    BIT(13) /* Forward Pause Frames */
#define RCR_PADEN     BIT(12) /* Frame padding enable */
#define RCR_RMII_10T  BIT( 9) /* 10 Mbps */
#define RCR_RMII_MODE BIT( 8) /* RMII Mode Enable */
#define RCR_RGMII_EN  BIT( 6) /* RGMII  Mode Enable. RMII must not be set */
#define RCR_FCE       BIT( 5) /* Flow control enable */
#define RCR_BC_REJ    BIT( 4) /* Broadcast frame reject */
#define RCR_PROM      BIT( 3) /* Promiscuous mode */
#define RCR_MII_MODE  BIT( 2) /* This field must always be set */
#define RCR_DRT       BIT( 1) /* Don't receive while transmitting (half duplex) */
#define RCR_LOOP      BIT( 0) /* internal loop back */

/* Transmit control register */
#define TCR_CRCINS    BIT( 9) /* Insert CRC on transit */
#define TCR_ADDINS    BIT( 8) /* Insert MAC address on transit */
#define TCR_ADDSEL(x) (((x) & 0x7) << 5) /* MAC select (supports only 0) */
#define TCR_RFCPAUSE  BIT(4) /* Received a pause frame */
#define TCR_TFCPAUSE  BIT(3) /* Transmit pause frame after the current frame completes */
#define TCR_FDEN      BIT(2) /* Full duplex enable */
#define TCR_GTS       BIT(0) /* Graceful TX stop */

/* Receive Accelerator Function Configuration */
#define RACC_LINEDIS  BIT(6) /* Discard frames with MAC layer errors */

/* Transmit FIFO watermark */
#define TFWR_STRFWD   BIT( 8) /* Enables store and forward */

/* MIB control */
#define MIBC_DIS      BIT(31) /* Disable MIB counters */
#define MIBC_IDLE     BIT(30) /* MIB currently updating a counter */
#define MIBC_CLEAR    BIT(29) /* Clear all counters */

/* RX descriptor active */
#define RDAR_RDAR     BIT(24) /* RX descriptor active */

/* TX descriptor active */
#define TDAR_TDAR     BIT(24) /* TX descriptor active */

/* TODO this should be defined elsewhere */
#define FRAME_LEN 1518

#define PAUSE_FRAME_TYPE_FIELD 0x8808 /* fixed magic */
#define PAUSE_OPCODE_FIELD     0x0001 /* Fixed magic opcode used when sending pause frames */
/* configurable */
#define PAUSE_DURATION             32 /* Pause duration field when sending pause frames */
#define STRFWD_BYTES              128 /* Number of bytes in buffer before transmission begins */

#define TIPG                       8 /* TX inter-packet gap between 8 and 27 bytes */

#define PHYOP_VALID      (BIT(30) | BIT(17))
#define PHYOP_READ        BIT(29)
#define PHYOP_WRITE       BIT(28)
#define PHYOP_PHY_SHIFT       23
#define PHYOP_REG_SHIFT       18
#define PHYOP_DATA_SHIFT       0

/******************
 *** MDIO clock ***
 ******************/

static freq_t _mdc_clk_get_freq(clk_t *clk)
{
    enet_regs_t *regs = (enet_regs_t *)clk->priv;
    uint32_t fin = clk_get_freq(clk->parent);
    uint32_t v = (regs->mscr >> 1) & 0x3f;
    uint32_t fout = fin / ((v + 1) * 2);
    return fout;
}

static freq_t _mdc_clk_set_freq(clk_t *clk, freq_t hz)
{
    enet_regs_t *regs = (enet_regs_t *)clk->priv;
    uint32_t fin = clk_get_freq(clk->parent);
    uint32_t v;

    if (hz > 2500000UL) {
        hz = 2500000UL;
    } else if (hz == 0) {
        hz = 1;
    }

    v = fin / (2 * hz) - 1;

    if (v == -1) {
        v = 0;
    } else if (v > 0x3f) {
        v = 0x3f;
    }

    regs->mscr = v << 1;
    CLK_DEBUG(printf("Set MDC frequency to %.1f Mhz (<= 2.5 Mhz)\n",
                     (float)clk_get_freq(clk) / MHZ));
    return clk_get_freq(clk);
}

static void _mdc_clk_recal(struct clock *clk)
{
    assert(0);
}

static clk_t *_mdc_clk_init(clk_t *clk)
{
    return clk;
}

static struct clock mdc_clk = {
    .id = CLK_CUSTOM,
    .name = "mdc_clk",
    .priv = NULL,
    .req_freq = 2500000UL,
    .set_freq = &_mdc_clk_set_freq,
    .get_freq = &_mdc_clk_get_freq,
    .recal = &_mdc_clk_recal,
    .init = &_mdc_clk_init,
    .parent = NULL,
    .sibling = NULL,
    .child = NULL,
};

#ifdef CONFIG_PLAT_IMX8MQ_EVK
static freq_t _enet_clk_get_freq(clk_t *clk)
{
    return clk->req_freq;
}

static struct clock enet_clk = {
    .id = CLK_CUSTOM,
    .name = "enet_clk",
    .priv = NULL,
    .req_freq = 125000000UL,
    .set_freq = NULL,
    .get_freq = &_enet_clk_get_freq,
    .recal = NULL,
    .init = NULL,
    .parent = NULL,
    .sibling = NULL,
    .child = NULL,
};
#endif

void enet_set_speed(struct enet *enet, int speed, int full_duplex)
{
    enet_regs_t *regs = enet_get_regs(enet);
    uint32_t ecr = regs->ecr;
    uint32_t rcr = regs->rcr;
    /* RGMII mode */
    rcr &= ~RCR_RMII_MODE;
    rcr |= RCR_RGMII_EN | RCR_MII_MODE;
    /* Now select speed */
    switch (speed) {
    case 1000:
        ecr |= ECR_SPEED;
        rcr &= ~RCR_RMII_10T;
        break;
    case 100:
        ecr &= ~ECR_SPEED;
        rcr &= ~RCR_RMII_10T;
        break;
    case 10:
        ecr &= ~ECR_SPEED;
        rcr |= RCR_RMII_10T;
        break;
    default:
        printf("Invalid speed\n");
        assert(0);
        return;
    }
    /* Now select duplex */
    if (full_duplex) {
        rcr &= ~RCR_DRT;
    } else {
        rcr |= RCR_DRT;
    }
    /* Write the registers */
    regs->ecr = ecr;
    regs->rcr = rcr;
}

/****************
 *** MDIO bus ***
 ****************/

int enet_mdio_read(struct enet *enet, uint16_t phy, uint16_t reg)
{
    enet_regs_t *regs = enet_get_regs(enet);
    uint32_t v;
    assert(!(phy & ~0x1f));
    assert(!(reg & ~0x1f));
    assert(regs->mscr);
    assert(!enet_clr_events(enet, NETIRQ_MII));
    v  = phy << PHYOP_PHY_SHIFT | reg << PHYOP_REG_SHIFT;
    v |= PHYOP_READ | PHYOP_VALID;
    writel(v, &regs->mmfr);
    while (!enet_clr_events(enet, NETIRQ_MII)) {
        dsb();
    }
    return readl(&regs->mmfr) & 0xffff;
}

int enet_mdio_write(struct enet *enet, uint16_t phy, uint16_t reg, uint16_t data)
{
    enet_regs_t *regs = enet_get_regs(enet);
    uint32_t v;
    assert(!(phy & ~0x1f));
    assert(!(reg & ~0x1f));
    assert(regs->mscr);
    assert(!enet_clr_events(enet, NETIRQ_MII));
    v  = phy << PHYOP_PHY_SHIFT | reg << PHYOP_REG_SHIFT | data;
    v |= PHYOP_WRITE | PHYOP_VALID;
    regs->mmfr = v;
    while (!enet_clr_events(enet, NETIRQ_MII));
    return 0;
}

/*******************
 *** ENET driver ***
 *******************/
void enet_rx_enable(struct enet *enet)
{
    enet_get_regs(enet)->rdar = RDAR_RDAR;
}

int enet_rx_enabled(struct enet *enet)
{
    return enet_get_regs(enet)->rdar == RDAR_RDAR;
}

int enet_tx_enabled(struct enet *enet)
{
    return enet_get_regs(enet)->tdar == TDAR_TDAR;
}

void enet_tx_enable(struct enet *enet)
{
    enet_get_regs(enet)->tdar = TDAR_TDAR;
}

void enet_enable(struct enet *enet)
{
    enet_regs_t *regs = enet_get_regs(enet);
    regs->ecr |= ECR_ETHEREN;
}

int enet_enabled(struct enet *enet)
{
    enet_regs_t *regs = enet_get_regs(enet);
    return (regs->ecr & ECR_ETHEREN) != 0;
}

void enet_disable(struct enet *enet)
{
    enet_regs_t *regs = enet_get_regs(enet);
    assert(!"WARNING Descriptors will be reset");
    regs->ecr &= ~ECR_ETHEREN;
}

void enet_set_mac(struct enet *enet, unsigned char *mac)
{
    enet_regs_t *regs = enet_get_regs(enet);
    regs->palr = mac[0] << 24 | mac[1] << 16 | mac[2] << 8 | mac[3] << 0;
    regs->paur = mac[4] << 24 | mac[5] << 16 | PAUSE_FRAME_TYPE_FIELD;
}

void enet_get_mac(struct enet *enet, unsigned char *mac)
{
    enet_regs_t *regs = enet_get_regs(enet);
    uint32_t macl = regs->palr;
    uint32_t macu = regs->paur;

    /* set MAC hardware address */
    mac[0] = macl >> 24;
    mac[1] = macl >> 16;
    mac[2] = macl >>  8;
    mac[3] = macl >>  0;
    mac[4] = macu >> 24;
    mac[5] = macu >> 16;
}

void enet_enable_events(struct enet *enet, uint32_t mask)
{
    assert(enet);
    enet_get_regs(enet)->eimr = mask;
}

uint32_t enet_get_events(struct enet *enet)
{
    return enet_get_regs(enet)->eir;
}

uint32_t enet_clr_events(struct enet *enet, uint32_t bits)
{
    enet_regs_t *regs = enet_get_regs(enet);
    uint32_t e = regs->eir & bits;
    /* write 1 to clear */
    regs->eir = e;
    return e;
}

void enet_prom_enable(struct enet *enet)
{
    enet_regs_t *regs = enet_get_regs(enet);
    regs->rcr |= RCR_PROM;
}

void enet_prom_disable(struct enet *enet)
{
    enet_regs_t *regs = enet_get_regs(enet);
    regs->rcr &= ~RCR_PROM;
}

struct enet *
enet_init(struct desc_data desc_data, ps_io_ops_t *io_ops)
{
    enet_regs_t *regs;
    struct enet *ret;
    struct clock *enet_clk_ptr = NULL;

    /* Map in the device */
    regs = RESOURCE(&io_ops->io_mapper, IMX6_ENET);
    if (regs == NULL) {
        return NULL;
    }
    ret = (struct enet *)regs;
    /* Perform reset */
    regs->ecr = ECR_RESET;
    while (regs->ecr & ECR_RESET);
    regs->ecr |= ECR_DBSWP;

    /* Clear and mask interrupts */
    regs->eimr = 0x00000000;
    regs->eir  = 0xffffffff;

#ifdef CONFIG_PLAT_IMX6
    /* Set the ethernet clock frequency */
    clock_sys_t *clk_sys = malloc(sizeof(clock_sys_t));
    clock_sys_init(io_ops, clk_sys);
    enet_clk_ptr = clk_get_clock(clk_sys, CLK_ENET);
    clk_set_freq(enet_clk_ptr, ENET_FREQ);
#endif
#ifdef CONFIG_PLAT_IMX8MQ_EVK
    enet_clk_ptr = &enet_clk;
    // TODO Implement an actual clock driver for the imx8mq
    void *clock_base = RESOURCE(&io_ops->io_mapper, CCM);
    if (!clock_base) {
        return NULL;
    }

    uint32_t *ccgr_enet_set = clock_base + 0x40a0;
    uint32_t *ccgr_enet_clr = clock_base + 0x40a4;
    uint32_t *ccgr_sim_enet_set = clock_base + 0x4400;
    uint32_t *ccgr_sim_enet_clr = clock_base + 0x4404;

    /* Gate the clocks first */
    *ccgr_enet_clr = 0x3;
    *ccgr_sim_enet_clr = 0x3;

    /* Setup the clocks to have the proper sources/configs */
    uint32_t *enet_axi_target = clock_base + 0x8880;
    uint32_t *enet_ref_target = clock_base + 0xa980;
    uint32_t *enet_timer_target = clock_base + 0xaa00;

    *enet_axi_target = BIT(28) | 0x01000000; // ENABLE | MUX SYS1_PLL | POST AND PRE DIVIDE BY 1
    *enet_ref_target = BIT(28) | 0x01000000; // ENABLE | MUX PLL2_DIV8 | POST AND PRE DIVIDE BY 1
    *enet_timer_target = BIT(28) | 0x01000000 | ((4) & 0x3f); // ENABLE | MUX PLL2_DIV10 | POST DIVIDE BY 4, PRE DIVIDE BY 1

    /* Ungate the clocks now */
    *ccgr_enet_set = 0x3;
    *ccgr_sim_enet_set = 0x3;
#endif

    /* Set the MDIO clock frequency */
    mdc_clk.priv = (void *)enet_get_regs(ret);
    clk_register_child(enet_clk_ptr, &mdc_clk);
    clk_set_freq(&mdc_clk, MDC_FREQ);

    /* Clear out MIB */
    enet_clear_mib(ret);

    /* Descriptor group and individual hash tables - Not changed on reset */
    regs->iaur = 0;
    regs->ialr = 0;
    regs->gaur = 0;
    regs->galr = 0;

    /* Set MAC and pause frame type field */
    enet_set_mac(ret, (unsigned char *)"\0\0\0\0\0\0");

    /* Configure pause frames (continues into MAC registers...) */
    regs->opd = PAUSE_OPCODE_FIELD << 16;
#ifdef PAUSE_DURATION
    if (PAUSE_DURATION >= 0) {
        regs->opd |= PAUSE_DURATION << 0;
    }
#endif

    /* TX inter-packet gap */
    regs->tipg = TIPG;
    /* Tranmsmit FIFO Watermark register - store and forward */
    regs->tfwr = 0;
#ifdef STRFWD_BYTES
    if (STRFWD_BYTES > 0) {
        regs->tfwr = STRFWD_BYTES / 64;
        regs->tfwr |= TFWR_STRFWD;
    }
#endif

    /* Do not forward frames with errors */
    regs->racc = RACC_LINEDIS;

    /* DMA descriptors */
    regs->tdsr = desc_data.tx_phys;
    regs->rdsr = desc_data.rx_phys;
    regs->mrbr = desc_data.rx_bufsize;

    /* Receive control - Set frame length and RGMII mode */
    regs->rcr = RCR_MAX_FL(FRAME_LEN) | RCR_RGMII_EN | RCR_MII_MODE;
    /* Transmit control - Full duplex mode */
    regs->tcr = TCR_FDEN;

    return ret;
}

/****************************
 *** Debug and statistics ***
 ****************************/

static void dump_regs(uint32_t *start, int size)
{
    int i, j;
    uint32_t *base = start;
    for (i = 0; i < size / sizeof(*start);) {
        printf("+0x%03x: ", ((uint32_t)(start - base)) * 4);
        for (j = 0; j < 4; j++, i++, start++) {
            printf("0x%08x ", *start);
        }
        printf("\n");
    }
}

void enet_dump_regs(struct enet *enet)
{
    enet_regs_t *regs = enet_get_regs(enet);
    printf("\nEthernet regs\n");
    dump_regs((uint32_t *)regs, sizeof(*regs));
    printf("\n");
}

void enet_clear_mib(struct enet *enet)
{
    enet_regs_t *regs = enet_get_regs(enet);
    /* Disable */
    regs->mibc |= MIBC_DIS;
    while (!(regs->mibc & MIBC_IDLE));
    /* Clear */
    regs->mibc |= MIBC_CLEAR;
    while (!(regs->mibc & MIBC_IDLE));
    /* Restart */
    regs->mibc &= ~MIBC_CLEAR;
    regs->mibc &= ~MIBC_DIS;
}

void enet_print_mib(struct enet *enet)
{
    enet_regs_t *regs = enet_get_regs(enet);
    volatile struct mib_regs *mib = &regs->mib;
    regs->mibc |= MIBC_DIS;

    printf("Ethernet Counter regs\n");
    dump_regs((uint32_t *)mib, sizeof(*mib));
    printf("\n");

    printf("-----------------------------\n");
    printf("RX  Frames RX OK: %d/%d\n", mib->ieee_r_frame_ok,
           mib->rmon_r_packets);
    printf("RX FIFO overflow: %d\n", mib->ieee_r_macerr);
    printf("RX  pause frames: %d\n", mib->ieee_r_fdxfc);
    printf("-----------------------------\n");
    printf("TX  Frames TX OK: %d/%d\n", mib->ieee_t_frame_ok,
           mib->rmon_t_packets);
    printf("TX FIFO underrun: %d\n", mib->ieee_t_macerr);
    printf("TX  pause frames: %d\n", mib->ieee_t_fdxfc);
    printf("-----------------------------\n");
    printf("\n");
    regs->mibc &= ~MIBC_DIS;
}

void enet_print_state(struct enet *enet)
{
    enet_regs_t *regs = enet_get_regs(enet);
    printf("Ethernet state: %s\n", (enet_enabled(enet)) ? "Active" : "Inactive");
    printf("      TX state: %s\n", (enet_tx_enabled(enet)) ? "Active" : "Inactive");
    printf("      RX state: %s\n", (enet_rx_enabled(enet)) ? "Active" : "Inactive");
    printf("    TX control: 0x%08x\n", regs->tcr);
    printf("    RX control: 0x%08x\n", regs->rcr);
    printf("  RX desc base: 0x%08x (size: 0x%x)\n", regs->rdsr, regs->mrbr);
    printf("  TX desc base: 0x%08x\n", regs->tdsr);
    printf("Enabled events: 0x%08x\n", regs->eimr);
    printf("Pending events: 0x%08x\n", regs->eir);
    printf("         Speed: ????\n");
    printf("\n");
}
