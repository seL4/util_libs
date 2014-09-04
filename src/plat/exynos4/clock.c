/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include "../../common.h"
#include "../../arch/arm/clock.h"
#include "../../mach/exynos/clock.h"
#include <assert.h>

/* Memory map */
#define CMU_LEFTBUS_PADDR  0x10034000
#define CMU_RIGHTBUS_PADDR 0x10038000
#define CMU_TOP_PADDR      0x1003C000
#define CMU_DMC1_PADDR     0x10040000
#define CMU_DMC2_PADDR     0x10041000
#define CMU_CPU1_PADDR     0x10044000
#define CMU_CPU2_PADDR     0x10045000
#define CMU_ISP_PADDR      0x10048000

#define CMU_LEFTBUS_SIZE   0x1000
#define CMU_RIGHTBUS_SIZE  0x1000
#define CMU_TOP_SIZE       0x1000
#define CMU_DMC1_SIZE      0x1000
#define CMU_DMC2_SIZE      0x1000
#define CMU_CPU1_SIZE      0x1000
#define CMU_CPU2_SIZE      0x1000
#define CMU_ISP_SIZE       0x1000


/* Root clock frequencies */
#define XUSBXTI_FREQ 24000000UL
#define XXTI_FREQ    0UL /* ? */

/* The input source could be read from boot switches */
#if 1
#define FINPLL_FREQ XUSBXTI_FREQ
#else
#define FINPLL_FREQ XXTI_FREQ
#endif


/************************
 ****       PLL      ****
 ************************/

/* CON 0 */
#define PLL_PMS(p,m,s)  (((p)<< 8) | ((m) << 16) | ((s) << 0))
#define PLL_PMS_MASK    PLL_PMS(0x3f, 0x1ff, 0x3)
#define PLL_ENABLE      BIT(31)
#define PLL_LOCKED      BIT(29)
/* CON1 */
#define PLL_BYPASS      BIT(22)

/**** suggested PMS values ****/
/* fout = fin * (m/p/(1<<s)) */
#define AMPLL_200      PLL_PMS(3, 100, 2)
#define AMPLL_300      PLL_PMS(4, 200, 2)
#define AMPLL_400      PLL_PMS(3, 100, 1)
#define AMPLL_500      PLL_PMS(3, 125, 1)
#define AMPLL_600      PLL_PMS(4, 200, 1)
#define AMPLL_700      PLL_PMS(3, 175, 1)
#define AMPLL_800      PLL_PMS(3, 100, 0)
#define AMPLL_900      PLL_PMS(4, 150, 0)
#define AMPLL_1000     PLL_PMS(3, 125, 0)
#define AMPLL_1100     PLL_PMS(6, 275, 0)
#define AMPLL_1200     PLL_PMS(4, 200, 0)
#define AMPLL_1300     PLL_PMS(6, 325, 0)
#define AMPLL_1400     PLL_PMS(3, 175, 0)

#define EPLL_90        PLL_PMS(2,  60, 3)
#define EPLL_180       PLL_PMS(2,  60, 2)
/* 180.6 and 180.6336 can be achieved by varying K */
#define EPLL_192       PLL_PMS(2,  64, 2)
#define EPLL_200       PLL_PMS(3, 100, 2)
#define EPLL_400       PLL_PMS(3, 100, 1)
#define EPLL_408       PLL_PMS(2,  68, 1)
#define EPLL_416       PLL_PMS(3, 104, 1)

#define VPLL_100       PLL_PMS(3, 100, 3)
#define VPLL_160       PLL_PMS(3, 160, 3)
#define VPLL_266       PLL_PMS(3, 133, 2)
#define VPLL_350       PLL_PMS(3, 175, 2)
#define VPLL_440       PLL_PMS(3, 110, 1)
struct pms_tbl {
    int mhz;
    uint32_t pms;
};
static struct pms_tbl _ampll_tbl[] = {
    { 200, AMPLL_200 },
    { 300, AMPLL_300 },
    { 400, AMPLL_400 },
    { 500, AMPLL_500 },
    { 600, AMPLL_600 },
    { 700, AMPLL_700 },
    { 800, AMPLL_800 },
    { 900, AMPLL_900 },
    {1000, AMPLL_1000},
    {1100, AMPLL_1100},
    {1200, AMPLL_1200},
    {1300, AMPLL_1300},
    {1400, AMPLL_1400}
};

static struct pms_tbl _epll_tbl[] = {
    {  90, EPLL_90   },
    { 180, EPLL_180  },
    { 192, EPLL_192  },
    { 200, EPLL_200  },
    { 400, EPLL_400  },
    { 408, EPLL_408  },
    { 416, EPLL_416  }
};

static struct pms_tbl _vpll_tbl[] = {
    { 100, VPLL_100 },
    { 160, VPLL_160 },
    { 266, VPLL_266 },
    { 350, VPLL_350 },
    { 440, VPLL_440 }
};

/* Default values */
/*****************************/
#define VPLL_PMS_VAL   VPLL_100
#define EPLL_PMS_VAL   EPLL_200
#define APLL_PMS_VAL   AMPLL_1400
#define MPLL_PMS_VAL   AMPLL_1400
/*****************************/

/************************
 **** source options ****
 ************************/
/* CMU_TOP fields */
#define PERIL0      0x50
#define TOP0        0x10
/* CMU_DMC fields */
#define DMC         0x00
/* CMU_CPU fields */
#define CPU         0x00

/* PERIL CLK SEL */
#define CLK_SEL_BITS        4
#define CLK_SEL_SHIFT(x)    ((x)*CLK_SEL_BITS)
#define CLK_SEL_MASK        ((1 << CLK_SEL_SHIFT(1)) - 1)
#define CLK_SEL_VPLL        0x8
#define CLK_SEL_EPLL        0x7
#define CLK_SEL_MPLL_USER_T 0x6
#define CLK_SEL_HDMIPHY     0x5
#define CLK_SEL_USBPHY0     0x3
#define CLK_SEL_HDMU24M     0x2
#define CLK_SEL_XUSBXTI     0x1
#define CLK_SEL_XXTI        0x0

enum clkregs {
    CLKREGS_LEFT,
    CLKREGS_RIGHT,
    CLKREGS_TOP,
    CLKREGS_DMC1,
    CLKREGS_DMC2,
    CLKREGS_CPU1,
    CLKREGS_CPU2,
    CLKREGS_ISP,
    NCLKREGS
};

volatile struct cmu_leftbus_regs*  _cmu_leftbus  = NULL;
volatile struct cmu_rightbus_regs* _cmu_rightbus = NULL;
volatile struct cmu_top_regs*      _cmu_top      = NULL;
volatile struct cmu_dmc1_regs*     _cmu_dmc1     = NULL;
volatile struct cmu_dmc2_regs*     _cmu_dmc2     = NULL;
volatile struct cmu_cpu1_regs*     _cmu_cpu1     = NULL;
volatile struct cmu_cpu2_regs*     _cmu_cpu2     = NULL;
volatile struct cmu_isp_regs*      _cmu_isp      = NULL;


static struct clock finpll_clk = { CLK_OPS_DEFAULT(MASTER) };

static freq_t
_pll_get_freq(clk_t* clk)
{
    volatile struct pll_regs *regs;
    uint32_t mux;
    switch (clk->id) {
    case CLK_MOUTAPLL:
        regs = (volatile struct pll_regs*)&_cmu_cpu1->apll_lock;
        mux = _cmu_cpu1->clk_src_stat_cpu >> 0;
        break;
    case CLK_SCLKMPLL:
        regs = (volatile struct pll_regs*)&_cmu_dmc1->mpll_lock;
        mux = _cmu_dmc1->clk_src_stat_dmc >> 12;
        break;
    case CLK_SCLKEPLL:
        regs = (volatile struct pll_regs*)&_cmu_top->epll_lock;
        mux = _cmu_top->clk_src_stat_top0 >> 4;
        break;
    case CLK_SCLKVPLL:
        regs = (volatile struct pll_regs*)&_cmu_top->vpll_lock;
        mux = _cmu_top->clk_src_stat_top0 >> 8;
        break;
    default:
        assert(0);
        return -1;
    }
    if ((regs->con1) & PLL_BYPASS || (mux & 0x1)) {
        /* Muxed or bypassed to FINPLL */
        return clk_get_freq(clk->parent);
    } else {
        uint32_t v, p, m, s;
        v = regs->con0 & PLL_PMS_MASK;
        m = (v >> 16) & 0x1ff;
        p = (v >>  8) &  0x3f;
        s = (v >>  0) &   0x3;
        return ((uint64_t)clk_get_freq(clk->parent) * m / p) >> s;
    }
}

static freq_t
_pll_set_freq(clk_t* clk, freq_t hz)
{
    volatile struct pll_regs* pll_regs;
    struct pms_tbl *tbl;
    int tbl_size;
    int mhz = hz / (1 * MHZ);
    uint32_t pms;
    int i;
    switch (clk->id) {
    case CLK_SCLKEPLL:
        tbl = _epll_tbl;
        tbl_size = sizeof(_epll_tbl) / sizeof(*_epll_tbl);
        pll_regs = (volatile struct pll_regs*)&_cmu_top->epll_lock;
        break;
    case CLK_SCLKMPLL:
        tbl = _ampll_tbl;
        tbl_size = sizeof(_ampll_tbl) / sizeof(*_ampll_tbl);
        pll_regs = (volatile struct pll_regs*)&_cmu_dmc1->mpll_lock;
        break;
    case CLK_SCLKVPLL:
        tbl = _vpll_tbl;
        tbl_size = sizeof(_vpll_tbl) / sizeof(*_vpll_tbl);
        pll_regs = (volatile struct pll_regs*)&_cmu_top->vpll_lock;
        break;
    case CLK_MOUTAPLL:
        tbl = _ampll_tbl;
        tbl_size = sizeof(_ampll_tbl) / sizeof(*_ampll_tbl);
        pll_regs = (volatile struct pll_regs*)&_cmu_cpu1->apll_lock;
        break;
    default:
        printf("Unknown clock ID %d\n", clk->id);
        assert(0);
        return 0;
    }
    /* Search the table for an appropriate value */
    pms = tbl[tbl_size - 1].pms;
    for (i = 0; i < tbl_size; i++) {
        if (tbl[i].mhz >= mhz) {
            pms = tbl[i].pms;
            break;
        }
    }
    pll_regs->con1 |= PLL_BYPASS;
    pll_regs->con0 = pms | PLL_ENABLE;
    while (!(pll_regs->con0 & PLL_LOCKED));
    pll_regs->con1 &= ~PLL_BYPASS;
    /* Can we handle this ourselves? */
    return clk_get_freq(clk);
}

static void
_pll_recal(clk_t* clk)
{
    assert(0);
}


static clk_t*
_pll_init(clk_t* clk)
{
    clk_t* parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MASTER);
    clk_init(parent);
    clk_register_child(parent, clk);
    return clk;
}

static struct clock aoutpll_clk  = { CLK_OPS(MOUTAPLL, pll, NULL) };
static struct clock sclkmpll_clk = { CLK_OPS(SCLKMPLL, pll, NULL) };
static struct clock sclkepll_clk = { CLK_OPS(SCLKEPLL, pll, NULL) };
static struct clock sclkvpll_clk = { CLK_OPS(SCLKVPLL, pll, NULL) };


static freq_t
_div_get_freq(clk_t* clk)
{
    uint32_t div = 1;
    uint32_t fin = clk_get_freq(clk->parent);
    assert(_cmu_cpu1);
    switch (clk->id) {
        /* CPU 0 */
    case CLK_DIVCORE2:
        div = _cmu_cpu1->clk_div_cpu0 >> 28;
        break;
    case CLK_SCLKAPLL:
        div = _cmu_cpu1->clk_div_cpu0 >> 24;
        break;
    case CLK_PCLK_DBG:
        div = _cmu_cpu1->clk_div_cpu0 >> 20;
        break;
    case CLK_ATCLK:
        div = _cmu_cpu1->clk_div_cpu0 >> 16;
        break;
    case CLK_PERIPHCLK:
        div = _cmu_cpu1->clk_div_cpu0 >> 12;
        break;
    case CLK_ACLK_COREM1:
        div = _cmu_cpu1->clk_div_cpu0 >> 8;
        break;
    case CLK_ACLK_COREM0:
        div = _cmu_cpu1->clk_div_cpu0 >> 4;
        break;
    case CLK_DIVCORE:
        div = _cmu_cpu1->clk_div_cpu0 >> 0;
        break;
        /* CPU1 */
    case CLK_ACLK_CORES:
        div = _cmu_cpu1->clk_div_cpu1 >> 8;
        break;
    case CLK_SCLKHPM:
        div = _cmu_cpu1->clk_div_cpu1 >> 4;
        break;
    case CLK_DIVCOPY:
        div = _cmu_cpu1->clk_div_cpu1 >> 0;
        break;
        /* ---- */
    default:
        printf("Unimplemented\n");
        return 0;
    }
    div &= CLK_DIV_MASK;
    return fin / (div + 1);
}

static freq_t
_div_set_freq(clk_t* clk, freq_t hz)
{
    uint32_t p;
    uint32_t v;
    uint32_t div;
    volatile uint32_t *reg;
    int shift;
    /* Can we achieve hz by division? */
    p = clk_get_freq(clk->parent);
    if (p < hz || p / (CLK_DIV_MASK + 1) > hz) {
        clk_set_freq(clk->parent, hz);
    }
    p = clk_get_freq(clk->parent);
    div = p / hz;
    if (div > CLK_DIV_MASK) {
        div = CLK_DIV_MASK;
    }
    switch (clk->id) {
        /* CPU 0 */
    case CLK_DIVCORE2:
        reg = &_cmu_cpu1->clk_div_cpu0;
        shift = 28;
        break;
    case CLK_SCLKAPLL:
        reg = &_cmu_cpu1->clk_div_cpu0;
        shift = 24;
        break;
    case CLK_PCLK_DBG:
        reg = &_cmu_cpu1->clk_div_cpu0;
        shift = 20;
        break;
    case CLK_ATCLK:
        reg = &_cmu_cpu1->clk_div_cpu0;
        shift = 16;
        break;
    case CLK_PERIPHCLK:
        reg = &_cmu_cpu1->clk_div_cpu0;
        shift = 12;
        break;
    case CLK_ACLK_COREM1:
        reg = &_cmu_cpu1->clk_div_cpu0;
        shift = 8;
        break;
    case CLK_ACLK_COREM0:
        reg = &_cmu_cpu1->clk_div_cpu0;
        shift = 4;
        break;
    case CLK_DIVCORE:
        reg = &_cmu_cpu1->clk_div_cpu0;
        shift = 0;
        break;
        /* CPU1 */
    case CLK_ACLK_CORES:
        reg = &_cmu_cpu1->clk_div_cpu1;
        shift = 8;
        break;
    case CLK_SCLKHPM:
        reg = &_cmu_cpu1->clk_div_cpu1;
        shift = 4;
        break;
    case CLK_DIVCOPY:
        reg = &_cmu_cpu1->clk_div_cpu1;
        shift = 0;
        break;
        /* ---- */
    default:
        printf("Unimplemented\n");
        return 0;
    }
    v = *reg;
    v &= ~(CLK_DIV_MASK << shift);
    v |= div << shift;
//    *reg = v;
    reg += CLK_DIVSTAT_OFFSET / 4;
    while (*reg & (CLK_DIVSTAT_UNSTABLE << shift));
    return clk_get_freq(clk);
}

static void
_div_recal(clk_t* clk)
{
    assert(0);
}

static clk_t*
_div_init(clk_t* clk)
{
    clk_t* parent;
    switch (clk->id) {
    case CLK_SCLKAPLL:
        parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MOUTAPLL);
        break;
    case CLK_DIVCORE:
    case CLK_ATCLK:
        parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MUXCORE);
        break;
    case CLK_DIVCORE2:
        parent = clk_get_clock(clk_get_clock_sys(clk), CLK_DIVCORE);
        break;
    case CLK_ACLK_COREM0:
    case CLK_ACLK_CORES:
    case CLK_ACLK_COREM1:
    case CLK_PERIPHCLK:
        parent = clk_get_clock(clk_get_clock_sys(clk), CLK_DIVCORE2);
        break;
    case CLK_PCLK_DBG:
        parent = clk_get_clock(clk_get_clock_sys(clk), CLK_ATCLK);
        break;
    case CLK_SCLKHPM:
        parent = clk_get_clock(clk_get_clock_sys(clk), CLK_DIVCOPY);
        break;
    case CLK_DIVCOPY:
        parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MUXHPM);
        break;
    default:
        assert(!"Unknown clock id for div");
        parent = NULL;
    }
    assert(parent);
    clk_init(parent);
    clk_register_child(parent, clk);
    return clk;
}

static struct clock sclkapll_clk  = { CLK_OPS(SCLKAPLL   , div, NULL) };
static struct clock divcore_clk   = { CLK_OPS(DIVCORE    , div, NULL) };
static struct clock arm_clk       = { CLK_OPS(DIVCORE2   , div, NULL) };
static struct clock corem0_clk    = { CLK_OPS(ACLK_COREM0, div, NULL) };
static struct clock corem1_clk    = { CLK_OPS(ACLK_COREM1, div, NULL) };
static struct clock cores_clk     = { CLK_OPS(ACLK_CORES , div, NULL) };
static struct clock periphclk_clk = { CLK_OPS(PERIPHCLK  , div, NULL) };
static struct clock atclk_clk     = { CLK_OPS(ATCLK      , div, NULL) };
static struct clock pclk_dbg_clk  = { CLK_OPS(PCLK_DBG   , div, NULL) };
static struct clock sclkhpm_clk   = { CLK_OPS(SCLKHPM    , div, NULL) };
static struct clock divcopy_clk   = { CLK_OPS(DIVCOPY    , div, NULL) };



/***************
 **** MUXes ****
 ***************/
static freq_t
_mux_get_freq(clk_t* clk)
{
    return clk_get_freq(clk->parent);
}

static freq_t
_mux_set_freq(clk_t* clk, freq_t hz)
{
    /* TODO: we can choose a different source... */
    clk_set_freq(clk->parent, hz);
    return clk_get_freq(clk);
}

static void
_mux_recal(clk_t* clk)
{
    assert(0);
}

static clk_t*
_mux_init(clk_t* clk)
{
    clk_t* parent = NULL;
    uint32_t mux;
    enum clk_id parent_id[2];
    assert(_cmu_cpu1);
    assert(clk);
    switch (clk->id) {
    case CLK_SCLKMPLL_USERC:
        mux = _cmu_cpu1->clk_src_stat_cpu >> 24;
        parent_id[0] = CLK_MASTER;
        parent_id[1] = CLK_SCLKMPLL;
        break;
    case CLK_MUXHPM:
        mux = _cmu_cpu1->clk_src_stat_cpu >> 20;
        parent_id[0] = CLK_MOUTAPLL;
        parent_id[1] = CLK_SCLKMPLL;
        break;
    case CLK_MUXCORE:
        mux = _cmu_cpu1->clk_src_stat_cpu >> 16;
        parent_id[0] = CLK_MOUTAPLL;
        /* Tree says SCLKMPLL_USERC, Table says SCLKMPLL... */
        parent_id[1] = CLK_SCLKMPLL_USERC;
//        parent_id[1] = CLK_SCLKMPLL;
        break;
    default:
        assert(!"Unknown clock id for mux");
        return NULL;
    }
    mux &= MASK(4);
    if (mux & 0x4) {
        printf("%s is in transition\n", clk->name);
        assert(0);
    }
    parent = clk_get_clock(clk_get_clock_sys(clk), parent_id[mux - 1]);
    assert(parent);
    clk_init(parent);
    clk_register_child(parent, clk);
    return clk;
}

static struct clock sclk_mpll_userc_clk = { CLK_OPS(SCLKMPLL_USERC, mux, NULL) };
static struct clock muxcore_clk = { CLK_OPS(MUXCORE, mux, NULL) };
static struct clock muxhpm_clk = { CLK_OPS(MUXHPM, mux, NULL) };

static int
exynos4_gate_enable(clock_sys_t* sys, enum clock_gate gate, enum clock_gate_mode mode)
{
    (void)sys;
    (void)gate;
    (void)mode;
    return 0;
}

int
clock_sys_init(ps_io_ops_t* o, clock_sys_t* clock_sys)
{
    MAP_IF_NULL(o, CMU_LEFTBUS , _cmu_leftbus);
    MAP_IF_NULL(o, CMU_RIGHTBUS, _cmu_rightbus);
    MAP_IF_NULL(o, CMU_TOP     , _cmu_top);
    MAP_IF_NULL(o, CMU_DMC1    , _cmu_dmc1);
    MAP_IF_NULL(o, CMU_DMC2    , _cmu_dmc2);
    MAP_IF_NULL(o, CMU_CPU1    , _cmu_cpu1);
    MAP_IF_NULL(o, CMU_CPU2    , _cmu_cpu2);
    MAP_IF_NULL(o, CMU_ISP     , _cmu_isp);
    MAPCHECK(&_cmu_leftbus->clkout_cmu_leftbus_div_stat, 0xa04);
    MAPCHECK(&_cmu_rightbus->clkout_cmu_rightbus_div_stat, 0xa04);
    MAPCHECK(&_cmu_dmc1->clkout_cmu_dmc_div_stat, 0xa04);
    MAPCHECK(&_cmu_cpu1->clkout_cmu_cpu_div_stat, 0xa04);
    MAPCHECK(&_cmu_dmc2->c2c_priv, 0x09c);

    MAPCHECK(&_cmu_cpu2->ptm_status, 0x420);
    MAPCHECK(&_cmu_isp->cmu_isp_spare[0], 0xb00);

    /* TODO: create a struct for registers */
    clock_sys->priv = (void*)0xDEADBEEF;
    clock_sys->get_clock = &ps_get_clock;
    clock_sys->gate_enable = &exynos4_gate_enable;
    return 0;
}



void
clk_print_clock_tree(clock_sys_t* sys UNUSED)
{
    clk_t* clk = ps_clocks[CLK_MASTER];
    clk_print_tree(clk, "");
}

clk_t* ps_clocks[] = {
    [CLK_MASTER]         = &finpll_clk,
    [CLK_MOUTAPLL]       = &aoutpll_clk,
    [CLK_SCLKMPLL]       = &sclkmpll_clk,
    [CLK_SCLKEPLL]       = &sclkepll_clk,
    [CLK_SCLKVPLL]       = &sclkvpll_clk,
    [CLK_SCLKAPLL]       = &sclkapll_clk,
    [CLK_MUXCORE]        = &muxcore_clk,
    [CLK_DIVCORE]        = &divcore_clk,
    [CLK_DIVCORE2]       = &arm_clk,
    [CLK_ACLK_COREM0]    = &corem0_clk,
    [CLK_ACLK_COREM1]    = &corem1_clk,
    [CLK_ACLK_CORES]     = &cores_clk,
    [CLK_PERIPHCLK]      = &periphclk_clk,
    [CLK_ATCLK]          = &atclk_clk,
    [CLK_PCLK_DBG]       = &pclk_dbg_clk,
    [CLK_SCLKMPLL_USERC] = &sclk_mpll_userc_clk,
    [CLK_SCLKHPM]        = &sclkhpm_clk,
    [CLK_DIVCOPY]        = &divcopy_clk,
    [CLK_MUXHPM]         = &muxhpm_clk
};


/* These frequencies are NOT the recommended
 * frequencies. They are to be used when we
 * need to make assumptions about what u-boot
 * has left us with. */
freq_t ps_freq_default[] = {
    [CLK_MASTER]         = FINPLL_FREQ,
    [CLK_MOUTAPLL]       = 1000 * MHZ,
    [CLK_SCLKMPLL]       =  800 * MHZ,
    [CLK_SCLKEPLL]       =   96 * MHZ,
    [CLK_SCLKVPLL]       =  108 * MHZ,
    [CLK_SCLKAPLL]       =  500 * MHZ,
    [CLK_MUXCORE]        = 1000 * MHZ,
    [CLK_DIVCORE]        = 1000 * MHZ,
    [CLK_DIVCORE2]       = 1000 * MHZ,
    [CLK_ACLK_COREM0]    =  333 * MHZ,
    [CLK_ACLK_COREM1]    =  167 * MHZ,
    [CLK_ACLK_CORES]     =  250 * MHZ,
    [CLK_PERIPHCLK]      =  125 * MHZ,
    [CLK_ATCLK]          =  200 * MHZ,
    [CLK_PCLK_DBG]       =  100 * MHZ,
    [CLK_SCLKMPLL_USERC] =  800 * MHZ,
    [CLK_SCLKHPM]        =  200 * MHZ,
    [CLK_DIVCOPY]        =  200 * MHZ,
    [CLK_MUXHPM]         = 1000 * MHZ
};


