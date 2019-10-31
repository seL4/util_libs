/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
#include "src.h"
#include "../../arch/arm/clock.h"
#include "../../services.h"
#include <assert.h>
#include <string.h>
#include <utils/util.h>

/****************
 ****  PLLs  ****
 ****************/

/* PLL Control (<pll>_PLL_CTRL) Register */
#define PLL_CTRL_RESET                  BIT(0)
#define PLL_CTRL_PWRDWN                 BIT(1)
#define PLL_CTRL_BYPASS_QUAL            BIT(3)
#define PLL_CTRL_BYPASS_FORCE           BIT(4)

#define PLL_CTRL_FDIV_SHIFT             12
#define PLL_CTRL_FDIV(x)                ((x) * BIT(PLL_CTRL_FDIV_SHIFT))
#define PLL_CTRL_FDIV_MASK              PLL_CTRL_FDIV(0x7F)
#define PLL_CTRL_FDIV_MIN               13
#define PLL_CTRL_FDIV_MAX               66

/* PLL Status Register */
#define PLL_STATUS_ARM_PLL_LOCK         BIT(0)
#define PLL_STATUS_DDR_PLL_LOCK         BIT(1)
#define PLL_STATUS_IO_PLL_LOCK          BIT(2)
#define PLL_STATUS_ARM_PLL_STABLE       BIT(3)
#define PLL_STATUS_DDR_PLL_STABLE       BIT(4)
#define PLL_STATUS_IO_PLL_STABLE        BIT(5)

/* PLL Configuration (<pll>_PLL_CFG) Register */
#define PLL_CFG(pll_cp, pll_res, lock_cnt)  \
    (((lock_cnt) << 12) | (pll_cp << 8) | (pll_res << 4))

/* Required PLL Frequency Configuration Settings */
#define PLL_CFG_FDIV13      PLL_CFG(2,  6, 750)
#define PLL_CFG_FDIV14      PLL_CFG(2,  6, 700)
#define PLL_CFG_FDIV15      PLL_CFG(2,  6, 650)
#define PLL_CFG_FDIV16      PLL_CFG(2, 10, 625)
#define PLL_CFG_FDIV17      PLL_CFG(2, 10, 575)
#define PLL_CFG_FDIV18      PLL_CFG(2, 10, 550)
#define PLL_CFG_FDIV19      PLL_CFG(2, 10, 525)
#define PLL_CFG_FDIV20      PLL_CFG(2, 12, 500)
#define PLL_CFG_FDIV21      PLL_CFG(2, 12, 475)
#define PLL_CFG_FDIV22      PLL_CFG(2, 12, 450)
#define PLL_CFG_FDIV23      PLL_CFG(2, 12, 425)
#define PLL_CFG_FDIV24      PLL_CFG(2, 12, 400)
#define PLL_CFG_FDIV25      PLL_CFG(2, 12, 400)
#define PLL_CFG_FDIV26      PLL_CFG(2, 12, 375)
#define PLL_CFG_FDIV27      PLL_CFG(2, 12, 350)
#define PLL_CFG_FDIV28      PLL_CFG(2, 12, 350)
#define PLL_CFG_FDIV29      PLL_CFG(2, 12, 325)
#define PLL_CFG_FDIV30      PLL_CFG(2, 12, 325)
#define PLL_CFG_FDIV31      PLL_CFG(2,  2, 300)
#define PLL_CFG_FDIV32      PLL_CFG(2,  2, 300)
#define PLL_CFG_FDIV33      PLL_CFG(2,  2, 300)
#define PLL_CFG_FDIV34      PLL_CFG(2,  2, 275)
#define PLL_CFG_FDIV35      PLL_CFG(2,  2, 275)
#define PLL_CFG_FDIV36      PLL_CFG(2,  2, 275)
#define PLL_CFG_FDIV37      PLL_CFG(2,  2, 250)
#define PLL_CFG_FDIV38      PLL_CFG(2,  2, 250)
#define PLL_CFG_FDIV39      PLL_CFG(2,  2, 250)
#define PLL_CFG_FDIV40      PLL_CFG(2,  2, 250)
#define PLL_CFG_FDIV41      PLL_CFG(3, 12, 250)
#define PLL_CFG_FDIV42      PLL_CFG(3, 12, 250)
#define PLL_CFG_FDIV43      PLL_CFG(3, 12, 250)
#define PLL_CFG_FDIV44      PLL_CFG(3, 12, 250)
#define PLL_CFG_FDIV45      PLL_CFG(3, 12, 250)
#define PLL_CFG_FDIV46      PLL_CFG(3, 12, 250)
#define PLL_CFG_FDIV47      PLL_CFG(3, 12, 250)
#define PLL_CFG_FDIV48      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV49      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV50      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV51      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV52      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV53      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV54      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV55      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV56      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV57      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV58      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV59      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV60      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV61      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV62      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV63      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV64      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV65      PLL_CFG(2,  4, 250)
#define PLL_CFG_FDIV66      PLL_CFG(2,  4, 250)

struct pll_cfg_t {
    uint8_t fdiv;
    uint32_t pll_cfg;
};

static struct pll_cfg_t pll_cfg_tbl[] = {
    /* Need to offset by PLL_CTRL_FDIV_MIN when accessing a row in the table */
    {13, PLL_CFG_FDIV13},
    {14, PLL_CFG_FDIV14},
    {15, PLL_CFG_FDIV15},
    {16, PLL_CFG_FDIV16},
    {17, PLL_CFG_FDIV17},
    {18, PLL_CFG_FDIV18},
    {19, PLL_CFG_FDIV19},
    {20, PLL_CFG_FDIV20},
    {21, PLL_CFG_FDIV21},
    {22, PLL_CFG_FDIV22},
    {23, PLL_CFG_FDIV23},
    {24, PLL_CFG_FDIV24},
    {25, PLL_CFG_FDIV25},
    {26, PLL_CFG_FDIV26},
    {27, PLL_CFG_FDIV27},
    {28, PLL_CFG_FDIV28},
    {29, PLL_CFG_FDIV29},
    {30, PLL_CFG_FDIV30},
    {31, PLL_CFG_FDIV31},
    {32, PLL_CFG_FDIV32},
    {33, PLL_CFG_FDIV33},
    {34, PLL_CFG_FDIV34},
    {35, PLL_CFG_FDIV35},
    {36, PLL_CFG_FDIV36},
    {37, PLL_CFG_FDIV37},
    {38, PLL_CFG_FDIV38},
    {39, PLL_CFG_FDIV39},
    {40, PLL_CFG_FDIV40},
    {41, PLL_CFG_FDIV41},
    {42, PLL_CFG_FDIV42},
    {43, PLL_CFG_FDIV43},
    {44, PLL_CFG_FDIV44},
    {45, PLL_CFG_FDIV45},
    {46, PLL_CFG_FDIV46},
    {47, PLL_CFG_FDIV47},
    {48, PLL_CFG_FDIV48},
    {49, PLL_CFG_FDIV49},
    {50, PLL_CFG_FDIV50},
    {51, PLL_CFG_FDIV51},
    {52, PLL_CFG_FDIV52},
    {53, PLL_CFG_FDIV53},
    {54, PLL_CFG_FDIV54},
    {55, PLL_CFG_FDIV55},
    {56, PLL_CFG_FDIV56},
    {57, PLL_CFG_FDIV57},
    {58, PLL_CFG_FDIV58},
    {59, PLL_CFG_FDIV59},
    {60, PLL_CFG_FDIV60},
    {61, PLL_CFG_FDIV61},
    {62, PLL_CFG_FDIV62},
    {63, PLL_CFG_FDIV63},
    {64, PLL_CFG_FDIV64},
    {65, PLL_CFG_FDIV65},
    {66, PLL_CFG_FDIV66}
};

/******************
 ****  Clocks  ****
 ******************/

/* Clock Source */
#define CLK_SRC_IO_PLL      0x01
#define CLK_SRC_ARM_PLL     0x02
#define CLK_SRC_DDR_PLL     0x03

#define CLK_SRCSEL(x)       ((x) << 4)
#define CLK_GET_SRCSEL(x)   (((x) >> 4) & 0x3)

/* Each clock uses a 6-bit divisor */
#define CLK_DIVISOR_MIN     1
#define CLK_DIVISOR_MAX     0x3F

/*
 * The clock divider is located at bits 8:13. Some clock generators have two
 * cascated dividers, with the second divider located at bits 20:25.
 */
#define CLK_DIVISOR0_SHIFT                  8
#define CLK_DIVISOR1_SHIFT                  20

#define CLK_DIVISOR(div, x)                 ((x) * BIT(CLK_DIVISOR##div##_SHIFT))
#define CLK_DIVISOR_MASK(div)               CLK_DIVISOR(div, CLK_DIVISOR_MAX)
#define CLK_SET_DIVISOR(div, reg, val)      \
    do {                                    \
        uint32_t v;                         \
        v = reg & ~(CLK_DIVISOR_MASK(div)); \
        reg = v | CLK_DIVISOR(div, val);    \
    } while (0)
#define CLK_GET_DIVISOR(div, reg)           \
    ((reg & CLK_DIVISOR_MASK(div)) >> CLK_DIVISOR##div##_SHIFT)

/* Clock Control (enable/disable a clock) */
#define CLK_CLKACT          BIT(0)

/**********************
 ****  CPU Clocks  ****
 **********************/

/* CPU Clock Control */
#define CPU_CLK_CTRL_CPU_6OR4XCLKACT        BIT(24)
#define CPU_CLK_CTRL_CPU_3OR2XCLKACT        BIT(25)
#define CPU_CLK_CTRL_CPU_2XCLKACT           BIT(26)
#define CPU_CLK_CTRL_CPU_1XCLKACT           BIT(27)
#define CPU_PERI_CLKACT                     BIT(28)

/* CPU Clock Ratio Mode Select */
#define CPU_CLK_621_TRUE                    BIT(0)

/**********************
 ****  DDR Clocks  ****
 **********************/

/* DDR Clock Control */
#define DDR_CLK_CTRL_DDR_3XCLKACT   BIT(0)
#define DDR_CLK_CTRL_DDR_2XCLKACT   BIT(1)

/* DDR Clock Divisors */
#define DDR_3XCLK_DIVISOR_SHIFT     20
#define DDR_2XCLK_DIVISOR_SHIFT     26
#define DDR_CLK_DIVISOR(dom, x)     ((x) * BIT(DDR_##dom##XCLK_DIVISOR_SHIFT))
#define DDR_CLK_DIVISOR_MASK(dom)   DDR_CLK_DIVISOR(dom, CLK_DIVISOR_MAX)
#define DDR_CLK_SET_DIVISOR(dom, val)                            \
    do {                                                         \
        clk_regs->ddr_clk_ctrl &= ~(DDR_CLK_DIVISOR_MASK(dom));  \
        clk_regs->ddr_clk_ctrl |= DDR_CLK_DIVISOR(dom, val);     \
    } while (0)
#define DDR_CLK_GET_DIVISOR(dom)    \
    ((clk_regs->ddr_clk_ctrl & DDR_CLK_DIVISOR_MASK(dom)) >> DDR_##dom##XCLK_DIVISOR_SHIFT)

/**************************
 ****  FPGA PL Clocks  ****
 **************************/

/* Programmable Logic (PL) - i.e. FPGA - clock registers */
typedef volatile struct {
    uint32_t clk_ctrl;  /* PL Clock x Output Control */
    uint32_t thr_ctrl;  /* PL Clock x Throttle Control */
    uint32_t thr_cnt;   /* PL Clock x Throttle Count */
    uint32_t thr_sta;   /* PL Clock x Throttle Status */
} pl_clk_regs_t;

/***************************
 ****  Clock Registers  ****
 ***************************/

struct zynq7000_clk_regs {
    /* We are only interested in the clock registers in SLCR */
    uint32_t arm_pll_ctrl;      /* 0x100 ARM PLL Control */
    uint32_t ddr_pll_ctrl;      /* 0x104 DDR PLL Control */
    uint32_t io_pll_ctrl;       /* 0x108 IO PLL Control */
    uint32_t pll_status;        /* 0x10C PLL Status */

    uint32_t arm_pll_cfg;       /* 0x110 ARM PLL Configuration */
    uint32_t ddr_pll_cfg;       /* 0x114 DDR PLL Configuration */
    uint32_t io_pll_cfg;        /* 0x118 IO PLL Configuration */
    uint32_t res0[1];
    uint32_t arm_clk_ctrl;      /* 0x120 CPU Clock Control */
    uint32_t ddr_clk_ctrl;      /* 0x124 DDR Clock Control */
    uint32_t dci_clk_ctrl;      /* 0x128 DCI Clock Control */

    uint32_t aper_clk_ctrl;     /* 0x12C AMBDA Peripheral Clock Control */
    uint32_t usb0_clk_ctrl;     /* 0x130 USB 0 ULPI Clock Control */
    uint32_t usb1_clk_ctrl;     /* 0x134 USB 1 ULPI Clock Control */
    uint32_t gem0_rclk_ctrl;    /* 0x138 GigE 0 Rx Clock and Rx Signals Select */
    uint32_t gem1_rclk_ctrl;    /* 0x13C GigE 1 Rx Clock and Rx Signals Select */
    uint32_t gem0_clk_ctrl;     /* 0x140 GigE 0 Ref Clock Control */
    uint32_t gem1_clk_ctrl;     /* 0x144 GigE 1 Ref Clock Control */
    uint32_t smc_clk_ctrl;      /* 0x148 SMC Ref Clock Control */
    uint32_t lqspi_clk_ctrl;    /* 0x14C QUAD SPI REf Clock Control */
    uint32_t sdio_clk_ctrl;     /* 0x150 SDIO Ref Clock Control */
    uint32_t uart_clk_ctrl;     /* 0x154 UART Ref Clock Control */
    uint32_t spi_clk_ctrl;      /* 0x158 SPI Ref Clock Control */
    uint32_t can_clk_ctrl;      /* 0x15C CAN Ref Clock Control */
    uint32_t can_mioclk_ctrl;   /* 0x160 CAN MIO Clock Control */
    uint32_t dbg_clk_ctrl;      /* 0x164 SoC Debug Clock Control */
    uint32_t pcap_clk_ctrl;     /* 0x168 PCAP Clock Control */
    uint32_t topsw_clk_ctrl;    /* 0x16C Central Interconnect Clock Control */
    pl_clk_regs_t fpga_clk[4];  /* 0x170 PL Clock 0 */
    uint32_t pad1[5];
    uint32_t clk_621_true;      /* 0x1C4 CPU Clock Ratio Mode Select */
    uint32_t pad2[79];
    uint32_t wdt_clk_sel;       /* 0x304 SWDT Clock Source Select */
};

static const enum clk_id cpu_clk_src[] = {
                                             CLK_ARM_PLL,
                                             CLK_ARM_PLL,
                                             CLK_DDR_PLL,
                                             CLK_IO_PLL
                                         };

static const enum clk_id generic_clk_src[] = {
                                                 CLK_IO_PLL,
                                                 CLK_IO_PLL,
                                                 CLK_ARM_PLL,
                                                 CLK_DDR_PLL
                                             };

#define fpga_clk_src generic_clk_src
#define can_clk_src generic_clk_src
#define pcap_clk_src generic_clk_src

static volatile struct zynq7000_clk_regs* clk_regs = NULL;

/* Set divisors, avoiding over clocking peripherals */
static inline void
set_divs(volatile uint32_t* ctrl, uint8_t div0, uint8_t div1)
{
    uint8_t old_div0;
    old_div0 = CLK_GET_DIVISOR(0, *ctrl);
    if (div0 > old_div0) {
        CLK_SET_DIVISOR(0, *ctrl, div0);
        CLK_SET_DIVISOR(1, *ctrl, div1);
    } else {
        CLK_SET_DIVISOR(1, *ctrl, div1);
        CLK_SET_DIVISOR(0, *ctrl, div0);
    }
}

/* Set divisors where only one divisor is available */
static inline void
set_div(volatile uint32_t* ctrl, uint8_t div0)
{
    CLK_SET_DIVISOR(0, *ctrl, div0);
}

/*
 * Calculate the clock rate divisors
 * @param        hz: Desired frequency
 * @param parent_hz: Parent clock's frequency
 * @param     rdiv0: Calculated DIVISOR0 value (returned)
 * @param     rdiv1: Calculated DIVISOR1 value (returned)
 * @return         : The actual frequency based on the calculated values
 */
static freq_t
zynq7000_clk_calc_divs(freq_t hz, freq_t parent_hz, uint8_t* rdiv0,
                       uint8_t* rdiv1)
{
    /*
     * Safety check.
     *
     * rdiv1 is allowed to be NULL if the particular clock does not have a
     * second divider.
     */
    assert(rdiv0 != NULL);

    uint8_t div0, div1, div1_max;
    freq_t calc_freq, rfreq = 0;
    uint32_t freq_error, best_freq_error = ~0;

    /*
     * The maximum value for div1 is dictated by whether a particular
     * clock uses one divisor or two.
     *
     * If the clock only has one divisor (i.e. rdiv1 is NULL), then this
     * value will not change.
     */
    if (rdiv1 == NULL) {
        div1_max = 1;
    } else {
        div1_max = CLK_DIVISOR_MAX;
    }

    /*
     * Calculate values for div0 and div1 based on the desired clock
     * frequency.
     */
    for (div0 = CLK_DIVISOR_MIN; div0 <= CLK_DIVISOR_MAX; div0++) {
        for (div1 = CLK_DIVISOR_MIN; div1 <= div1_max; div1++) {
            calc_freq = parent_hz / (div0 * div1);

            if (hz > calc_freq) {
                freq_error = hz - calc_freq;
            } else {
                freq_error = calc_freq - hz;
            }

            if (freq_error < best_freq_error) {
                best_freq_error = freq_error;
                *rdiv0 = div0;
                if (rdiv1 != NULL) {
                    *rdiv1 = div1;
                }
                rfreq = calc_freq;

                /* Short-circuit */
                if (freq_error == 0) {
                    goto end;
                }
            }
        }
    }

end:
    return rfreq;
}

/*
 * Ensure the divisor is an even number. If it isn't, make it in even number.
 * @param divisor: Divisor to check
 * @return       : An even-number divisor
 */
static inline uint8_t
zynq7000_even_divisor(uint8_t divisor)
{
    if ((divisor % 2) != 0) {
        return INRANGE(CLK_DIVISOR_MIN + 1, divisor - 1, CLK_DIVISOR_MAX - 1);
    } else {
        return divisor;
    }
}

/* PS_CLK */
static struct clock master_clk = { CLK_OPS_DEFAULT(MASTER) };

static uint32_t
_decode_pll(const clk_t* clk, volatile uint32_t** ctrl, volatile uint32_t** cfg)
{
    switch (clk->id) {
    case CLK_ARM_PLL:
        *ctrl = &clk_regs->arm_pll_ctrl;
        *cfg = &clk_regs->arm_pll_cfg;
        return PLL_STATUS_ARM_PLL_LOCK;
    case CLK_DDR_PLL:
        *ctrl = &clk_regs->ddr_pll_ctrl;
        *cfg = &clk_regs->ddr_pll_cfg;
        return PLL_STATUS_DDR_PLL_LOCK;
    case CLK_IO_PLL:
        *ctrl = &clk_regs->io_pll_ctrl;
        *cfg = &clk_regs->io_pll_cfg;
        return PLL_STATUS_IO_PLL_LOCK;
    default:
        assert(!"Invalid clock");
        return 0;
    }
}

/* PLLs */
static freq_t
_pll_get_freq(const clk_t* clk)
{
    volatile uint32_t* ctrl_reg;
    volatile uint32_t* cfg_reg;
    uint32_t status_mask;
    uint8_t fdiv;
    uint32_t fin, fout;

    status_mask = _decode_pll(clk, &ctrl_reg, &cfg_reg);
    if (status_mask == 0) {
        return 0;
    }
    assert(!(*ctrl_reg & (BIT(4) | BIT(1) | BIT(0))));

    fin = clk_get_freq(clk->parent);
    fdiv = (*ctrl_reg & PLL_CTRL_FDIV_MASK) >> PLL_CTRL_FDIV_SHIFT;
    fout = fin * fdiv;

    return fout;
}

static freq_t
_pll_set_freq(const clk_t* clk, freq_t hz)
{
    volatile uint32_t* ctrl_reg;
    volatile uint32_t* cfg_reg;
    uint32_t status_mask;
    uint32_t fin;
    uint8_t fdiv;

    fin = clk_get_freq(clk->parent);
    fdiv = fin / hz;
    fdiv = INRANGE(PLL_CTRL_FDIV_MIN, fdiv, PLL_CTRL_FDIV_MAX);

    status_mask = _decode_pll(clk, &ctrl_reg, &cfg_reg);
    if (status_mask == 0) {
        return 0;
    }

    /* Program the feedback divider value and the configuration register */
    *ctrl_reg &= ~(PLL_CTRL_FDIV_MASK);
    *ctrl_reg |= PLL_CTRL_FDIV(fdiv);
    *cfg_reg = pll_cfg_tbl[fdiv - PLL_CTRL_FDIV_MIN].pll_cfg;

    /* Force the PLL into bypass mode */
    *ctrl_reg |= PLL_CTRL_BYPASS_FORCE;

    /* Assert and de-assert the PLL reset */
    *ctrl_reg |= PLL_CTRL_RESET;
    *ctrl_reg &= ~(PLL_CTRL_RESET);

    /* Verify that the PLL is locked */
    while (!(clk_regs->pll_status & status_mask));

    /* Disable the PLL bypass mode */
    *ctrl_reg &= ~(PLL_CTRL_BYPASS_FORCE);

    return clk_get_freq(clk);
}

static void
_pll_recal(const clk_t* clk UNUSED)
{
    assert(0);
}

static clk_t*
_pll_init(clk_t* clk)
{
    if (clk->priv == NULL) {
        clk_t* parent;
        parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MASTER);
        clk_register_child(parent, clk);
        clk->priv = (void*)clk_regs;
    }

    return clk;
}

static struct clock arm_pll_clk = { CLK_OPS(ARM_PLL, pll, NULL) };
static struct clock ddr_pll_clk = { CLK_OPS(DDR_PLL, pll, NULL) };
static struct clock io_pll_clk  = { CLK_OPS(IO_PLL,  pll, NULL) };

static int
_cpu_set_621(clk_t* clk, int v)
{
    switch (clk->id) {
    case CLK_CPU_6OR4X:
    case CLK_CPU_3OR2X:
    case CLK_CPU_2X:
    case CLK_CPU_1X:
        if (v) {
            clk_regs->clk_621_true |= CPU_CLK_621_TRUE;
        } else {
            clk_regs->clk_621_true &= ~CPU_CLK_621_TRUE;
        }
        return 0;
    default:
        return -1;
    }
}

int
clk_cpu_clk_select_621(clk_t* clk)
{
    return _cpu_set_621(clk, 1);
}

int
clk_cpu_clk_select_421(clk_t* clk)
{
    return _cpu_set_621(clk, 0);
}

/* CPU Clocks */
static freq_t
_cpu_get_freq(const clk_t* clk)
{
    uint8_t clk_621_true, divisor0;
    uint32_t divisor;
    uint32_t fout, fin;

    clk_621_true = clk_regs->clk_621_true & CPU_CLK_621_TRUE;
    divisor0 = CLK_GET_DIVISOR(0, clk_regs->arm_clk_ctrl);

    switch (clk->id) {
    case CLK_CPU_6OR4X:
        divisor = divisor0;
        break;
    case CLK_CPU_3OR2X:
        divisor = divisor0 * 2;
        break;
    case CLK_CPU_2X:
        divisor = divisor0 * ((clk_621_true) ? 3 : 2);
        break;
    case CLK_CPU_1X:
        divisor = divisor0 * ((clk_621_true) ? 6 : 4);
        break;
    default:
        assert(!"Invalid clock");
        return -1;
    }

    fin = clk_get_freq(clk->parent);
    fout = fin / divisor;

    return fout;
}

static freq_t
_cpu_set_freq(const clk_t* clk, freq_t hz)
{
    uint32_t fin;
    uint8_t divisor0;

    /*
     * We only set cpu_6x4x's frequency. The other CPU clock frequencies are
     * derived from cpu_6x4x.
     */
    if (clk->id != CLK_CPU_6OR4X) {
        return -1;
    }

    fin = clk_get_freq(clk->parent);
    zynq7000_clk_calc_divs(hz, fin, &divisor0, NULL);

    /* CPU clocks must have an even divisor */
    divisor0 = zynq7000_even_divisor(divisor0);
    set_div(&clk_regs->arm_clk_ctrl, divisor0);

    return clk_get_freq(clk);
}

static void
_cpu_recal(const clk_t* clk UNUSED)
{
    assert(0);
}

static clk_t*
_cpu_init(clk_t* clk)
{
    if (clk->priv == NULL) {
        clk_t* parent;
        enum clk_id parent_id;
        parent_id = cpu_clk_src[CLK_GET_SRCSEL(clk_regs->arm_clk_ctrl)];
        parent = clk_get_clock(clk_get_clock_sys(clk), parent_id);
        clk_register_child(parent, clk);
        clk->priv = (void*)clk_regs;
    }

    return clk;
}

static struct clock cpu_6or4x_clk = { CLK_OPS(CPU_6OR4X, cpu, NULL) };
static struct clock cpu_3or2x_clk = { CLK_OPS(CPU_3OR2X, cpu, NULL) };
static struct clock cpu_2x_clk    = { CLK_OPS(CPU_2X,    cpu, NULL) };
static struct clock cpu_1x_clk    = { CLK_OPS(CPU_1X,    cpu, NULL) };

/* DDR Clocks */
static freq_t
_ddr_get_freq(const clk_t* clk)
{
    uint8_t divisor0, divisor1;
    uint32_t fout, fin;

    switch (clk->id) {
    case CLK_DDR_2X:
        divisor0 = DDR_CLK_GET_DIVISOR(2);
        divisor1 = 1;
        break;
    case CLK_DDR_3X:
        divisor0 = DDR_CLK_GET_DIVISOR(3);
        divisor1 = 1;
        break;
    case CLK_DCI:
        divisor0 = CLK_GET_DIVISOR(0, clk_regs->dci_clk_ctrl);
        divisor1 = CLK_GET_DIVISOR(1, clk_regs->dci_clk_ctrl);
        break;
    default:
        assert(!"Invalid clock");
        return -1;
    }

    fin = clk_get_freq(clk->parent);
    fout = fin / (divisor0 * divisor1);

    return fout;
}

static freq_t
_ddr_set_freq(const clk_t* clk, freq_t hz)
{
    uint32_t fin;
    uint8_t divisor0, divisor1;

    fin = clk_get_freq(clk->parent);

    switch (clk->id) {
    case CLK_DDR_2X:
        zynq7000_clk_calc_divs(hz, fin, &divisor0, NULL);
        DDR_CLK_SET_DIVISOR(2, divisor0);
        break;
    case CLK_DDR_3X:
        zynq7000_clk_calc_divs(hz, fin, &divisor0, NULL);

        /* DDR_3XCLK must have an even divisor */
        divisor0 = zynq7000_even_divisor(divisor0);

        DDR_CLK_SET_DIVISOR(3, divisor0);
        break;
    case CLK_DCI:
        zynq7000_clk_calc_divs(hz, fin, &divisor0, &divisor1);
        set_divs(&clk_regs->dci_clk_ctrl, divisor0, divisor1);
        break;
    default:
        assert(!"Invalid clock");
        return -1;
    }

    return clk_get_freq(clk);
}

static void
_ddr_recal(const clk_t* clk UNUSED)
{
    assert(0);
}

static clk_t*
_ddr_init(clk_t* clk)
{
    if (clk->priv == NULL) {
        clk_t* parent;
        parent = clk_get_clock(clk_get_clock_sys(clk), CLK_DDR_PLL);
        clk_register_child(parent, clk);
        clk->priv = (void*)clk_regs;
    }

    return clk;
}

static struct clock ddr_2x_clk = { CLK_OPS(DDR_2X, ddr, NULL) };
static struct clock ddr_3x_clk = { CLK_OPS(DDR_3X, ddr, NULL) };
static struct clock dci_clk    = { CLK_OPS(DCI,    ddr, NULL) };

/* I/O Peripheral Clocks */
static freq_t
_aper_get_freq(const clk_t* clk)
{
    enum clk_id id;
    uint8_t divisor0, divisor1;
    uint32_t fout, fin;

    id = clk->id;

    switch (id) {
        /* One divider clocks */
    case CLK_SMC:
        divisor0 = CLK_GET_DIVISOR(0, clk_regs->smc_clk_ctrl);
        divisor1 = 1;
        break;
    case CLK_LQSPI:
        divisor0 = CLK_GET_DIVISOR(0, clk_regs->lqspi_clk_ctrl);
        divisor1 = 1;
        break;
    case CLK_SDIO0:
    case CLK_SDIO1:
        divisor0 = CLK_GET_DIVISOR(0, clk_regs->sdio_clk_ctrl);
        divisor1 = 1;
        break;
    case CLK_UART0:
    case CLK_UART1:
        divisor0 = CLK_GET_DIVISOR(0, clk_regs->uart_clk_ctrl);
        divisor1 = 1;
        break;
    case CLK_SPI0:
    case CLK_SPI1:
        divisor0 = CLK_GET_DIVISOR(0, clk_regs->spi_clk_ctrl);
        divisor1 = 1;
        break;
        /* Two divider clocks */
    case CLK_GEM0:
        divisor0 = CLK_GET_DIVISOR(0, clk_regs->gem0_clk_ctrl);
        divisor1 = CLK_GET_DIVISOR(1, clk_regs->gem0_clk_ctrl);
        break;
    case CLK_GEM1:
        divisor0 = CLK_GET_DIVISOR(0, clk_regs->gem1_clk_ctrl);
        divisor1 = CLK_GET_DIVISOR(1, clk_regs->gem1_clk_ctrl);
        break;
    case CLK_CAN0:
    case CLK_CAN1:
        divisor0 = CLK_GET_DIVISOR(0, clk_regs->can_clk_ctrl);
        divisor1 = CLK_GET_DIVISOR(1, clk_regs->can_clk_ctrl);
        break;
    default:
        assert(!"Invalid clock");
        return -1;
    }

    fin = clk_get_freq(clk->parent);
    if (divisor0 == 0 || divisor1 == 0) {
        fout = 0;
    } else {
        fout = fin / (divisor0 * divisor1);
    }
    return fout;
}

static freq_t
_aper_set_freq(const clk_t* clk, freq_t hz)
{
    uint32_t fin;
    uint8_t divisor0, divisor1;

    fin = clk_get_freq(clk->parent);

    switch (clk->id) {
        /* One divider clocks */
    case CLK_SMC:
        zynq7000_clk_calc_divs(hz, fin, &divisor0, NULL);
        set_div(&clk_regs->smc_clk_ctrl, divisor0);
        break;
    case CLK_LQSPI:
        zynq7000_clk_calc_divs(hz, fin, &divisor0, NULL);
        set_div(&clk_regs->lqspi_clk_ctrl, divisor0);
        break;
    case CLK_SDIO0:
    case CLK_SDIO1:
        zynq7000_clk_calc_divs(hz, fin, &divisor0, NULL);
        set_div(&clk_regs->sdio_clk_ctrl, divisor0);
        break;
    case CLK_UART0:
    case CLK_UART1:
        zynq7000_clk_calc_divs(hz, fin, &divisor0, NULL);
        set_div(&clk_regs->uart_clk_ctrl, divisor0);
        break;
    case CLK_SPI0:
    case CLK_SPI1:
        zynq7000_clk_calc_divs(hz, fin, &divisor0, NULL);
        set_div(&clk_regs->spi_clk_ctrl, divisor0);
        break;
        /* Two divider clocks */
    case CLK_GEM0:
        zynq7000_clk_calc_divs(hz, fin, &divisor0, &divisor1);
        set_divs(&clk_regs->gem0_clk_ctrl, divisor0, divisor1);
        break;
    case CLK_GEM1:
        zynq7000_clk_calc_divs(hz, fin, &divisor0, &divisor1);
        set_divs(&clk_regs->gem1_clk_ctrl, divisor0, divisor1);
        break;
    case CLK_CAN0:
    case CLK_CAN1:
        zynq7000_clk_calc_divs(hz, fin, &divisor0, &divisor1);
        set_divs(&clk_regs->can_clk_ctrl, divisor0, divisor1);
        break;
    default:
        assert(!"Invalid clock");
        return -1;
    }

    return clk_get_freq(clk);
}

static void
_aper_recal(const clk_t* clk UNUSED)
{
    assert(0);
}

static clk_t*
_aper_init(clk_t* clk)
{
    if (clk->priv == NULL) {
        clk_t* parent;
        parent = clk_get_clock(clk_get_clock_sys(clk), CLK_IO_PLL);
        clk_register_child(parent, clk);
        clk->priv = (void*)clk_regs;
    }

    return clk;
}

static struct clock smc_clk   = { CLK_OPS(SMC,   aper, NULL) };
static struct clock lqspi_clk = { CLK_OPS(LQSPI, aper, NULL) };
static struct clock gem0_clk  = { CLK_OPS(GEM0,  aper, NULL) };
static struct clock gem1_clk  = { CLK_OPS(GEM1,  aper, NULL) };
static struct clock sdio0_clk = { CLK_OPS(SDIO0, aper, NULL) };
static struct clock sdio1_clk = { CLK_OPS(SDIO1, aper, NULL) };
static struct clock uart0_clk = { CLK_OPS(UART0, aper, NULL) };
static struct clock uart1_clk = { CLK_OPS(UART1, aper, NULL) };
static struct clock spi0_clk  = { CLK_OPS(SPI0,  aper, NULL) };
static struct clock spi1_clk  = { CLK_OPS(SPI1,  aper, NULL) };
static struct clock can0_clk  = { CLK_OPS(CAN0,  aper, NULL) };
static struct clock can1_clk  = { CLK_OPS(CAN1,  aper, NULL) };

static inline pl_clk_regs_t*
get_pl_clk_regs(const clk_t* clk)
{
    switch (clk->id) {
    case CLK_FPGA_PL0:
        return &clk_regs->fpga_clk[0];
    case CLK_FPGA_PL1:
        return &clk_regs->fpga_clk[1];
    case CLK_FPGA_PL2:
        return &clk_regs->fpga_clk[2];
    case CLK_FPGA_PL3:
        return &clk_regs->fpga_clk[3];
    default:
        return NULL;
    }
}

/* FPGA PL Clocks */
static freq_t
_fpga_get_freq(const clk_t* clk)
{
    pl_clk_regs_t* regs = (pl_clk_regs_t*)clk->priv;
    uint8_t div0, div1;
    freq_t fin;
    div0 = CLK_GET_DIVISOR(0, regs->clk_ctrl);
    div1 = CLK_GET_DIVISOR(1, regs->clk_ctrl);
    fin = clk_get_freq(clk->parent);
    return fin / div0 / div1;
}

static freq_t
_fpga_set_freq(const clk_t* clk, freq_t hz)
{
    pl_clk_regs_t* regs = (pl_clk_regs_t*)clk->priv;
    uint8_t div0, div1;
    freq_t fin;

    fin = clk_get_freq(clk->parent);
    zynq7000_clk_calc_divs(hz, fin, &div0, &div1);
    set_divs(&regs->clk_ctrl, div0, div1);

    return clk_get_freq(clk);
}

static void
_fpga_recal(const clk_t* clk UNUSED)
{
    assert(0);
}

static clk_t*
_fpga_init(clk_t* clk)
{
    if (clk->priv == NULL) {
        pl_clk_regs_t* regs;
        clk_t* parent;
        enum clk_id parent_id;
        regs = get_pl_clk_regs(clk);
        parent_id = fpga_clk_src[CLK_GET_SRCSEL(regs->clk_ctrl)];
        parent = clk_get_clock(clk_get_clock_sys(clk), parent_id);
        clk_register_child(parent, clk);
        clk->priv = (void*)regs;
        /* Continuous clock */
        regs->thr_ctrl = 0;
    }

    return clk;
}

static struct clock fpga_pl0_clk = { CLK_OPS(FPGA_PL0, fpga, NULL) };
static struct clock fpga_pl1_clk = { CLK_OPS(FPGA_PL1, fpga, NULL) };
static struct clock fpga_pl2_clk = { CLK_OPS(FPGA_PL2, fpga, NULL) };
static struct clock fpga_pl3_clk = { CLK_OPS(FPGA_PL3, fpga, NULL) };

/*** These clocks yet to be implemented ***/
static struct clock pcap_clk = { CLK_OPS_DEFAULT(PCAP) };
static struct clock dbg_clk  = { CLK_OPS_DEFAULT(DBG ) };

static int
zynq7000_gate_enable(const clock_sys_t* clock_sys, enum clock_gate gate,
                     enum clock_gate_mode mode)
{
    uint32_t aper_clk_ctrl;

    assert(clk_regs);
    assert(mode == CLKGATE_ON);
    assert(gate >= 0);
    assert(gate < 32);

    aper_clk_ctrl = clk_regs->aper_clk_ctrl;
    aper_clk_ctrl |= BIT(gate);
    clk_regs->aper_clk_ctrl = aper_clk_ctrl;

    return 0;
}

int
clock_sys_init(const ps_io_ops_t* o, clock_sys_t* clock_sys)
{
    src_dev_t slcr;
    int err;
    assert(sizeof(struct zynq7000_clk_regs) == 0x208);
    /* Grab a handle to the clock registers */
    err = reset_controller_init(SLCR, o, &slcr);
    if (err) {
        return err;
    }
    clk_regs = (volatile struct zynq7000_clk_regs*)reset_controller_get_clock_regs(&slcr);
    assert(clk_regs);

    /* Initialise the clock subsystem structure */
    clock_sys->priv = (void*)clk_regs;
    clock_sys->get_clock = &ps_get_clock;
    clock_sys->gate_enable = &zynq7000_gate_enable;
    return 0;
}

void
clk_print_clock_tree(const clock_sys_t* sys)
{
    clk_t *clk = clk_get_clock(sys, CLK_MASTER);
    clk_print_tree(clk, "");
}

clk_t* ps_clocks[] = {
    [CLK_MASTER]    = &master_clk,
    [CLK_ARM_PLL]   = &arm_pll_clk,
    [CLK_DDR_PLL]   = &ddr_pll_clk,
    [CLK_IO_PLL]    = &io_pll_clk,
    [CLK_CPU_6OR4X] = &cpu_6or4x_clk,
    [CLK_CPU_3OR2X] = &cpu_3or2x_clk,
    [CLK_CPU_2X]    = &cpu_2x_clk,
    [CLK_CPU_1X]    = &cpu_1x_clk,
    [CLK_DDR_2X]    = &ddr_2x_clk,
    [CLK_DDR_3X]    = &ddr_3x_clk,
    [CLK_DCI]       = &dci_clk,
    [CLK_SMC]       = &smc_clk,
    [CLK_LQSPI]     = &lqspi_clk,
    [CLK_GEM0]      = &gem0_clk,
    [CLK_GEM1]      = &gem1_clk,
    [CLK_SDIO0]     = &sdio0_clk,
    [CLK_SDIO1]     = &sdio1_clk,
    [CLK_UART0]     = &uart0_clk,
    [CLK_UART1]     = &uart1_clk,
    [CLK_SPI0]      = &spi0_clk,
    [CLK_SPI1]      = &spi1_clk,
    [CLK_CAN0]      = &can0_clk,
    [CLK_CAN1]      = &can1_clk,
    [CLK_DBG]       = &dbg_clk,
    [CLK_PCAP]      = &pcap_clk,
    [CLK_FPGA_PL0]  = &fpga_pl0_clk,
    [CLK_FPGA_PL1]  = &fpga_pl1_clk,
    [CLK_FPGA_PL2]  = &fpga_pl2_clk,
    [CLK_FPGA_PL3]  = &fpga_pl3_clk,
};

freq_t ps_freq_default[] = {
    [CLK_MASTER]    = 33333 * KHZ, /* PS_CLK frequency = 33.33 MHz */
    [CLK_ARM_PLL]   =  1333 * MHZ,
    [CLK_DDR_PLL]   =  1067 * MHZ,
    [CLK_IO_PLL]    =  1000 * MHZ,
    [CLK_CPU_6OR4X] =   667 * MHZ,
    [CLK_CPU_3OR2X] =   333 * MHZ,
    [CLK_CPU_2X]    =   222 * MHZ,
    [CLK_CPU_1X]    =   111 * MHZ,
    [CLK_DDR_2X]    =   356 * MHZ,
    [CLK_DDR_3X]    =   533 * MHZ,
    [CLK_DCI]       =    10 * MHZ,
    [CLK_SMC]       =   100 * MHZ,
    [CLK_LQSPI]     =   200 * MHZ,
    [CLK_GEM0]      =   125 * MHZ,
    [CLK_GEM1]      =   125 * MHZ,
    [CLK_SDIO0]     =   100 * MHZ,
    [CLK_SDIO1]     =   100 * MHZ,
    [CLK_UART0]     =    25 * MHZ,
    [CLK_UART1]     =    25 * MHZ,
    [CLK_SPI0]      =   200 * MHZ,
    [CLK_SPI1]      =   200 * MHZ,
    [CLK_CAN0]      =   100 * MHZ,
    [CLK_CAN1]      =   100 * MHZ,
    [CLK_PCAP]      =   200 * MHZ,
    [CLK_DBG]       =   100 * MHZ,
    [CLK_FPGA_PL0]  =    50 * MHZ,
    [CLK_FPGA_PL1]  =    50 * MHZ,
    [CLK_FPGA_PL2]  =    50 * MHZ,
    [CLK_FPGA_PL3]  =    50 * MHZ,
};
