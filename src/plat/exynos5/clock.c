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
#define EXYNOS5_CMU_MEM_PADDR   0x10038000

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
#define EXYNOS5_CMU_MEM_SIZE    EXYNOS5_CMU_SIZE

#define OFFSET_SCLKCPLL    (0x20 / 4)
#define OFFSET_SCLKEPLL    (0x30 / 4)
#define OFFSET_SCLKVPLL    (0x40 / 4)
#define OFFSET_SCLKGPLL    (0x50 / 4)
#define OFFSET_SCLKMPLL    (0x00 / 4)
#define OFFSET_SCLKBPLL    (0x10 / 4)

#define CLKID_SCLKCPLL     CLKID(TOP, 6, 2)
#define CLKID_SCLKEPLL     CLKID(TOP, 6, 3)
#define CLKID_SCLKVPLL     CLKID(TOP, 6, 4)
#define CLKID_SCLKGPLL     CLKID(TOP, 6, 7)
#define CLKID_SCLKMPLL     CLKID(CORE, 0, 2)
#define CLKID_SCLKBPLL     CLKID(CDREX, 0, 0)

#define CLKID_UART0      CLKID(TOP, 20, 0)
#define CLKID_UART1      CLKID(TOP, 20, 1)
#define CLKID_UART2      CLKID(TOP, 20, 2)
#define CLKID_UART3      CLKID(TOP, 20, 3)
#define CLKID_PWM        CLKID(TOP, 20, 5)
#define CLKID_SPI0       CLKID(TOP, 21, 4)
#define CLKID_SPI1       CLKID(TOP, 21, 5)
#define CLKID_SPI2       CLKID(TOP, 21, 6)
#define CLKID_SPI0_ISP   CLKID(TOP, 28, 0)
#define CLKID_SPI1_ISP   CLKID(TOP, 28, 1)

/* Available clock sources for the peripheral block */
static enum clk_id clk_src_peri_blk[] = {
                                            CLK_MASTER,
                                            CLK_MASTER,
                                            -1, /* CLK_HDMI27M   */
                                            -1, /* CLK_DPTXPHY   */
                                            -1, /* CLK_UHOSTPHY  */
                                            -1, /* CLK_HDMIPHY   */
                                            CLK_SCLKMPLL,
                                            CLK_SCLKEPLL,
                                            CLK_SCLKVPLL,
                                            CLK_SCLKCPLL
                                        };


volatile struct clk_regs* _clk_regs[NCLKREGS];

static struct clock master_clk = { CLK_OPS(MASTER, default_clk, NULL) };


static struct pms_tbl _ambcgpll_tbl[] = {
    {  200, PLL_PMS( 3, 100, 2) },
    {  333, PLL_PMS( 4, 222, 2) },
    {  400, PLL_PMS( 3, 100, 1) },
    {  533, PLL_PMS(12, 533, 1) },
    {  600, PLL_PMS( 4, 200, 1) },
    {  667, PLL_PMS( 7, 389, 1) },
    {  800, PLL_PMS( 3, 100, 0) },
    { 1000, PLL_PMS( 3, 125, 0) },
    { 1066, PLL_PMS(12, 533, 0) },
    { 1200, PLL_PMS( 3, 150, 0) },
    { 1400, PLL_PMS( 3, 175, 0) },
    { 1600, PLL_PMS( 3, 200, 0) }
};

static struct pms_tbl _epll_tbl[] = {
    {  33, PLL_PMSK(3, 131, 5,  4719) },
    {  45, PLL_PMSK(3,  90, 4, 20762) },
    {  48, PLL_PMSK(2,  64, 4,     0) },
    {  49, PLL_PMSK(3,  98, 4, 19923) },
    {  50, PLL_PMSK(2,  67, 4, 43691) },
    {  68, PLL_PMSK(2,  90, 4, 20762) },
    {  74, PLL_PMSK(2,  98, 4, 19923) },
    {  80, PLL_PMSK(2, 107, 4, 43691) },
    {  84, PLL_PMSK(2, 112, 4,     0) },
    {  96, PLL_PMSK(2,  64, 3,     0) },
    { 144, PLL_PMSK(2,  96, 3,     0) },
    { 192, PLL_PMSK(2,  64, 2,     0) },
    { 288, PLL_PMSK(2,  96, 2,     0) },
};

static struct pms_tbl _vpll_tbl[] = {
    {  27, PLL_PMSK(2,  72, 5,     0) },
    {  54, PLL_PMSK(2,  72, 4,     0) },
    {  74, PLL_PMSK(2,  99, 4,     0) },
    { 108, PLL_PMSK(2,  72, 3,     0) },
    { 148, PLL_PMSK(2,  99, 3, 59070) },
    { 149, PLL_PMSK(2,  99, 3,     0) },
    { 223, PLL_PMSK(2,  74, 2, 16384) },
    { 300, PLL_PMSK(2, 100, 2,     0) },
    { 320, PLL_PMSK(2, 107, 2, 43691) },
    { 330, PLL_PMSK(2, 110, 2,     0) },
    { 333, PLL_PMSK(2, 111, 2,     0) },
    { 335, PLL_PMSK(2, 112, 2, 43691) },
    { 371, PLL_PMSK(2,  62, 1, 53292) },
    { 445, PLL_PMSK(3, 111, 1, 17285) },
    { 446, PLL_PMSK(2,  74, 1, 16384) },
    { 519, PLL_PMSK(3, 130, 1, 52937) },
    { 600, PLL_PMSK(2, 100, 1,     0) },
};




static struct pll_priv sclkmpll_priv = PLL_PRIV(SCLKMPLL, PMS, _ambcgpll_tbl);
static struct pll_priv sclkbpll_priv = PLL_PRIV(SCLKBPLL, PMS, _ambcgpll_tbl);
static struct pll_priv sclkcpll_priv = PLL_PRIV(SCLKCPLL, PMS, _ambcgpll_tbl);
static struct pll_priv sclkgpll_priv = PLL_PRIV(SCLKGPLL, PMS, _ambcgpll_tbl);

static struct pll_priv sclkepll_priv = PLL_PRIV(SCLKEPLL, PMSK, _epll_tbl);
static struct pll_priv sclkvpll_priv = PLL_PRIV(SCLKVPLL, PMSK, _vpll_tbl);

static struct clock sclkmpll_clk  = { CLK_OPS(SCLKMPLL, pll, &sclkmpll_priv) };
static struct clock sclkbpll_clk  = { CLK_OPS(SCLKBPLL, pll, &sclkbpll_priv) };
static struct clock sclkcpll_clk  = { CLK_OPS(SCLKCPLL, pll, &sclkcpll_priv) };
static struct clock sclkgpll_clk  = { CLK_OPS(SCLKGPLL, pll, &sclkgpll_priv) };
static struct clock sclkepll_clk  = { CLK_OPS(SCLKEPLL, pll, &sclkepll_priv) };
static struct clock sclkvpll_clk  = { CLK_OPS(SCLKVPLL, pll, &sclkvpll_priv) };




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
    case CLK_SPI0_ISP:
        clkid += 32;
        break;
    case CLK_SPI1_ISP:
        clkid += 35;
        break;
    case CLK_SPI0:
        clkid += 12;
        break;
    case CLK_SPI1:
        clkid += 15;
        break;
    case CLK_SPI2:
        clkid += 18;
        break;
    default:
        return 0;
    }
    r = exynos_cmu_get_div(_clk_regs, clkid, 1);
    rpre = exynos_cmu_get_div(_clk_regs, clkid + 2, 2);
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
    case CLK_SPI0_ISP:
        clkid += 32;
        break;
    case CLK_SPI1_ISP:
        clkid += 35;
        break;
    case CLK_SPI0:
        clkid += 12;
        break;
    case CLK_SPI1:
        clkid += 15;
        break;
    case CLK_SPI2:
        clkid += 18;
        break;
    default:
        return 0;
    }
    fin = clk_get_freq(clk->parent);
    rpre = fin / 0xf / hz;
    r = fin / (rpre + 1) / hz;
    exynos_cmu_set_div(_clk_regs, clkid, 1, r);
    exynos_cmu_set_div(_clk_regs, clkid + 2, 2, rpre);
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
    /* The DIV register has an additional 2 word offset */
    clkid += 16;
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
    div = fin / hz;
    clkid = exynos_clk_get_priv_id(clk);
    /* The DIV register has an additional 2 word offset */
    clkid += 16;
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
                       void* lex, void* r0x,  void* r1x, void* cdrex, void* mem,
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
    if (mem) {
        _clk_regs[CLKREGS_MEM] = mem;
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
    [CLK_SCLKMPLL]   = &sclkmpll_clk,
    [CLK_SCLKBPLL]   = &sclkbpll_clk,
    [CLK_SCLKCPLL]   = &sclkcpll_clk,
    [CLK_SCLKGPLL]   = &sclkgpll_clk,
    [CLK_SCLKEPLL]   = &sclkepll_clk,
    [CLK_SCLKVPLL]   = &sclkvpll_clk
};

/* These frequencies are NOT the recommended
 * frequencies. They are to be used when we
 * need to make assumptions about what u-boot
 * has left us with. */
freq_t ps_freq_default[] = {
    [CLK_MASTER  ]   =    24 * MHZ,
    [CLK_SCLKVPLL]   =    24 * MHZ,
    [CLK_SCLKGPLL]   =  1056 * MHZ,
    [CLK_SCLKBPLL]   =   800 * MHZ,
    [CLK_PWM     ]   =    24 * MHZ,
    [CLK_SCLKCPLL]   =   640 * MHZ,
    [CLK_UART3   ]   =    64 * MHZ,
    [CLK_UART2   ]   =    64 * MHZ,
    [CLK_UART1   ]   =    64 * MHZ,
    [CLK_UART0   ]   =    64 * MHZ,
    [CLK_SPI1_ISP]   =    24 * MHZ,
    [CLK_SPI0_ISP]   =    24 * MHZ,
    [CLK_SCLKMPLL]   =   532 * MHZ,
    [CLK_SPI1    ]   = 53200 * KHZ,
    [CLK_SCLKEPLL]   =    24 * MHZ,
    [CLK_SPI2    ]   =     6 * MHZ,
    [CLK_SPI0    ]   =     6 * MHZ,
};

