/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include "../../arch/arm/clock.h"
#include "../../mach/exynos/clock.h"
#include "../../services.h"
#include <assert.h>
#include <string.h>
#include <utils/util.h>


#define EXYNOS5_CMU_CPU_PADDR   0x10010000
#define EXYNOS5_CMU_CORE_PADDR  0x10014000
#define EXYNOS5_CMU_ACP_PADDR   0x10018000
#define EXYNOS5_CMU_ISP_PADDR   0x1001C000
#define EXYNOS5_CMU_TOP_PADDR   0x10020000
#define EXYNOS5_CMU_LEX_PADDR   0x10024000
#define EXYNOS5_CMU_R0X_PADDR   0x10028000
#define EXYNOS5_CMU_R1X_PADDR   0x1002C000
#define EXYNOS5_CMU_CDREX_PADDR 0x10030000

#define EXYNOS5_CMU_SIZE       0x1000
#define EXYNOS5_CMU_CPU_SIZE    EXYNOS5_CMU_SIZE
#define EXYNOS5_CMU_CORE_SIZE   EXYNOS5_CMU_SIZE
#define EXYNOS5_CMU_ACP_SIZE    EXYNOS5_CMU_SIZE
#define EXYNOS5_CMU_ISP_SIZE    EXYNOS5_CMU_SIZE
#define EXYNOS5_CMU_TOP_SIZE    EXYNOS5_CMU_SIZE
#define EXYNOS5_CMU_LEX_SIZE    EXYNOS5_CMU_SIZE
#define EXYNOS5_CMU_R0X_SIZE    EXYNOS5_CMU_SIZE
#define EXYNOS5_CMU_R1X_SIZE    EXYNOS5_CMU_SIZE
#define EXYNOS5_CMU_CDREX_SIZE  EXYNOS5_CMU_SIZE


#define CLKID_UART0      CLKID(TOP, 23, 0)
#define CLKID_UART1      CLKID(TOP, 23, 1)
#define CLKID_UART2      CLKID(TOP, 23, 2)
#define CLKID_UART3      CLKID(TOP, 23, 3)
#define CLKID_PWM        CLKID(TOP, 23, 4)
#define CLKID_SPI0       CLKID(TOP, 24, 4)
#define CLKID_SPI1       CLKID(TOP, 24, 5)
#define CLKID_SPI2       CLKID(TOP, 24, 6)
#define CLKID_SPI0_ISP   CLKID(TOP, 28, 0)
#define CLKID_SPI1_ISP   CLKID(TOP, 28, 1)

enum clkregs {
    CLKREGS_CPU,
    CLKREGS_CORE,
    CLKREGS_ACP,
    CLKREGS_ISP,
    CLKREGS_TOP,
    CLKREGS_LEX,
    CLKREGS_R0X,
    CLKREGS_R1X,
    CLKREGS_CDREX,
    NCLKREGS
};

/* Available clock sources for the peripheral block */
static enum clk_id clk_src_peri_blk[] = {
                                            CLK_MASTER,
                                            CLK_MASTER,
                                            -1, /* CLK_HDMI27M   */
                                            -1, /* CLK_DPTXPHY   */
                                            -1, /* CLK_UHOSTPHY  */
                                            -1, /* CLK_HDMIPHY   */
                                            -1, /* CLK_MPLL_USER */
                                            -1, /* CLK_EPLL      */
                                            -1, /* CLK_VPLL      */
                                            -1  /* CLK_CPLL      */
                                        };


volatile struct clk_regs* _clk_regs[NCLKREGS];

static struct clock master_clk = { CLK_OPS(MASTER, default_clk, NULL) };

/* The SPI div register is a special case as we have 2 dividers, one of which
 * is 2 nibbles wide */
static freq_t
_spi_get_freq(clk_t* clk)
{
    freq_t fin;
    int clkid;
    int rpre, r;
    clkid = exynos_clk_get_priv_id(clk);
    switch (clk->id) {
    case CLK_SPI0:
    case CLK_SPI0_ISP:
    case CLK_SPI1_ISP:
        break;
    case CLK_SPI1:
        clkid += 3;
        break;
    case CLK_SPI2:
        clkid += 6;
        break;
    default:
        return 0;
    }
    r = exynos_cmu_get_div(_clk_regs, clkid, 1);
    rpre = exynos_cmu_get_div(_clk_regs, clkid + 3, 2);
    fin = clk_get_freq(clk->parent);
    return fin / (r + 1) / (rpre + 1);
}

static freq_t
_spi_set_freq(clk_t* clk, freq_t hz)
{
    freq_t fin;
    int clkid;
    int rpre, r;
    clkid = exynos_clk_get_priv_id(clk);
    switch (clk->id) {
    case CLK_SPI0:
    case CLK_SPI0_ISP:
    case CLK_SPI1_ISP:
        break;
    case CLK_SPI1:
        clkid += 3;
        break;
    case CLK_SPI2:
        clkid += 6;
        break;
    default:
        return 0;
    }
    fin = clk_get_freq(clk->parent);
    r = fin / 0xff / hz;
    rpre = fin / (r + 1) / hz;
    exynos_cmu_set_div(_clk_regs, clkid, 1, r);
    exynos_cmu_set_div(_clk_regs, clkid + 3, 2, rpre);
    return clk_get_freq(clk);
}

static void
_spi_recal(clk_t* clk)
{
    assert(!"Not implemented");
}

static clk_t*
_spi_init(clk_t* clk)
{
    /* MUX -> DIVspix -> DIVspipre */
    clk_t* parent;
    int clkid;
    int src;
    clkid = exynos_clk_get_priv_id(clk);
    src = exynos_cmu_get_src(_clk_regs, clkid);
    assert(src < ARRAY_SIZE(clk_src_peri_blk) && src >= 0);
    assert(clk_src_peri_blk[src] != -1);
    parent = clk_get_clock(clk->clk_sys, clk_src_peri_blk[src]);
    assert(parent);
    clk_init(parent);
    clk_register_child(parent, clk);
    return clk;
}

static struct clock spi0_clk     = { CLK_OPS(SPI0    , spi, CLKID_SPI0    ) };
static struct clock spi1_clk     = { CLK_OPS(SPI1    , spi, CLKID_SPI1    ) };
static struct clock spi2_clk     = { CLK_OPS(SPI2    , spi, CLKID_SPI2    ) };
static struct clock spi0_isp_clk = { CLK_OPS(SPI0_ISP, spi, CLKID_SPI0_ISP) };
static struct clock spi1_isp_clk = { CLK_OPS(SPI1_ISP, spi, CLKID_SPI1_ISP) };



static freq_t
_peric_clk_get_freq(clk_t* clk)
{
    freq_t fin;
    int clkid;
    int div;
    clkid = exynos_clk_get_priv_id(clk);
    div = exynos_cmu_get_div(_clk_regs, clkid, 1);
    fin = clk_get_freq(clk->parent);
    return fin / (div + 1);
}

static freq_t
_peric_clk_set_freq(clk_t* clk, freq_t hz)
{
    freq_t fin;
    int clkid;
    int div;
    fin = clk_get_freq(clk->parent);
    clkid = exynos_clk_get_priv_id(clk);
    div = fin / hz;
    exynos_cmu_set_div(_clk_regs, clkid, 1, div);
    return clk_get_freq(clk);
}

static void
_peric_clk_recal(clk_t* clk)
{
    assert(!"Not implemented");
}

static clk_t*
_peric_clk_init(clk_t* clk)
{
    /* MUX -> DIVuartx -> DIVuartpre */
    clk_t* parent;
    int clkid;
    int src;
    clkid = exynos_clk_get_priv_id(clk);
    src = exynos_cmu_get_src(_clk_regs, clkid);
    assert(src < ARRAY_SIZE(clk_src_peri_blk) && src >= 0);
    assert(clk_src_peri_blk[src] != -1);
    parent = clk_get_clock(clk->clk_sys, clk_src_peri_blk[src]);
    assert(parent);
    clk_init(parent);
    clk_register_child(parent, clk);
    return clk;
}

static struct clock uart0_clk = { CLK_OPS(UART0, peric_clk, CLKID_UART0) };
static struct clock uart1_clk = { CLK_OPS(UART1, peric_clk, CLKID_UART1) };
static struct clock uart2_clk = { CLK_OPS(UART2, peric_clk, CLKID_UART2) };
static struct clock uart3_clk = { CLK_OPS(UART3, peric_clk, CLKID_UART3) };
static struct clock pwm_clk   = { CLK_OPS(PWM  , peric_clk, CLKID_PWM  ) };

static int
exynos5_gate_enable(clock_sys_t* clock_sys, enum clock_gate gate, enum clock_gate_mode mode)
{
    return -1;
}

static int
clock_sys_common_init(clock_sys_t* clock_sys)
{
    clock_sys->priv = (void*)&_clk_regs;
    clock_sys->get_clock = &ps_get_clock;
    clock_sys->gate_enable = &exynos5_gate_enable;
    return 0;
}

int
exynos5_clock_sys_init(void* cpu, void* core, void* acp, void* isp, void* top,
                       void* lex, void* r0x,  void* r1x, void* cdrex,
                       clock_sys_t* clock_sys)
{
    if (cpu) {
        _clk_regs[CLKREGS_CPU]   = cpu;
    }
    if (core) {
        _clk_regs[CLKREGS_CORE]  = core;
    }
    if (acp) {
        _clk_regs[CLKREGS_ACP]   = acp;
    }
    if (isp) {
        _clk_regs[CLKREGS_ISP]   = isp;
    }
    if (top) {
        _clk_regs[CLKREGS_TOP]   = top;
    }
    if (lex) {
        _clk_regs[CLKREGS_LEX]   = lex;
    }
    if (r0x) {
        _clk_regs[CLKREGS_R0X]   = r0x;
    }
    if (r1x) {
        _clk_regs[CLKREGS_R1X]   = r1x;
    }
    if (cdrex) {
        _clk_regs[CLKREGS_CDREX] = cdrex;
    }
    return clock_sys_common_init(clock_sys);
}

int
clock_sys_init(ps_io_ops_t* o, clock_sys_t* clock_sys)
{
    MAP_IF_NULL(o, EXYNOS5_CMU_CPU,   _clk_regs[CLKREGS_CPU]);
    MAP_IF_NULL(o, EXYNOS5_CMU_CORE,  _clk_regs[CLKREGS_CORE]);
    MAP_IF_NULL(o, EXYNOS5_CMU_ACP,   _clk_regs[CLKREGS_ACP]);
    MAP_IF_NULL(o, EXYNOS5_CMU_ISP,   _clk_regs[CLKREGS_ISP]);
    MAP_IF_NULL(o, EXYNOS5_CMU_TOP,   _clk_regs[CLKREGS_TOP]);
    MAP_IF_NULL(o, EXYNOS5_CMU_LEX,   _clk_regs[CLKREGS_LEX]);
    MAP_IF_NULL(o, EXYNOS5_CMU_R0X,   _clk_regs[CLKREGS_R0X]);
    MAP_IF_NULL(o, EXYNOS5_CMU_R1X,   _clk_regs[CLKREGS_R1X]);
    MAP_IF_NULL(o, EXYNOS5_CMU_CDREX, _clk_regs[CLKREGS_CDREX]);
    return clock_sys_common_init(clock_sys);
}


void
clk_print_clock_tree(clock_sys_t* sys)
{
    (void)sys;
    clk_t* clk = ps_clocks[CLK_MASTER];
    clk_print_tree(clk, "");
}


clk_t* ps_clocks[] = {
    [CLK_MASTER  ]   = &master_clk,
    [CLK_SPI0    ]   = &spi0_clk,
    [CLK_SPI1    ]   = &spi1_clk,
    [CLK_SPI2    ]   = &spi2_clk,
    [CLK_SPI0_ISP]   = &spi0_isp_clk,
    [CLK_SPI1_ISP]   = &spi1_isp_clk,
    [CLK_UART0   ]   = &uart0_clk,
    [CLK_UART1   ]   = &uart1_clk,
    [CLK_UART2   ]   = &uart2_clk,
    [CLK_UART3   ]   = &uart3_clk,
    [CLK_PWM     ]   = &pwm_clk,
};

/* These frequencies are NOT the recommended
 * frequencies. They are to be used when we
 * need to make assumptions about what u-boot
 * has left us with. */
freq_t ps_freq_default[] = {
    [CLK_MASTER  ]   = 24 * MHZ,
    [CLK_SPI0    ]   = 24 * MHZ,
    [CLK_SPI1    ]   = 24 * MHZ,
    [CLK_SPI2    ]   = 24 * MHZ,
    [CLK_SPI0_ISP]   = 24 * MHZ,
    [CLK_SPI1_ISP]   = 24 * MHZ,
    [CLK_UART0   ]   = 24 * MHZ,
    [CLK_UART1   ]   = 24 * MHZ,
    [CLK_UART2   ]   = 24 * MHZ,
    [CLK_UART3   ]   = 24 * MHZ,
    [CLK_PWM     ]   = 24 * MHZ,
};



