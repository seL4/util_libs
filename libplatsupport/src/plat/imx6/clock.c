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

/* common flags for all PLLs */
#define PLL_LOCK                BIT(31)
#define PLL_BYPASS              BIT(16)
#define PLL_GET_BYPASS_SRC(x)   (((x) >> 14) & 0x3)
#define PLL_ENABLE              BIT(13)
#define PLL_PWR_DOWN            BIT(12)

/* PLL_ARM (PLL1), up to 1.3 GHz */
#define PLL_ARM_DIV_MASK        0x7F
#define PLL_ARM_GET_DIV(x)      ((x) & PLL_ARM_DIV_MASK)

/* PLL_SYS (also called PLL2 or PLL_528) runs at a fixed multiplier 22 based
 * on the 24 MHz oscillator to create a 528 MHz a reference clock
 * (24 MHz * 22 = 528 MHz). Other values than 22 are not supposed to be used.
 */
#define PLL_SYS_DIV_MASK        BIT(0)
#define PLL_SYS_GET_DIV(x)      ((x) & PLL_SYS_DIV_MASK)

/* PLL_SYS spread spectrum control (CCM_ANALOG_PLL_SYS_SS) */
#define PLL_SYS_SS_STOP(x)      (((x) & 0xffff) << 16)
#define PLL_SYS_SS_EN           BIT(15)
#define PLL_SYS_SS_STEP(x)      ((x) & 0x7fff)

/* PLL_ENET (PLL6), disabled on reset, runs at a fixed multiplier 20+(5/6)
 * which based on the 24 MHz oscillator as reference clock gives a 500 MHz clock
 * (24 MHz * (20+(5/6)) = 500 MHz). This use used to generate the frequencies
 * 125 MHz (for PCIe and gigabit ethernet), 100 MHz (for SATA) and 50 or 25 MHz
 * for the external ethernet interface.
 */
#define PLL_ENET_ENABLE_100M    BIT(20)
#define PLL_ENET_ENABLE_125M    BIT(19)
#define PLL_ENET_DIV_MASK       0x3
#define PLL_ENET_GET_DIV(x)     ((x) & PLL_ENET_DIV_MASK)

/* PLL_USB. There is PLL3 (USB1_PLL or 480 PLL) that runs at a fixed multiplier
 * 20 and based on the 24 MHz oscillator creates a 480 MHz reference clock
 * (24 MHz * 20 = 480 MHz). It is used for USB0 PHY (OTG PHY) and also to create
 * clocks for UART, CAN, other serial interfaces and audio interfaces.
 * There is also 480_PLL2 (USB2_PLL), which provides clock exclusively to
 * USB2 PHY (also known as HOST PHY) that also runs at a fixed multiplier of 20
 * to generate a 480 MHz clock.
 */
#define PLL_USB_ENABLE_CLKS     BIT(6)
#define PLL_USB_DIV_MASK        0x3
#define PLL_USB_GET_DIV(x)      ((x) & PLL_USB_DIV_MASK)


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

typedef volatile struct {
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
} ccm_regs_t;

typedef volatile struct {
    uint32_t val;                    /* 0x00 */
    uint32_t set;                    /* 0x04 */
    uint32_t clr;                    /* 0x08 */
    uint32_t tog;                    /* 0x0C */
} alg_sct_t;

typedef volatile struct {
    alg_sct_t vbus_detect;           /* 0x00 */
    alg_sct_t chrg_detect;           /* 0x10 */
    uint32_t vbus_detect_stat;       /* 0x20 */
    uint32_t res0[3];
    uint32_t chrg_detect_stat;       /* 0x30 */
    uint32_t res1[3];
    uint32_t res2[4];
    alg_sct_t misc;                  /* +0x50 */
} ccm_alg_usbphy_regs_t;

typedef volatile struct {
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
    ccm_alg_usbphy_regs_t phy[2];    /* 0x1a0, 0x200 */
    uint32_t digprog;                /* 0x260 */
} ccm_alg_regs_t;

static struct {
    ccm_regs_t     *ccm;
    ccm_alg_regs_t *alg;
} clk_regs = { .ccm = NULL, .alg = NULL};

static struct clock master_clk = { CLK_OPS_DEFAULT(MASTER) };

static int change_pll(alg_sct_t *pll, uint32_t div_mask, uint32_t div)
{
    /* div can't exceed the mask */
    assert(0 == (div & (~div_mask)));

    /* bypass on during clock manipulation */
    pll->set = PLL_BYPASS;
    /* power down the PLL */
    pll->set = PLL_PWR_DOWN;
    /* set the divisor */
    pll->clr = div_mask;
    pll->set = div;
    /* power up the PLL */
    pll->clr = PLL_PWR_DOWN;

    /* wait for PLL to be stable, stop after a bit over 65 million loops */
    unsigned int loop_cnt = 0x4000000;
    for (;;) {
        if (pll->val & PLL_LOCK) {
            break;
        }

        if (0 == loop_cnt--) {
            ZF_LOGE("waiting for PLL_LOCK aborted");
            return -1;
        }
    }

    /* bypass off */
    pll-> clr = PLL_BYPASS;
    /* ensure PLL output is enabled */
    pll->set = PLL_ENABLE;

    return 0;
}


/*
 *------------------------------------------------------------------------------
 * ARM_CLK
 *------------------------------------------------------------------------------
 */

static freq_t _arm_get_freq(clk_t *clk)
{
    if (!clk_regs.alg) {
        ZF_LOGE("clk_regs.alg is NULL, clocks not initialised properly");
        return 0;
    }

    uint32_t v = clk_regs.alg->pll_enet.val;

    /* clock output enabled? */
    if (!(v & PLL_ENABLE)) {
        /* we should never be here, because we are running on the ARM core */
        return 0;
    }

    if (v & PLL_BYPASS) {
        /* bypass on
         * 0x0 source is 24MHz oscillator
         * 0x1 source is CLK1_N/CLK1_P
         * 0x2 source is CLK2_N/CLK2_P
         * 0x3 source is CLK1_N/CLK1_P XOR CLK2_N/CLK2_P
         */
        unsigned int src = PLL_GET_BYPASS_SRC(v);
        switch (src) {
        case 0:
            return 24 * MHZ;
        default:
            break;
        }
        ZF_LOGE("can't determine frequency for bypass sources %u", src);
        return 0;
    }

    /* PLL enabled, but powered down? */
    if (v & PLL_PWR_DOWN) {
        /* we should never be here, because we are running on the ARM core */
        return 0;
    }

    /* valid divider range 54 to 108, F_out = F_in * div_select / 2 */
    unsigned int div = PLL_ARM_GET_DIV(v);
    if ((div < 54) || (div > 108)) {
        ZF_LOGE("PLL_ARM divider out of valid range 54-108: %u", div);
    }
    freq_t f_in = clk_get_freq(clk->parent);
    return f_in * div / 2;
}

static freq_t _arm_set_freq(clk_t *clk, freq_t hz)
{
    ZF_LOGI("clock '%s': switch from %u Mhz to %llu (%u Mhz)",
            clk->name, (int)(clk_get_freq(clk) / MHZ), hz, (int)(hz / MHZ));

    if (!clk_regs.alg) {
        ZF_LOGE("clk_regs.alg is NULL, clocks not initialised properly");
        return 0;
    }

    alg_sct_t *pll = &clk_regs.alg->pll_arm;

    freq_t fin = clk_get_freq(clk->parent);
    unsigned int div = 2 * hz / fin;
    if ((div < 54) || (div > 108)) {
        ZF_LOGE("PLL_ARM divider out of valid range 54-108: %u", div);
        return 0;
    }

    int ret = change_pll(pll, PLL_ARM_DIV_MASK, div);
    if (0 != ret) {
        ZF_LOGE("waiting for PLL_ARM lock aborted, code %d", ret);
        return 0;
    }

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
    if (!clk_regs.alg) {
        ZF_LOGE("clk_regs.alg is NULL, clocks not initialised properly");
        return 0;
    }

    uint32_t v = clk_regs.alg->pll_enet.val;

    /* clock output enabled? */
    if (!(v & PLL_ENABLE)) {
        return 0;
    }

    if (v & PLL_BYPASS) {
        /* bypass on
         * 0x0 source is 24MHz oscillator
         * 0x1 source is CLK1_N/CLK1_P
         * 0x2 source is CLK2_N/CLK2_P
         * 0x3 source is CLK1_N/CLK1_P XOR CLK2_N/CLK2_P
         */
        unsigned int src = PLL_GET_BYPASS_SRC(v);
        switch (src) {
        case 0:
            return 24 * MHZ;
        default:
            break;
        }
        ZF_LOGE("can't determine frequency for bypass sources %u", src);
        return 0;
    }

    /* PLL enabled, but powered down? */
    if (v & PLL_PWR_DOWN) {
        return 0;
    }

    unsigned int div = v & PLL_ENET_DIV_MASK;
    switch (div) {
    case 0:
        return  25 * MHZ;
    case 1:
        return  50 * MHZ;
    case 2:
        return 100 * MHZ;
    case 3:
        return 125 * MHZ;
    default:
        break;
    }
    ZF_LOGE("unsupported ENET PLL divider %u", v);
    return 0;
}

static freq_t _enet_set_freq(clk_t *clk, freq_t hz)
{
    if (!clk_regs.alg) {
        ZF_LOGE("clk_regs.alg is NULL, clocks not initialised properly");
        return 0;
    }

#if defined(CONFIG_PLAT_IMX6DQ)

    alg_sct_t *pll = &clk_regs.alg->pll_enet;

    uint32_t div;
    switch (hz) {
    case 125 * MHZ:
        div = 3;
        break;
    case 100 * MHZ:
        div = 2;
        break;
    case  50 * MHZ:
        div = 1;
        break;
    case  25 * MHZ:
        div = 0;
        break;
    default:
        ZF_LOGE("unsupported ENET clock frequency %d", hz);
        return 0;
    }

    // ENET requires ahb_clk_root to be at least 125 MHz. In the clock tree
    // PLL_SYS feeds CLK_MMDC_CH0 which feeds CLK_AHB. For CLK_AHB the default
    // divider is 4, set via CBCDR.AHB_PODF, so setting PLL_SYS to the standard
    // 528 MHz will result in an AHB clock of 132 MHz.
    freq_t f = clk_get_freq(clk_get_clock(clk->clk_sys, CLK_AHB));
    if (f < (125 * MHZ)) {
        ZF_LOGI("setting system PLL from %llu to 528 MHz", f);
        clk_set_freq(clk_get_clock(clk->clk_sys, CLK_PLL2), 528 * MHZ);
        f = clk_get_freq(clk_get_clock(clk->clk_sys, CLK_AHB));
        if (f < (125 * MHZ)) {
            ZF_LOGE("clock tree setup broken, AHB clock is %llu", f);
            return 0;
        }
    }

    int ret = change_pll(pll, PLL_ENET_DIV_MASK, div);
    if (0 != ret) {
        ZF_LOGE("waiting for PLL_ENET lock aborted, code %d", ret);
        return 0;
    }

    clk_gate_enable(clk_get_clock_sys(clk), enet_clock, CLKGATE_ON);

#elif defined(CONFIG_PLAT_IMX6SX)

    /* ENET requires enet_clk_root to be at least 133 MHz. However, we don't do
     * anything here and just trust u-boot to have set things up properly.
     */

#else
#error "unknown i.MX6 SOC"
#endif

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
 * PLL_SYS
 *------------------------------------------------------------------------------
 */

static freq_t _pll_sys_get_freq(clk_t *clk)
{
    if (!clk_regs.alg) {
        ZF_LOGE("clk_regs.alg is NULL, clocks not initialised properly");
        return 0;
    }

    uint32_t v = clk_regs.alg->pll_sys.val;

    /* clock output enabled? */
    if (!(v & PLL_ENABLE)) {
        return 0;
    }

    if (v & PLL_BYPASS) {
        /* bypass on
         * 0x0 source is 24MHz oscillator
         * 0x1 source is CLK1_N/CLK1_P
         * 0x2 source is CLK2_N/CLK2_P
         * 0x3 source is CLK1_N/CLK1_P XOR CLK2_N/CLK2_P
         */
        unsigned int src = PLL_GET_BYPASS_SRC(v);
        switch (src) {
        case 0:
            return 24 * MHZ;
        default:
            break;
        }
        ZF_LOGE("can't determine frequency for bypass sources %u", src);
        return 0;
    }

    /* PLL enabled, but powered down? */
    if (v & PLL_PWR_DOWN) {
        return 0;
    }

    freq_t f_parent = clk_get_freq(clk->parent);
    unsigned int div = PLL_SYS_GET_DIV(v);
    switch (div) {
    case 0:
        return  f_parent * 20;
    case 1:
        return  f_parent * 22;
    default:
        break;
    }
    ZF_LOGE("unsupported PLL_SYS divisor %u", div);
    return 0;
}

static freq_t _pll_sys_set_freq(clk_t *clk, freq_t hz)
{
    ZF_LOGI("clock '%s': switch from %u Mhz to %llu (%u Mhz)",
            clk->name, (int)(clk_get_freq(clk) / MHZ), hz, (int)(hz / MHZ));

    if (!clk_regs.alg) {
        ZF_LOGE("clk_regs.alg is NULL, clocks not initialised properly");
        return 0;
    }

    alg_sct_t *pll = &clk_regs.alg->pll_sys;

    if (24 * MHZ == hz) {
        /* bypass PLL and use 24 MHz oscillator */
        pll->set = PLL_BYPASS;
        pll->set = PLL_PWR_DOWN;
        pll->clr = PLL_ENET_DIV_MASK;
        pll->set = PLL_ENABLE;
    } else {
        uint32_t div;
        switch (hz) {
        case 480 * MHZ:
            div = 0;
            break; // 480 MHz = 20 * 24 MHz
        case 528 * MHZ:
            div = 1;
            break; // 528 MHz = 22 * 24 MHz
        default:
            ZF_LOGE("unsupported PLL_SYS clock frequency %llu", hz);
            return 0;
        }

        int ret = change_pll(pll, PLL_SYS_DIV_MASK, div);
        if (0 != ret) {
            ZF_LOGE("PLL_SYS change failed, code %d", ret);
            return 0;
        }
    }

    return clk_get_freq(clk);
}

static void _pll_sys_recal(clk_t *clk UNUSED)
{
    ZF_LOGE("PLL_SYS recal is not supported");
    assert(0);
}

static clk_t *_pll_sys_init(clk_t *clk)
{
    if (clk->parent == NULL) {
        clk_t *parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MASTER);
        clk_register_child(parent, clk);
        clk->priv = (void *)&clk_regs;
    }

    // After a reset, the system PLL is not active and the clock signal comes
    // from the 24 MHz oscillator. We don't reconfigure anything here, enabling
    // the 528 MHz PLL must be done manually somewhere. Note that if U-Boot is
    // used, it may have done this already.

    return clk;
}

static struct clock pll_sys_clk = { CLK_OPS(PLL2, pll_sys, NULL) };


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
    /* TODO: there are the two MUXers here:
     *         - CCM Bus Clock Multiplexer CBCMR.PRE_PERIPH_CLK_SEL
     *         - CCM Bus Clock Divider CBCDR.PERIPH_CLK_SEL
     *       by default they route the clock PLL_SYS (PLL2)
     */
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
    /* ToDo: read divider from CBCDR.AHB_PODF */
    return clk_get_freq(clk->parent) / 4;
}

static freq_t _ahb_set_freq(clk_t *clk, freq_t hz)
{
    /* ToDo: we assume the default value CBCDR.AHB_PODF = b011 has not been
     * changed and thus the divider is 4 (132 MHZ * 4 = 528 MHz).
     */
    assert(hz == 132 * MHZ);
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
    unsigned int idx;
    switch (clk->id) {
    case CLK_USB1:
        idx = 0;
        break;
    case CLK_USB2:
        idx = 1;
        break;
    default:
        ZF_LOGE("invalid USB clock ID: %u", clk->id);
        return 0;
    }

    if (!clk_regs.alg) {
        ZF_LOGE("clk_regs.alg is NULL, clocks not initialised properly");
        return 0;
    }

    uint32_t v = clk_regs.alg->pll_usb[idx].val;

    /* clock output enabled? */
    if (!(v & PLL_ENABLE)) {
        return 0;
    }

    if (v & PLL_BYPASS) {
        /* bypass on
         * 0x0 source is 24MHz oscillator
         * 0x1 source is CLK1_N/CLK1_P
         * 0x2 source is CLK2_N/CLK2_P
         * 0x3 source is CLK1_N/CLK1_P XOR CLK2_N/CLK2_P
         */
        unsigned int src = PLL_GET_BYPASS_SRC(v);
        switch (src) {
        case 0:
            return 24 * MHZ;
        default:
            break;
        }
        ZF_LOGE("can't determine frequency for bypass sources %u", src);
        return 0;
    }

    /* PLL enabled, but powered down? */
    if (v & PLL_PWR_DOWN) {
        return 0;
    }

    freq_t f_parent = clk_get_freq(clk->parent);
    unsigned int div = PLL_USB_GET_DIV(v);
    switch (div) {
    case 0:
        return  f_parent * 20;
    case 1:
        return  f_parent * 22;
    default:
        break;
    }
    ZF_LOGE("unsupported USB PLL divider %u", v);
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
    if (clk->parent == NULL) {
        clk_t *parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MASTER);
        clk_register_child(parent, clk);
        clk->priv = (void *)&clk_regs;
    }

    unsigned int idx;
    switch (clk->id) {
    case CLK_USB1:
        idx = 0;
        break;
    case CLK_USB2:
        idx = 1;
        break;
    default:
        ZF_LOGE("invalid USB clock ID: %u", clk->id);
        return NULL;
    }

    if (!clk_regs.alg) {
        ZF_LOGE("clk_regs.alg is NULL, clocks not initialised properly");
        return NULL;
    }
    alg_sct_t *pll = &clk_regs.alg->pll_usb[idx];

    /* While we are here, gate the clocks */
    pll->clr = PLL_BYPASS;
    pll->set = PLL_ENABLE | PLL_PWR_DOWN | PLL_USB_ENABLE_CLKS;
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
    [CLK_PLL2]     = &pll_sys_clk,
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
    [CLK_PLL2]     = 528 * MHZ, /* PLL_SYS */
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
