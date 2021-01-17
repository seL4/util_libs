/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2020, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <platsupport/i2c.h>

#include "../../arch/arm/clock.h"
#include "../../services.h"
#include <assert.h>
#include <string.h>
#include <utils/util.h>

#define CCM_PADDR        0x020C4000
#define CCM_SIZE             0x1000

#define CCM_ANALOG_PADDR 0x020C8000
#define CCM_ANALOG_SIZE      0x1000

/* Generic PLL */
#define PLL_LOCK          BIT(31)
#define PLL_BYPASS        BIT(16)

/* SYS PLL */
#define PLL_ARM_DIV_MASK  0x7F
#define PLL_ARM_ENABLE    BIT(13)

/* ENET PLL */
#define PLL_ENET_DIV_MASK 0x3
#define PLL_ENET_ENABLE   BIT(13)

/* USB */
#define PLL_USB_DIV_MASK  0x3
#define PLL_EN_USB_CLKS   BIT(6)
#define PLL_USB_ENABLE    BIT(13)
#define PLL_USB_POWER     BIT(12)

/* Also known as PLL_SYS */
#define PLL2_PADDR 0x020C8030

#define PLL2_CTRL_LOCK          BIT(31)
#define PLL2_CTRL_PDFOFFSET_EN  BIT(18)
#define PLL2_CTRL_BYPASS        BIT(16)
#define PLL2_CTRL_BYPASS_SRC(x) ((x) << 14)
#define PLL2_CTRL_ENABLE        BIT(13)
#define PLL2_CTRL_PWR_DOWN      BIT(12)
#define PLL2_CTRL_DIVSEL        BIT(0)
#define PLL2_SS_STOP(x)         ((x) << 16)
#define PLL2_SS_EN              BIT(15)
#define PLL2_SS_STEP(x)         ((x) <<  0)

#define PLL_CLKGATE BIT(31)
#define PLL_STABLE  BIT(30)
#define PLL_FRAC(x) ((x) << 24)

#define CLKGATE_OFF    0x0
#define CLKGATE_ON_RUN 0x2
#define CLKGATE_ON_ALL 0x3
#define CLKGATE_MASK   CLKGATE_ON_ALL

#define CLKO1_SRC_AHB       (0xBU << 0)
#define CLKO1_SRC_IPG       (0xCU << 0)
#define CLKO1_SRC_MASK      (0xFU << 0)
#define CLKO1_ENABLE        (1U << 7)

#define CLKO2_SRC_MMDC_CH0  (0U << 21)
#define CLKO2_SRC_MASK      (0x1FU << 16)
#define CLKO2_ENABLE        (1U << 24)
#define CLKO_SEL            (1U << 8)

struct ccm_regs {
    uint32_t ccr;      /* 0x000 */
    uint32_t ccdr;     /* 0x004 */
    uint32_t csr;      /* 0x008 */
    uint32_t ccsr;     /* 0x00C */
    uint32_t cacrr;    /* 0x010 */
    uint32_t cbcdr;    /* 0x014 */
    uint32_t cbcmr;    /* 0x018 */
    uint32_t cscmr1;   /* 0x01C */
    uint32_t cscmr2;   /* 0x020 */
    uint32_t cscdr1;   /* 0x024 */
    uint32_t cs1cdr;   /* 0x028 */
    uint32_t cs2cdr;   /* 0x02C */
    uint32_t cdcdr;    /* 0x030 */
    uint32_t chsccdr;  /* 0x034 */
    uint32_t cscdr2;   /* 0x038 */
    uint32_t cscdr3;   /* 0x03C */
    uint32_t res0[2];
    uint32_t cdhipr;   /* 0x048 */
    uint32_t res1[1];
    uint32_t ctor;     /* 0x050 */
    uint32_t clpcr;    /* 0x054 */
    uint32_t cisr;     /* 0x058 */
    uint32_t cimr;     /* 0x05C */
    uint32_t ccosr;    /* 0x060 */
    uint32_t cgpr;     /* 0x064 */
    uint32_t ccgr[7];  /* 0x068 */
    uint32_t res2[1];
    uint32_t cmeor;    /* 0x088 */
};

typedef struct {
    uint32_t val;                    /* 0x00 */
    uint32_t set;                    /* 0x04 */
    uint32_t clr;                    /* 0x08 */
    uint32_t tog;                    /* 0x0C */
} alg_sct_t;

struct ccm_alg_usbphy_regs {
    alg_sct_t vbus_detect;           /* 0x00 */
    alg_sct_t chrg_detect;           /* 0x10 */
    uint32_t vbus_detect_stat;       /* 0x20 */
    uint32_t res0[3];
    uint32_t chrg_detect_stat;       /* 0x30 */
    uint32_t res1[3];
    uint32_t res2[4];
    alg_sct_t misc;                  /* 0x50 */
};

struct ccm_alg_regs {
    /* PLL_ARM */
    alg_sct_t pll_arm;               /* 0x000 */
    /* PLL_USB * 2 */
    alg_sct_t pll_usb[2];            /* 0x010 */
    /* PLL_SYS */
    alg_sct_t pll_sys;               /* 0x030 */
    uint32_t  pll_sys_ss;            /* 0x040 */
    uint32_t  res0[3];
    uint32_t  pll_sys_num;           /* 0x050 */
    uint32_t  res1[3];
    uint32_t  pll_sys_denom;         /* 0x060 */
    uint32_t  res2[3];
    /* PLL_AUDIO */
    alg_sct_t pll_audio;             /* 0x070 */
    uint32_t  pll_audio_num;         /* 0x080 */
    uint32_t  res3[3];
    uint32_t  pll_audio_denom;       /* 0x090 */
    uint32_t  res4[3];
    /* PLL_VIDIO */
    alg_sct_t pll_video;             /* 0x0A0 */
    uint32_t  pll_video_num;         /* 0x0B0 */
    uint32_t  res5[3];
    uint32_t  pll_video_denom;       /* 0x0C0 */
    uint32_t  res6[3];
    /* PLL_MLB */
    alg_sct_t pll_mlb;               /* 0x0D0 */
    /* PLL_ENET */
    alg_sct_t pll_enet;              /* 0x0E0 */
    /* PDF_480 */
    alg_sct_t pfd_480;               /* 0x0F0 */
    /* PDF_528 */
    alg_sct_t pfd_528;               /* 0x100 */
    uint32_t  res7[16];
    /* MISC0 */
    alg_sct_t misc0;                 /* 0x150 */
    uint32_t  res8[4];
    /* MISC2 */
    alg_sct_t misc2;                 /* 0x170 */
    uint32_t  res9[8];
    /* USB phy control is implemented here for the sake of componentisation
     * since it shares the same register space.
     */
    struct ccm_alg_usbphy_regs phy1; /* 0x1a0 */
    struct ccm_alg_usbphy_regs phy2; /* 0x200 */
    uint32_t digprog;                /* 0x260 */
};

static volatile struct clock_regs {
    struct ccm_regs      *ccm;
    struct ccm_alg_regs *alg;
} clk_regs = {.ccm = NULL, .alg = NULL};

struct pll2_regs {
    uint32_t ctrl;
    uint32_t ctrl_s;
    uint32_t ctrl_c;
    uint32_t ctrl_t;
    uint32_t ss;
    uint32_t res0[3];
    uint32_t num;
    uint32_t res1[3];
    uint32_t denom;
    uint32_t res2[3];
};

static struct clock master_clk = { CLK_OPS_DEFAULT(MASTER) };


/*
 *------------------------------------------------------------------------------
 * ARM_CLK
 *------------------------------------------------------------------------------
 */

static freq_t _arm_get_freq(clk_t *clk)
{
    uint32_t div;
    uint32_t fout, fin;
    div = clk_regs.alg->pll_arm.val;
    div &= PLL_ARM_DIV_MASK;
    fin = clk_get_freq(clk->parent);
    fout = fin * div / 2;
    return fout;
}

static freq_t _arm_set_freq(clk_t *clk, freq_t hz)
{
    uint32_t div;
    uint32_t fin;
    uint32_t v;

    fin = clk_get_freq(clk->parent);
    div = 2 * hz / fin;
    div = INRANGE(54, div, 108);
    /* bypass on during clock manipulation */
    clk_regs.alg->pll_arm.set = PLL_BYPASS;
    /* Set the divisor */
    v = clk_regs.alg->pll_arm.val & ~(PLL_ARM_DIV_MASK);
    v |= div;
    clk_regs.alg->pll_arm.val = v;
    /* wait for lock */
    while (!(clk_regs.alg->pll_arm.val & PLL_LOCK));
    /* bypass off */
    clk_regs.alg->pll_arm.clr = PLL_BYPASS;
    return clk_get_freq(clk);
}

static void _arm_recal(clk_t *clk UNUSED)
{
    assert(0);
}

static clk_t *_arm_init(clk_t *clk)
{
    if (clk->priv == NULL) {
        clk_t *parent;
        parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MASTER);
        clk_register_child(parent, clk);
        clk->priv = (void *)&clk_regs;
    }
    return clk;
}

static struct clock arm_clk = { CLK_OPS(ARM, arm, NULL) };


/*
 *------------------------------------------------------------------------------
 * ENET_CLK
 *------------------------------------------------------------------------------
 */

static freq_t _enet_get_freq(clk_t *clk)
{
    uint32_t div;
    uint32_t fin;

    fin = clk_get_freq(clk->parent);
    div = clk_regs.alg->pll_enet.val;
    div &= PLL_ENET_DIV_MASK;
    switch (div) {
    case 3:
        return 5 * fin;
    case 2:
        return 4 * fin;
    case 1:
        return 2 * fin;
    case 0:
        return 1 * fin;
    default:
        return 0 * fin;
    }
}

static freq_t _enet_set_freq(clk_t *clk, freq_t hz)
{
    uint32_t div, fin;
    uint32_t v;
    if (clk_regs.alg == NULL) {
        return clk_get_freq(clk);
    }
    fin = clk_get_freq(clk->parent);
    if (hz >= 5 * fin) {
        div = 3;
    } else if (hz >= 4 * fin) {
        div = 2;
    } else if (hz >= 2 * fin) {
        div = 1;
    } else if (hz >= 1 * fin) {
        div = 0;
    } else {
        div = 0;
    }
    /* bypass on */
    clk_regs.alg->pll_enet.set = PLL_BYPASS;
    v = PLL_ENET_ENABLE | PLL_BYPASS;
    clk_regs.alg->pll_enet.val = v;
    /* Change the frequency */
    v = clk_regs.alg->pll_enet.val & ~(PLL_ENET_DIV_MASK);
    v |= div;
    clk_regs.alg->pll_enet.val = v;
    while (!(clk_regs.alg->pll_enet.val & PLL_LOCK));
    /* bypass off */
    clk_regs.alg->pll_enet.clr = PLL_BYPASS;
    printf("Set ENET frequency to %ld Mhz... ", (long int)clk_get_freq(clk) / MHZ);
    return clk_get_freq(clk);
}

static void _enet_recal(clk_t *clk UNUSED)
{
    assert(0);
}

static clk_t *_enet_init(clk_t *clk)
{
    if (clk->priv == NULL) {
        clk_t *parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MASTER);
        clk_register_child(parent, clk);
        clk->priv = (void *)&clk_regs;
    }
    return clk;
}

static struct clock enet_clk = { CLK_OPS(ENET, enet, NULL) };


/*
 *------------------------------------------------------------------------------
 * PLL2_CLK
 *------------------------------------------------------------------------------
 */

static freq_t _pll2_get_freq(clk_t *clk)
{
    uint32_t p, s;
    struct pll2_regs *regs;
    regs = (struct pll2_regs *)((uint32_t)clk_regs.alg + (PLL2_PADDR & 0xfff));
    assert((regs->ctrl & PLL2_CTRL_LOCK) != 0);
    assert((regs->ctrl & PLL2_CTRL_BYPASS) == 0);
    assert((regs->ctrl & PLL2_CTRL_PWR_DOWN) == 0);
    /* pdf offset? */

    p = clk_get_freq(clk->parent);
    if (regs->ctrl & PLL2_CTRL_DIVSEL) {
        s = 22;
    } else {
        s = 20;
    }
    return p * s;
}

static freq_t _pll2_set_freq(clk_t *clk, freq_t hz)
{
    uint32_t s;
    if (clk_regs.alg == NULL) {
        return clk_get_freq(clk);
    }
    s = hz / clk_get_freq(clk->parent);
    (void)s; /* TODO implement */
    assert(hz == 528 * MHZ);
    return clk_get_freq(clk);
}

static void _pll2_recal(clk_t *clk UNUSED)
{
    assert(0);
}

static clk_t *_pll2_init(clk_t *clk)
{
    if (clk->parent == NULL) {
        clk_t *parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MASTER);
        clk_register_child(parent, clk);
        clk->priv = (void *)&clk_regs;
    }
    return clk;
}

static struct clock pll2_clk = { CLK_OPS(PLL2, pll2, NULL) };


/*
 *------------------------------------------------------------------------------
 * MMDC_CH0_CLK
 *------------------------------------------------------------------------------
 */

static freq_t _mmdc_ch0_get_freq(clk_t *clk)
{
    return clk_get_freq(clk->parent);
}

static freq_t _mmdc_ch0_set_freq(clk_t *clk, freq_t hz)
{
    /* TODO there is a mux here */
    assert(hz == 528 * MHZ);
    return clk_set_freq(clk->parent, hz);
}

static void _mmdc_ch0_recal(clk_t *clk UNUSED)
{
    assert(0);
}

static clk_t *_mmdc_ch0_init(clk_t *clk)
{
    if (clk->parent == NULL) {
        clk_t *parent = clk_get_clock(clk_get_clock_sys(clk), CLK_PLL2);
        clk_register_child(parent, clk);
        clk->priv = (void *)&clk_regs;
    }
    return clk;
}

static struct clock mmdc_ch0_clk = { CLK_OPS(MMDC_CH0, mmdc_ch0, NULL) };


/*
 *------------------------------------------------------------------------------
 * AHB_CLK_ROOT
 *------------------------------------------------------------------------------
 */

static freq_t _ahb_get_freq(clk_t *clk)
{
    return clk_get_freq(clk->parent) / 4;
}

static freq_t _ahb_set_freq(clk_t *clk, freq_t hz)
{
    return clk_set_freq(clk->parent, hz * 4);
}

static void _ahb_recal(clk_t *clk UNUSED)
{
    assert(0);
}

static clk_t *_ahb_init(clk_t *clk)
{
    if (clk->parent == NULL) {
        clk_t *parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MMDC_CH0);
        clk_register_child(parent, clk);
        clk->priv = (void *)&clk_regs;
    }
    return clk;
}

static struct clock ahb_clk = { CLK_OPS(AHB, ahb, NULL) };


/*
 *------------------------------------------------------------------------------
 * IPG_CLK_ROOT
 *------------------------------------------------------------------------------
 */

static freq_t _ipg_get_freq(clk_t *clk)
{
    return clk_get_freq(clk->parent) / 2;
};

static freq_t _ipg_set_freq(clk_t *clk, freq_t hz)
{
    return clk_set_freq(clk->parent, hz * 2);
};

static void _ipg_recal(clk_t *clk UNUSED)
{
    assert(0);
}

static clk_t *_ipg_init(clk_t *clk)
{
    if (clk->parent == NULL) {
        clk_t *parent = clk_get_clock(clk_get_clock_sys(clk), CLK_AHB);
        clk_register_child(parent, clk);
        clk->priv = (void *)&clk_regs;
    }
    return clk;
}

static struct clock ipg_clk = { CLK_OPS(IPG, ipg, NULL) };


/*
 *------------------------------------------------------------------------------
 * USB_CLK
 *------------------------------------------------------------------------------
 */

static freq_t _usb_get_freq(clk_t *clk)
{
    volatile alg_sct_t *pll_usb;
    pll_usb = clk_regs.alg->pll_usb;
    if (clk->id == CLK_USB2) {
        pll_usb++;
    } else if (clk->id != CLK_USB1) {
        assert(0);
        return 0;
    }
    if (pll_usb->val & ~PLL_BYPASS) {
        if (pll_usb->val & (PLL_USB_ENABLE | PLL_USB_POWER | PLL_EN_USB_CLKS)) {
            uint32_t div = (pll_usb->val & PLL_USB_DIV_MASK) ? 22 : 20;
            return clk_get_freq(clk->parent) * div;
        }
    }
    /* Not enabled or in bypass mode...
     * We should only be in bypass when changing Fout */
    return 0;
}

static freq_t _usb_set_freq(clk_t *clk, freq_t hz UNUSED)
{
    assert(!"Not implemented");
    return clk_get_freq(clk);
}

static void _usb_recal(clk_t *clk UNUSED)
{
    assert(0);
}

static clk_t *_usb_init(clk_t *clk)
{
    volatile alg_sct_t *pll_usb;
    if (clk->parent == NULL) {
        clk_t *parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MASTER);
        clk_register_child(parent, clk);
        clk->priv = (void *)&clk_regs;
    }
    if (clk_regs.alg == NULL) {
        ZF_LOGF("clk_regs.alg is NULL: Clocks likely not initialised properly");
        return NULL;
    }
    /* While we are here, gate the clocks */
    pll_usb = clk_regs.alg->pll_usb;
    if (clk->id == CLK_USB2) {
        pll_usb++;
    } else if (clk->id != CLK_USB1) {
        assert(0);
        return NULL;
    }
    pll_usb->clr = PLL_BYPASS;
    pll_usb->set = PLL_USB_ENABLE | PLL_USB_POWER | PLL_EN_USB_CLKS;
    clk_gate_enable(clk_get_clock_sys(clk), usboh3, CLKGATE_ON);
    return clk;
}

static struct clock usb1_clk = { CLK_OPS(USB1, usb, NULL) };
static struct clock usb2_clk = { CLK_OPS(USB2, usb, NULL) };


/*
 *------------------------------------------------------------------------------
 * CLK_Ox
 *------------------------------------------------------------------------------
 */

static freq_t _clko_get_freq(clk_t *clk)
{
    uint32_t fin = clk_get_freq(clk->parent);
    uint32_t div;
    switch (clk->id) {
    case CLK_CLKO1:
        div = (clk_regs.ccm->ccosr >> 4) & 0x7;
        break;
    case CLK_CLKO2:
        div = (clk_regs.ccm->ccosr >> 21) & 0x7;
        break;
    default:
        assert(!"Invalid clock");
        return -1;
    }
    return fin / (div + 1);
}

static freq_t _clko_set_freq(clk_t *clk, freq_t hz)
{
    uint32_t fin = clk_get_freq(clk->parent);
    uint32_t div = (fin / hz) + 1;
    uint32_t v = clk_regs.ccm->ccosr;
    if (div > 0x7) {
        div = 0x7;
    }
    switch (clk->id) {
    case CLK_CLKO1:
        v &= ~(0x7U << 4);
        v |= div << 4;
        break;
    case CLK_CLKO2:
        v &= ~(0x7U << 21);
        v |= div << 21;
        break;
    default:
        assert(!"Invalid clock");
        return -1;
    }
    clk_regs.ccm->ccosr = v;
    return clk_get_freq(clk);
}

static void _clko_recal(clk_t *clk UNUSED)
{
    assert(0);
}

static clk_t *_clko_init(clk_t *clk)
{
    assert(clk_get_clock_sys(clk));
    if (clk->parent == NULL) {
        /* We currently only support 1 src, but there are many to choose from */
        clk_t *parent;
        uint32_t v = clk_regs.ccm->ccosr;
        switch (clk->id) {
        case CLK_CLKO1:
            parent = clk_get_clock(clk_get_clock_sys(clk), CLK_IPG);
            /* set source */
            v &= ~CLKO1_SRC_MASK;
            v |= CLKO1_SRC_IPG;
            /* Enable */
            v |= CLKO1_ENABLE;
            /* Output to CCM_CLKO1 output */
            v &= ~CLKO_SEL;
            break;
        case CLK_CLKO2:
            /* set source */
            parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MMDC_CH0);
            v &= ~CLKO2_SRC_MASK;
            v |= CLKO2_SRC_MMDC_CH0;
            /* Enable */
            v |= CLKO2_ENABLE;
            break;
        default:
            assert(!"Invalid clock for operation");
            return NULL;
        }
        clk_regs.ccm->ccosr = v;
        clk_register_child(parent, clk);
    }
    return clk;
}

static struct clock clko1_clk = { CLK_OPS(CLKO1, clko, NULL) };
static struct clock clko2_clk = { CLK_OPS(CLKO2, clko, NULL) };

static int imx6_gate_enable(clock_sys_t *clock_sys, enum clock_gate gate, enum clock_gate_mode mode)
{
    assert(clk_regs.ccm);
    assert(mode == CLKGATE_ON);
    (void)assert(gate >= 0);
    assert(gate < 112);

    uint32_t v;
    uint32_t reg = gate / 16;
    uint32_t shift = (gate & 0xf) * 2;
    v = clk_regs.ccm->ccgr[reg];
    v &= ~(CLKGATE_MASK << shift);
    v |= (CLKGATE_ON_ALL << shift);
    clk_regs.ccm->ccgr[reg] = v;
    return 0;
}

int clock_sys_init(ps_io_ops_t *o, clock_sys_t *clock_sys)
{
    MAP_IF_NULL(o, CCM, clk_regs.ccm);
    MAP_IF_NULL(o, CCM_ANALOG, clk_regs.alg);
    clock_sys->priv = (void *)&clk_regs;
    clock_sys->get_clock = &ps_get_clock;
    clock_sys->gate_enable = &imx6_gate_enable;
    return 0;
}

void clk_print_clock_tree(clock_sys_t *sys)
{
    clk_t *clk = clk_get_clock(sys, CLK_MASTER);
    clk_print_tree(clk, "");
}

clk_t *ps_clocks[] = {
    [CLK_MASTER]   = &master_clk,
    [CLK_PLL2  ]   = &pll2_clk,
    [CLK_MMDC_CH0] = &mmdc_ch0_clk,
    [CLK_AHB]      = &ahb_clk,
    [CLK_IPG]      = &ipg_clk,
    [CLK_ARM]      = &arm_clk,
    [CLK_ENET]     = &enet_clk,
    [CLK_USB1]     = &usb1_clk,
    [CLK_USB2]     = &usb2_clk,
    [CLK_CLKO1]    = &clko1_clk,
    [CLK_CLKO2]    = &clko2_clk,
};

/* These frequencies are NOT the recommended frequencies. They are to be used
 * when we need to make assumptions about what u-boot has left us with.
 */
freq_t ps_freq_default[] = {
    [CLK_MASTER]   =  24 * MHZ,
    [CLK_PLL2  ]   = 528 * MHZ,
    [CLK_MMDC_CH0] = 528 * MHZ,
    [CLK_AHB]      = 132 * MHZ,
    [CLK_IPG]      =  66 * MHZ,
    [CLK_ARM]      = 792 * MHZ,
    [CLK_ENET]     =  48 * MHZ,
    [CLK_USB1]     = 480 * MHZ,
    [CLK_USB2]     = 480 * MHZ,
    [CLK_CLKO1]    =  66 * MHZ,
    [CLK_CLKO2]    = 528 * MHZ,
};
