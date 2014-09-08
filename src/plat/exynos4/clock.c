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

#define OFFSET_MOUTAPLL    (0x00 / 4)
#define OFFSET_SCLKMPLL    (0x08 / 4)
#define OFFSET_SCLKEPLL    (0x10 / 4)
#define OFFSET_SCLKVPLL    (0x20 / 4)

#define CLKID_MOUTAPLL     CLKID(CPU1, 0, 0)
#define CLKID_SCLKMPLL     CLKID(DMC1, 2, 3)
#define CLKID_SCLKEPLL     CLKID(TOP , 4, 1)
#define CLKID_SCLKVPLL     CLKID(TOP , 4, 2)

#define CLKID_DIVCORE      CLKID(CPU1, 0, 0)
#define CLKID_ACLK_COREM0  CLKID(CPU1, 0, 1)
#define CLKID_ACLK_COREM1  CLKID(CPU1, 0, 2)
#define CLKID_PERIPHCLK    CLKID(CPU1, 0, 3)
#define CLKID_ATCLK        CLKID(CPU1, 0, 4)
#define CLKID_PCLK_DBG     CLKID(CPU1, 0, 5)
#define CLKID_SCLKAPLL     CLKID(CPU1, 0, 6)
#define CLKID_DIVCORE2     CLKID(CPU1, 0, 7)
#define CLKID_DIVCOPY      CLKID(CPU1, 1, 0)
#define CLKID_SCLKHPM      CLKID(CPU1, 1, 1)
#define CLKID_ACLK_CORES   CLKID(CPU1, 1, 2)



/************************
 ****       PLL      ****
 ************************/

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

volatile struct clk_regs* _clk_regs[NCLKREGS];

static struct clock finpll_clk = { CLK_OPS_DEFAULT(MASTER) };




static clk_t*
_pll_init(clk_t* clk)
{
    clk_t* parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MASTER);
    clk_init(parent);
    clk_register_child(parent, clk);
    return clk;
}

static struct pll_priv moutapll_priv = PLL_PRIV(MOUTAPLL, PMS, _ampll_tbl);
static struct pll_priv sclkmpll_priv = PLL_PRIV(SCLKMPLL, PMS, _ampll_tbl);
static struct pll_priv sclkepll_priv = PLL_PRIV(SCLKEPLL, PMS, _epll_tbl);
static struct pll_priv sclkvpll_priv = PLL_PRIV(SCLKVPLL, PMS, _vpll_tbl);

static struct clock aoutpll_clk  = { CLK_OPS(MOUTAPLL, pll, &moutapll_priv) };
static struct clock sclkmpll_clk = { CLK_OPS(SCLKMPLL, pll, &sclkmpll_priv) };
static struct clock sclkepll_clk = { CLK_OPS(SCLKEPLL, pll, &sclkepll_priv) };
static struct clock sclkvpll_clk = { CLK_OPS(SCLKVPLL, pll, &sclkvpll_priv) };

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


static struct clock sclkapll_clk  = { CLK_OPS(SCLKAPLL   , div, CLKID_SCLKAPLL)    };
static struct clock divcore_clk   = { CLK_OPS(DIVCORE    , div, CLKID_DIVCORE)     };
static struct clock arm_clk       = { CLK_OPS(DIVCORE2   , div, CLKID_DIVCORE2)    };
static struct clock corem0_clk    = { CLK_OPS(ACLK_COREM0, div, CLKID_ACLK_COREM0) };
static struct clock corem1_clk    = { CLK_OPS(ACLK_COREM1, div, CLKID_ACLK_COREM1) };
static struct clock cores_clk     = { CLK_OPS(ACLK_CORES , div, CLKID_ACLK_CORES)  };
static struct clock periphclk_clk = { CLK_OPS(PERIPHCLK  , div, CLKID_PERIPHCLK)   };
static struct clock atclk_clk     = { CLK_OPS(ATCLK      , div, CLKID_ATCLK)       };
static struct clock pclk_dbg_clk  = { CLK_OPS(PCLK_DBG   , div, CLKID_PCLK_DBG)    };
static struct clock sclkhpm_clk   = { CLK_OPS(SCLKHPM    , div, CLKID_SCLKHPM)     };
static struct clock divcopy_clk   = { CLK_OPS(DIVCOPY    , div, CLKID_DIVCOPY)     };



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
    assert(clk);
    switch (clk->id) {
    case CLK_SCLKMPLL_USERC:
        mux = _clk_regs[CLKREGS_CPU1]->srcstat[0] >> 24;
        parent_id[0] = CLK_MASTER;
        parent_id[1] = CLK_SCLKMPLL;
        break;
    case CLK_MUXHPM:
        mux = _clk_regs[CLKREGS_CPU1]->srcstat[0] >> 20;
        parent_id[0] = CLK_MOUTAPLL;
        parent_id[1] = CLK_SCLKMPLL;
        break;
    case CLK_MUXCORE:
        mux = _clk_regs[CLKREGS_CPU1]->srcstat[0] >> 16;
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

static int
clock_sys_common_init(clock_sys_t* clock_sys)
{
    clock_sys->priv = (void*)_clk_regs;
    clock_sys->get_clock = &ps_get_clock;
    clock_sys->gate_enable = &exynos4_gate_enable;
    return 0;
}

int
clock_sys_init(ps_io_ops_t* o, clock_sys_t* clock_sys)
{
    MAP_IF_NULL(o, CMU_LEFTBUS , _clk_regs[CLKREGS_LEFT]);
    MAP_IF_NULL(o, CMU_RIGHTBUS, _clk_regs[CLKREGS_RIGHT]);
    MAP_IF_NULL(o, CMU_TOP     , _clk_regs[CLKREGS_TOP]);
    MAP_IF_NULL(o, CMU_DMC1    , _clk_regs[CLKREGS_DMC1]);
    MAP_IF_NULL(o, CMU_DMC2    , _clk_regs[CLKREGS_DMC2]);
    MAP_IF_NULL(o, CMU_CPU1    , _clk_regs[CLKREGS_CPU1]);
    MAP_IF_NULL(o, CMU_CPU2    , _clk_regs[CLKREGS_CPU2]);
    MAP_IF_NULL(o, CMU_ISP     , _clk_regs[CLKREGS_ISP]);
    return clock_sys_common_init(clock_sys);
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
    [CLK_MUXHPM]         = 1000 * MHZ,
    [CLK_DIVCOPY]        =  200 * MHZ,
    [CLK_SCLKHPM]        =  200 * MHZ,
    [CLK_MUXCORE]        = 1000 * MHZ,
    [CLK_ATCLK]          =  200 * MHZ,
    [CLK_PCLK_DBG]       =  100 * MHZ,
    [CLK_DIVCORE]        = 1000 * MHZ,
    [CLK_DIVCORE2]       = 1000 * MHZ,
    [CLK_PERIPHCLK]      =  125 * MHZ,
    [CLK_ACLK_COREM1]    =  167 * MHZ,
    [CLK_ACLK_CORES]     =  250 * MHZ,
    [CLK_ACLK_COREM0]    =  333 * MHZ,

    [CLK_SCLKAPLL]       =  500 * MHZ,
    [CLK_SCLKVPLL]       =  108 * MHZ,
    [CLK_SCLKEPLL]       =   96 * MHZ,
    [CLK_SCLKMPLL]       =  800 * MHZ,
    [CLK_SCLKMPLL_USERC] =  800 * MHZ,
};

