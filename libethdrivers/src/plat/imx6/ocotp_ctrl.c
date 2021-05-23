/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2020, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/* Author Alex Kroh */
#include "ocotp_ctrl.h"
#include <assert.h>
#include <stdio.h>
#include <utils/util.h>
#include "unimplemented.h"

/*
 * NOTE: ocotp clock is sources by ipg_clk but must be gated
 * CCM_CGR2[CG6]
 */

#if defined(CONFIG_PLAT_IMX6)

#define IMX6_OCOTP_PADDR   0x021BC000
#define IMX6_OCOTP_SIZE    0x00004000

#elif defined(CONFIG_PLAT_IMX8MQ_EVK)

#define IMX6_OCOTP_PADDR   0x30350000
#define IMX6_OCOTP_SIZE    0x00010000

#endif


#define TIMING_WAIT(x)       ((x) << 22)
#define TIMING_SREAD(x)      ((x) << 16)
#define TIMING_RELAX(x)      ((x) << 12)
#define TIMING_SPROG(x)      ((x) <<  0)
#define TIMING_WAIT_MAX      0x03f
#define TIMING_SREAD_MAX     0x03f
#define TIMING_RELAX_MAX     0x00f
#define TIMING_SPROG_MAX     0xfff

#define CTRL_UNLOCK_KEY      (0x3E77 << 16)
#define CTRL_RELOAD_SHADOWS  BIT(10)
#define CTRL_ERROR           BIT(9)
#define CTRL_BUSY            BIT(8)
#define CTRL_ADDR(x)         ((x) << 0)

#define LOCK_FUSE            BIT(0)
#define LOCK_SHADOW          BIT(1)
#define LOCK_ANALOG(x)       ((x) << 18)
#define LOCK_GP2(x)          ((x) << 12)
#define LOCK_CP1(x)          ((x) << 10)
#define LOCK_MAC(x)          ((x) <<  8)
#define LOCK_MEMTRIM(x)      ((x) <<  4)
#define LOCK_BOOTCFG(x)      ((x) <<  2)
#define LOCK_TESTER(x)       ((x) <<  0)

#define FADDR_LOCK 0x00
#define FADDR_MAC0 0x22
#define FADDR_MAC1 0x23

struct ocotp_regs {
    uint32_t ctrl;            /* 000 */
    uint32_t ctrl_set;        /* 004 */
    uint32_t ctrl_clr;        /* 008 */
    uint32_t ctrl_tog;        /* 00C */
    uint32_t timing;          /* 010 */
    uint32_t res1[3];         /* 014 */
    uint32_t data;            /* 020 */
    uint32_t res2[3];         /* 024 */
    uint32_t read_ctrl;       /* 030 */
    uint32_t res3[3];         /* 034 */
    uint32_t read_fuse_data;  /* 040 */
    uint32_t res4[3];         /* 044 */
    uint32_t sw_sticky;       /* 050 */
    uint32_t res5[3];         /* 054 */
    uint32_t scs;             /* 060 */
    uint32_t scs_set;         /* 064 */
    uint32_t scs_clr;         /* 068 */
    uint32_t scs_tog;         /* 06C */
    uint32_t res6[8];         /* 070 */
    uint32_t ver;             /* 090 */
    uint32_t res7[219];       /* 094 */
    /* Bank 0 */
    uint32_t lock;            /* 400 */
    uint32_t res8[3];
    uint32_t cfg0;            /* 410 */
    uint32_t res9[3];
    uint32_t cfg1;            /* 420 */
    uint32_t res10[3];
    uint32_t cfg2;            /* 430 */
    uint32_t res11[3];
    uint32_t cfg3;            /* 440 */
    uint32_t res12[3];
    uint32_t cfg4;            /* 450 */
    uint32_t res13[3];
    uint32_t cfg5;            /* 460 */
    uint32_t res14[3];
    uint32_t cfg6;            /* 470 */
    uint32_t res15[3];
    /* Bank 1 */
    uint32_t mem0;            /* 480 */
    uint32_t res16[3];
    uint32_t mem1;            /* 490 */
    uint32_t res17[3];
    uint32_t mem2;            /* 4A0 */
    uint32_t res18[3];
    uint32_t mem3;            /* 4B0 */
    uint32_t res19[3];
    uint32_t mem4;            /* 4C0 */
    uint32_t res20[3];
    uint32_t ana0;            /* 4D0 */
    uint32_t res21[3];
    uint32_t ana1;            /* 4E0 */
    uint32_t res22[3];
    uint32_t ana2;            /* 4F0 */
    uint32_t res23[3];
    uint32_t res24[32];       /* 500 */
    /* Bank 3 shadow */
    uint32_t srk0;            /* 580 */
    uint32_t res25[3];
    uint32_t srk1;            /* 590 */
    uint32_t res26[3];
    uint32_t srk2;            /* 5A0 */
    uint32_t res27[3];
    uint32_t srk3;            /* 5B0 */
    uint32_t res28[3];
    uint32_t srk4;            /* 5C0 */
    uint32_t res29[3];
    uint32_t srk5;            /* 5D0 */
    uint32_t res30[3];
    uint32_t srk6;            /* 5E0 */
    uint32_t res31[3];
    uint32_t srk7;            /* 5F0 */
    uint32_t res32[3];
    /* Bank 4 */
    uint32_t resp0;           /* 600 */
    uint32_t res33[3];
    uint32_t hsjc_resp1;      /* 610 */
    uint32_t res34[3];
    uint32_t mac0;            /* 620 */
    uint32_t res35[3];
    uint32_t mac1;            /* 630 */
    uint32_t res36[3];
#ifdef CONFIG_PLAT_IMX6SX
    uint32_t mac2;            /* 640 */
    uint32_t res37[7];
#else
    uint32_t res37[8];
#endif
    uint32_t gp0;             /* 660 */
    uint32_t res38[3];
    uint32_t gp1;             /* 670 */
    uint32_t res39[3];
    uint32_t res40[20];
    /* Bank 5 */
    uint32_t misc_conf;       /* 6D0 */
    uint32_t res41[3];
    uint32_t field_return;    /* 6E0 */
    uint32_t res42[3];
    uint32_t srk_revoke;      /* 6F0 */
    uint32_t res43[3];
};

struct ocotp {
    int dummy;
};

typedef volatile struct ocotp_regs ocotp_regs_t;

static inline ocotp_regs_t *ocotp_get_regs(struct ocotp *ocotp)
{
    return (ocotp_regs_t *)ocotp;
}

struct ocotp *ocotp_init(ps_io_mapper_t *io_mapper)
{
    return (struct ocotp *)RESOURCE(io_mapper, IMX6_OCOTP);
}

void ocotp_free(struct ocotp *ocotp, ps_io_mapper_t *io_mapper)
{
    UNRESOURCE(io_mapper, IMX6_OCOTP, ocotp);
}

uint64_t ocotp_get_mac(struct ocotp *ocotp, unsigned int id)
{
    assert(ocotp);
    ocotp_regs_t *regs = ocotp_get_regs(ocotp);

    /* i.MX6 dual/quad has one ENET device, i.MX6 Solo X has two. OCOTP usage
     * is defined in the TRM as:
     *     0x620 - 0x630[15:0]:   MAC1_ADDR
     *     0x630[31:16] - 0x640:  MAC2_ADDR
     * Which means if OCOTP was raw memory the MACs
     *   ENET1: <aa>:<bb>:<cc>:<dd>:<ee>:<ff>
     *   ENET2: <gg>:<hh>:<ii>:<jj>:<kk>:<ll>
     * are stored as
     *     0x620:  <ff> <ee> <dd> <cc>  xx xx xx xx  xx xx xx xx  xx xx xx xx
     *     0x630:  <bb> <aa> <ll> <kk>  xx xx xx xx  xx xx xx xx  xx xx xx xx
     *     0x640:  <jj> <ii> <hh> <gg>  xx xx xx xx  xx xx xx xx  xx xx xx xx
     * Reading the OCOTP contents as little endian uint32_t gives
     *     mac0 = 0x<cc><dd><ee><ff>
     *     mac1 = 0x<kk><ll><aa><bb>
     *     mac2 = 0x<gg><hh><ii><jj>
     * We return the MAC as uint64_t
     *     ENET1: 0x0000<aa><bb><cc><dd><ee><ff>
     *     ENET2: 0x0000<gg><hh><ii><jj><kk><ll>
     */

    uint64_t mac = 0;

    switch (id) {
    case 0:
        /* 0x0000<aa><bb><cc><dd><ee><ff> */
        mac = ((uint64_t)((uint16_t)regs->mac1) << 32) | regs->mac0;
        break;

#ifdef CONFIG_PLAT_IMX6SX

    case 1:
        /* 0x0000<gg><hh><ii><jj><kk><ll> */
        mac = ((uint64_t)regs->mac2 << 16) | (regs->mac1 >> 16);
        break;

#endif

    default:
        ZF_LOGE("Unsupported MAC ID %u", id);
        return 0;

    } /* switch (id) */

    /* at least one bit must be set in the MAC to consider it valid */
    if (0 == mac) {
        ZF_LOGE("no MAC in OCOTP for id %d", id);
        return 0;
    }

    ZF_LOGI("OCOTP MAC #%u: %02x:%02x:%02x:%02x:%02x:%02x",
            id, (uint8_t)(mac >> 40), (uint8_t)(mac >> 32),
            (uint8_t)(mac >> 24), (uint8_t)(mac >> 16), (uint8_t)(mac >> 8),
            (uint8_t)mac);

    return mac;
}
