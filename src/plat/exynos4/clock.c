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

#define DIV_CLK_OPS(clk)           \
        .get_freq = _div_get_freq, \
        .set_freq = _div_set_freq, \
        .recal = _div_recal,       \
        .init = _div_init,         \
        .parent = NULL,            \
        .sibling = NULL,           \
        .child = NULL,             \
        .name = #clk

#define MUX_CLK_OPS(clk)           \
        .get_freq = _mux_get_freq, \
        .set_freq = _mux_set_freq, \
        .recal = _mux_recal,       \
        .init = _mux_init,         \
        .parent = NULL,            \
        .sibling = NULL,           \
        .child = NULL,             \
        .name = #clk




/************************
 ****       PLL      ****
 ************************/
#define PLL_CLK_OPS(clk)           \
        .get_freq = _pll_get_freq, \
        .set_freq = _pll_set_freq, \
        .recal = _pll_recal,       \
        .init = _pll_init,         \
        .parent = NULL,            \
        .sibling = NULL,           \
        .child = NULL,             \
        .name = #clk

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


/* MUX STAT */
#define CLK_MUXSTAT_BITS     4
#define CLK_MUXSTAT_SHIFT(x) ((x)*CLK_MASK_BITS)
#define CLK_MUXSTAT_MASK     ((1 << CLK_MASK_SHIFT(1)) - 1)
#define CLK_MUXSTAT_CHANGING (0x4)


/* CLK MASK */
#define CLK_MASK_BITS     4
#define CLK_MASK_SHIFT(x) ((x)*CLK_MASK_BITS)
#define CLK_MASK_MASK     ((1 << CLK_MASK_SHIFT(1)) - 1)
#define CLK_MASK_SET      (0x00)
#define CLK_MASK_CLEAR    (0x01)

/* CLK DIV */
#define CLK_DIV_BITS     4
#define CLK_DIV_SHIFT(x) ((x)*CLK_DIV_BITS)
#define CLK_DIV_MASK     ((1 << CLK_DIV_SHIFT(1)) - 1)

/* CLK DIVSTAT */
#define CLK_DIVSTAT_OFFSET   (0x100)
#define CLK_DIVSTAT_BITS     4
#define CLK_DIVSTAT_SHIFT(x) ((x)*CLK_DIVSTAT_BITS)
#define CLK_DIVSTAT_MASK     ((1 << CLK_DIVSTAT_SHIFT(1)) - 1)
#define CLK_DIVSTAT_STABLE   0x0
#define CLK_DIVSTAT_UNSTABLE 0x1

/***** clock gating *****/
#define CLK_GATE_BITS     1
#define CLK_GATE_SHIFT(x) ((x)*CLK_GATE_BITS)
#define CLK_GATE_MASK     ((1<<CLK_GATE_SHIFT(1)) - 1)
#define CLK_GATE_SKIP     0x0
#define CLK_GATE_PASS     0x1

struct pll_regs {
    uint32_t lock;
    uint32_t res[63];
    uint32_t con0;
    uint32_t con1;
    uint32_t con2;
};

/* 0x10034000 */
struct cmu_leftbus_regs {
    uint32_t res0[128];
    uint32_t clk_src_leftbus;              /* 0x200 */
    uint32_t res1[127];
    uint32_t clk_mux_stat_leftbus;         /* 0x400 */
    uint32_t res2[63];
    uint32_t clk_div_leftbus;              /* 0x500 */
    uint32_t res3[63];
    uint32_t clk_div_stat_leftbus;         /* 0x600 */
    uint32_t res4[127];
    uint32_t clk_gate_ip_leftbus;          /* 0x800 */
    uint32_t res6[63];
    uint32_t res7[12];
    uint32_t clk_gate_ip_image;            /* 0x930 */
    uint32_t res8[51];
    uint32_t clkout_cmu_leftbus;           /* 0xa00 */
    uint32_t clkout_cmu_leftbus_div_stat;  /* 0xa04 */
};
/* 0x10038000 */
struct cmu_rightbus_regs {
    uint32_t res0[128];
    uint32_t clk_src_rightbus;             /* 0x200 */
    uint32_t res1[127];
    uint32_t clk_mux_stat_rightbus;        /* 0x400 */
    uint32_t res2[63];
    uint32_t clk_div_rightbus;             /* 0x500 */
    uint32_t res3[63];
    uint32_t clk_div_stat_rightbus;        /* 0x600 */
    uint32_t res4[127];
    uint32_t clk_gate_ip_rightbus;         /* 0x800 */
    uint32_t res6[63];
    uint32_t res7[24];
    uint32_t clk_gate_ip_perir;            /* 0x960 */
    uint32_t res8[39];
    uint32_t clkout_cmu_rightbus;          /* 0xa00 */
    uint32_t clkout_cmu_rightbus_div_stat; /* 0xa04 */
};
/* 0x1003c000 */
struct cmu_top_regs {
    uint32_t res0[4];
    uint32_t epll_lock;               /* 0x010 */
    uint32_t res1[3];
    uint32_t vpll_lock;               /* 0x020 */
    uint32_t res2[59];
    uint32_t epll_con0;               /* 0x110 */
    uint32_t epll_con1;               /* 0x114 */
    uint32_t epll_con2;               /* 0x118 */
    uint32_t res3[1];
    uint32_t vpll_con0;               /* 0x120 */
    uint32_t vpll_con1;               /* 0x124 */
    uint32_t vpll_con2;               /* 0x128 */
    uint32_t res4[57];
    uint32_t clk_src_top0;            /* 0x210 */
    uint32_t clk_src_top1;            /* 0x214 */
    uint32_t res5[2];
    uint32_t clk_src_cam0;            /* 0x220 */
    uint32_t clk_src_tv;              /* 0x224 */
    uint32_t clk_src_mfc;             /* 0x228 */
    uint32_t clk_src_g3d;             /* 0x22c */
    uint32_t res6[1];
    uint32_t clk_src_lcd;             /* 0x234 */
    uint32_t clk_src_isp;             /* 0x238 */
    uint32_t clk_src_maudio;          /* 0x23c */
    uint32_t clk_src_fsys;            /* 0x240 */
    uint32_t res7[3];
    uint32_t clk_src_peril0;          /* 0x250 */
    uint32_t clk_src_peril1;          /* 0x254 */
    uint32_t clk_src_cam1;            /* 0x258 */
    uint32_t res8[49];
    uint32_t clk_src_mask_cam0;       /* 0x320 */
    uint32_t clk_src_mask_tv;         /* 0x324 */
    uint32_t res9[3];
    uint32_t clk_src_mask_lcd;        /* 0x334 */
    uint32_t clk_src_mask_isp;        /* 0x338 */
    uint32_t clk_src_mask_maudio;     /* 0x33c */
    uint32_t clk_src_mask_fsys;       /* 0x340 */
    uint32_t res10[3];
    uint32_t clk_src_mask_peril0;     /* 0x350 */
    uint32_t clk_src_mask_peril1;     /* 0x354 */
    uint32_t res11[46];
    uint32_t clk_mux_stat_top0;       /* 0x410 */
    uint32_t clk_mux_stat_top1;       /* 0x414 */
    uint32_t res12[4];
    uint32_t clk_mux_stat_mfc;        /* 0x428 */
    uint32_t clk_mux_stat_g3d;        /* 0x42c */
    uint32_t res13[10];
    uint32_t clk_mux_stat_cam1;       /* 0x458 */
    uint32_t res14[45];
    uint32_t clk_div_top;             /* 0x510 */
    uint32_t res15[3];
    uint32_t clk_div_cam0;            /* 0x520 */
    uint32_t clk_div_tv;              /* 0x524 */
    uint32_t clk_div_mfc;             /* 0x528 */
    uint32_t clk_div_g3d;             /* 0x52c */
    uint32_t res16[1];
    uint32_t clk_div_lcd;             /* 0x534 */
    uint32_t clk_div_isp;             /* 0x538 */
    uint32_t clk_div_maudio;          /* 0x53c */
    uint32_t clk_div_fsys0;           /* 0x540 */
    uint32_t clk_div_fsys1;           /* 0x544 */
    uint32_t clk_div_fsys2;           /* 0x548 */
    uint32_t clk_div_fsys3;           /* 0x54c */
    uint32_t clk_div_peril0;          /* 0x550 */
    uint32_t clk_div_peril1;          /* 0x554 */
    uint32_t clk_div_peril2;          /* 0x558 */
    uint32_t clk_div_peril3;          /* 0x55c */
    uint32_t clk_div_peril4;          /* 0x560 */
    uint32_t clk_div_peril5;          /* 0x564 */
    uint32_t clk_div_cam1;            /* 0x568 */
    uint32_t res17[5];
    uint32_t clkdiv2_ratio;           /* 0x580 */
    uint32_t res18[35];
    uint32_t clk_div_stat_top;        /* 0x610 */
    uint32_t res19[3];
    uint32_t clk_div_stat_cam0;       /* 0x620 */
    uint32_t clk_div_stat_tv;         /* 0x624 */
    uint32_t clk_div_stat_mfc;        /* 0x628 */
    uint32_t clk_div_stat_g3d;        /* 0x62c */
    uint32_t res20[2];
    uint32_t clk_div_stat_isp;        /* 0x638 */
    uint32_t clk_div_stat_maudio;     /* 0x63c */
    uint32_t clk_div_stat_fsys0;      /* 0x640 */
    uint32_t clk_div_stat_fsys1;      /* 0x644 */
    uint32_t clk_div_stat_fsys2;      /* 0x648 */
    uint32_t clk_div_stat_fsys3;      /* 0x64c */
    uint32_t clk_div_stat_peril0;     /* 0x650 */
    uint32_t clk_div_stat_peril1;     /* 0x654 */
    uint32_t clk_div_stat_peril2;     /* 0x658 */
    uint32_t clk_div_stat_peril3;     /* 0x65c */
    uint32_t clk_div_stat_peril4;     /* 0x660 */
    uint32_t clk_div_stat_peril5;     /* 0x664 */
    uint32_t clk_div_stat_cam1;       /* 0x668 */
    uint32_t res21[5];
    uint32_t clkdiv2_stat;            /* 0x680 */
    uint32_t res22[48];
    uint32_t clk_gate_bus_fsys1;      /* 0x744 */
    uint32_t res23[48];
    uint32_t clk_gate_ip_cam;         /* 0x920 */
    uint32_t clk_gate_ip_tv;          /* 0x924 */
    uint32_t clk_gate_ip_mfc;         /* 0x928 */
    uint32_t clk_gate_ip_g3d;         /* 0x92c */
    uint32_t res24[2];
    uint32_t clk_gate_ip_isp;         /* 0x938 */
    uint32_t res25[1];
    uint32_t clk_gate_ip_fsys;        /* 0x940 */
    uint32_t clk_gate_ip_gps;         /* 0x94c */
    uint32_t clk_gate_ip_peril;       /* 0x950 */
    uint32_t res26[7];
    uint32_t clk_gate_block;          /* 0x970 */
    uint32_t res27[35];
    uint32_t clkout_cmu_top;          /* 0xa00 */
    uint32_t clkout_cmu_top_div_stat; /* 0xa04 */
};
/* 0x10040000 */
struct cmu_dmc1_regs {
    uint32_t res0[2];
    uint32_t mpll_lock;               /* 0x008 */
    uint32_t res1[63];
    uint32_t mpll_con0;               /* 0x108 */
    uint32_t mpll_con1;               /* 0x10c */
    uint32_t res2[60];
    uint32_t clk_src_dmc;             /* 0x200 */
    uint32_t res3[63];
    uint32_t clk_src_mask_dmc;        /* 0x300 */
    uint32_t res4[63];
    uint32_t clk_mux_stat_dmc;        /* 0x400 */
    uint32_t res5[63];
    uint32_t clk_div_dmc0;            /* 0x500 */
    uint32_t clk_div_dmc1;            /* 0x504 */
    uint32_t res6[62];
    uint32_t clk_div_stat_dmc0;       /* 0x600 */
    uint32_t clk_div_stat_dmc1;       /* 0x604 */
    uint32_t res7[62];
    uint32_t clk_gate_bus_dmc0;       /* 0x700 */
    uint32_t clk_gate_bus_dmc1;       /* 0x704 */
    uint32_t res8[62];
    uint32_t res9[64];
    uint32_t clk_gate_ip_dmc0;        /* 0x900 */
    uint32_t clk_gate_ip_dmc1;        /* 0x904 */
    uint32_t res10[62];
    uint32_t clkout_cmu_dmc;          /* 0xa00 */
    uint32_t clkout_cmu_dmc_div_stat; /* 0xa04 */
};
/* 0x10041000 */
struct cmu_dmc2_regs {
    uint32_t dcgidx_map0;             /* 0x000 */
    uint32_t dcgidx_map1;             /* 0x004 */
    uint32_t dcgidx_map2;             /* 0x008 */
    uint32_t res0[5];
    uint32_t dcgperf_map0;            /* 0x020 */
    uint32_t dcgperf_map1;            /* 0x024 */
    uint32_t res1[6];
    uint32_t dvcidx_map;              /* 0x040 */
    uint32_t res2[7];
    uint32_t freq_cpu;                /* 0x060 */
    uint32_t freq_dpm;                /* 0x064 */
    uint32_t res3[6];
    uint32_t dvsemclk_en;             /* 0x080 */
    uint32_t maxperf;                 /* 0x084 */
    uint32_t res4[3];
    uint32_t dmc_pause_ctrl;          /* 0x094 */
    uint32_t ddrphy_lock_ctrl;        /* 0x098 */
    uint32_t c2c_priv;               /* 0x09c */
};
/* 0x10044000 */
struct cmu_cpu1_regs {
    uint32_t apll_lock;               /* 0x000 */
    uint32_t res0[63];
    uint32_t apll_con0;               /* 0x100 */
    uint32_t apll_con1;               /* 0x104 */
    uint32_t res1[62];
    uint32_t clk_src_cpu;             /* 0x200 */
    uint32_t res2[127];
    uint32_t clk_mux_stat_cpu;        /* 0x400 */
    uint32_t res3[63];
    uint32_t clk_div_cpu0;            /* 0x500 */
    uint32_t clk_div_cpu1;            /* 0x504 */
    uint32_t res4[62];
    uint32_t clk_div_stat_cpu0;       /* 0x600 */
    uint32_t clk_div_stat_cpu1;       /* 0x604 */
    uint32_t res5[62];
    uint32_t res6[128];
    uint32_t clk_gate_ip_cpu;         /* 0x900 */
    uint32_t res8[63];
    uint32_t clkout_cmu_cpu;          /* 0xa00 */
    uint32_t clkout_cmu_cpu_div_stat; /* 0xa04 */
};
/* 0x10045000 */
struct cmu_cpu2_regs {
    uint32_t armclk_stopctrl;         /* 0x000 */
    uint32_t atclk_stopctrl;          /* 0x004 */
    uint32_t res0[6];
    uint32_t pwr_ctrl;                /* 0x020 */
    uint32_t pwr_ctrl2;               /* 0x024 */
    uint32_t res1[246];
    uint32_t l2_status;               /* 0x400 */
    uint32_t res2[3];
    uint32_t cpu_status;              /* 0x410 */
    uint32_t res3[3];
    uint32_t ptm_status;              /* 0x420 */
};
/* 0x10048000 */
struct cmu_isp_regs {
    uint32_t res0[192];
    uint32_t clk_div_isp0;            /* 0x300 */
    uint32_t clk_div_isp1;            /* 0x304 */
    uint32_t res1[62];
    uint32_t clk_div_stat_isp0;       /* 0x400 */
    uint32_t clk_div_stat_isp1;       /* 0x404 */
    uint32_t res3[254];
    uint32_t clk_gate_ip_isp0;        /* 0x800 */
    uint32_t clk_gate_ip_isp1;        /* 0x804 */
    uint32_t res4[126];
    uint32_t clkout_cmu_isp;          /* 0xa00 */
    uint32_t clkout_cmu_isp_div_stat; /* 0xa04 */
    uint32_t res5[62];
    uint32_t cmu_isp_spare[4];        /* 0xb00 */
};

volatile struct cmu_leftbus_regs*  _cmu_leftbus  = NULL;
volatile struct cmu_rightbus_regs* _cmu_rightbus = NULL;
volatile struct cmu_top_regs*      _cmu_top      = NULL;
volatile struct cmu_dmc1_regs*     _cmu_dmc1     = NULL;
volatile struct cmu_dmc2_regs*     _cmu_dmc2     = NULL;
volatile struct cmu_cpu1_regs*     _cmu_cpu1     = NULL;
volatile struct cmu_cpu2_regs*     _cmu_cpu2     = NULL;
volatile struct cmu_isp_regs*      _cmu_isp      = NULL;



static struct clock finpll_clk;
static struct clock aoutpll_clk;
static struct clock sclkmpll_clk;
static struct clock sclkepll_clk;
static struct clock sclkvpll_clk;
static struct clock sclkapll_clk;
static struct clock muxcore_clk;
static struct clock divcore_clk;
static struct clock arm_clk;
static struct clock corem0_clk;
static struct clock corem1_clk;
static struct clock cores_clk;
static struct clock periphclk_clk;
static struct clock atclk_clk;
static struct clock pclk_dbg_clk;
static struct clock sclk_mpll_userc_clk;
static struct clock sclkhpm_clk;
static struct clock divcopy_clk;
static struct clock muxhpm_clk;

static clk_t* clks[];
static clk_t* clks[] = {
    [CLK_MASTER]         = &finpll_clk,
    [CLK_MOUTAPLL]       = &aoutpll_clk,
    [CLK_SCLKMPLL]       = &sclkmpll_clk,
    [CLK_SCLKEPLL]       = &sclkepll_clk,
    [CLK_SCLKVPLL]       = &sclkvpll_clk,
    [CLK_SCLKAPLL]       = &sclkapll_clk,
    [MUX_CORE]           = &muxcore_clk,
    [DIV_CORE]           = &divcore_clk,
    [DIV_CORE2]          = &arm_clk,
    [CLK_ACLK_COREM0]    = &corem0_clk,
    [CLK_ACLK_COREM1]    = &corem1_clk,
    [CLK_ACLK_CORES]     = &cores_clk,
    [CLK_PERIPHCLK]      = &periphclk_clk,
    [CLK_ATCLK]          = &atclk_clk,
    [CLK_PCLK_DBG]       = &pclk_dbg_clk,
    [CLK_SCLKMPLL_USERC] = &sclk_mpll_userc_clk,
    [CLK_SCLKHPM]        = &sclkhpm_clk,
    [DIV_COPY]           = &divcopy_clk,
    [MUX_HPM]            = &muxhpm_clk
};



static clk_t*
exynos4_get_clock(clock_sys_t* sys, enum clk_id id)
{
    clk_t* clk = clks[id];
    if (id < 0 || id >= sizeof(clks) / sizeof(*clks)) {
        printf("Invalid clock: %d\n", id);
        assert(!"Invalid clock");
        return NULL;
    }
    clk->clk_sys = sys;
    clk = clk_init(clk);
    return clk;
}

static int
exynos4_gate_enable(clock_sys_t* sys, enum clock_gate gate, enum clock_gate_mode mode){
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
    clock_sys->get_clock = &exynos4_get_clock; 
    clock_sys->gate_enable = &exynos4_gate_enable;
    return 0;
}

void
clk_print_clock_tree(clock_sys_t* sys)
{
    (void)sys;
    clk_t* clk = clks[CLK_MASTER];
    clk_print_tree(clk, "");
}

/* MASTER_CLK */
static freq_t
_finpll_get_freq(clk_t* clk)
{
    return clk->freq;
}

static freq_t
_finpll_set_freq(clk_t* clk, freq_t hz)
{
    /* Master clock frequency is fixed */
    (void)hz;
    return clk_get_freq(clk);
}

static void
_finpll_recal(clk_t* clk)
{
    assert(0);
}

static clk_t*
_finpll_init(clk_t* clk)
{
    /* TODO: Read source from boot switch/qpio */
#if 1
    clk->freq = XUSBXTI_FREQ;
#else
    clk->freq = XXTI_FREQ
#endif
    return clk;
}

static struct clock finpll_clk = {
    .id = CLK_MASTER,
    CLK_OPS(finpll),
    .freq = 0 * MHZ,
    .priv = NULL,
    .parent = NULL,
    .sibling = NULL,
    .child = NULL
};

static freq_t
_pll_get_freq(clk_t* clk)
{
    volatile struct pll_regs *regs;
    uint32_t mux;
    switch (clk->id) {
    case CLK_MOUTAPLL:
        regs = (volatile struct pll_regs*)&_cmu_cpu1->apll_lock;
        mux = _cmu_cpu1->clk_mux_stat_cpu >> 0;
        break;
    case CLK_SCLKMPLL:
        regs = (volatile struct pll_regs*)&_cmu_dmc1->mpll_lock;
        mux = _cmu_dmc1->clk_mux_stat_dmc >> 12;
        break;
    case CLK_SCLKEPLL:
        regs = (volatile struct pll_regs*)&_cmu_top->epll_lock;
        mux = _cmu_top->clk_mux_stat_top0 >> 4;
        break;
    case CLK_SCLKVPLL:
        regs = (volatile struct pll_regs*)&_cmu_top->vpll_lock;
        mux = _cmu_top->clk_mux_stat_top0 >> 8;
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

static struct clock aoutpll_clk = {
    .id = CLK_MOUTAPLL,
    PLL_CLK_OPS(aoutpll),
    .freq = 1000 * MHZ,
    .priv = NULL,
};


/* MPLL_CLK */
static struct clock sclkmpll_clk = {
    .id = CLK_SCLKMPLL,
    PLL_CLK_OPS(sclkmpll),
    .freq = 800 * MHZ,
    .priv = NULL,
};

/* EPLL_CLK */
static struct clock sclkepll_clk = {
    .id = CLK_SCLKMPLL,
    PLL_CLK_OPS(sclkepll),
    .freq = 800 * MHZ,
    .priv = NULL,
};

/* VPLL_CLK */
static struct clock sclkvpll_clk = {
    .id = CLK_SCLKMPLL,
    PLL_CLK_OPS(sclkvpll),
    .freq = 800 * MHZ,
    .priv = NULL,
};



static freq_t
_div_get_freq(clk_t* clk)
{
    uint32_t div = 1;
    uint32_t fin = clk_get_freq(clk->parent);
    assert(_cmu_cpu1);
    switch (clk->id) {
        /* CPU 0 */
    case DIV_CORE2:
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
    case DIV_CORE:
        div = _cmu_cpu1->clk_div_cpu0 >> 0;
        break;
        /* CPU1 */
    case CLK_ACLK_CORES:
        div = _cmu_cpu1->clk_div_cpu1 >> 8;
        break;
    case CLK_SCLKHPM:
        div = _cmu_cpu1->clk_div_cpu1 >> 4;
        break;
    case DIV_COPY:
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
    case DIV_CORE2:
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
    case DIV_CORE:
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
    case DIV_COPY:
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
    case DIV_CORE:
    case CLK_ATCLK:
        parent = clk_get_clock(clk_get_clock_sys(clk), MUX_CORE);
        break;
    case DIV_CORE2:
        parent = clk_get_clock(clk_get_clock_sys(clk), DIV_CORE);
        break;
    case CLK_ACLK_COREM0:
    case CLK_ACLK_CORES:
    case CLK_ACLK_COREM1:
    case CLK_PERIPHCLK:
        parent = clk_get_clock(clk_get_clock_sys(clk), DIV_CORE2);
        break;
    case CLK_PCLK_DBG:
        parent = clk_get_clock(clk_get_clock_sys(clk), CLK_ATCLK);
        break;
    case CLK_SCLKHPM:
        parent = clk_get_clock(clk_get_clock_sys(clk), DIV_COPY);
        break;
    case DIV_COPY:
        parent = clk_get_clock(clk_get_clock_sys(clk), MUX_HPM);
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

static struct clock sclkapll_clk = {
    .id = CLK_SCLKAPLL,
    DIV_CLK_OPS(sclkapll),
    .freq = 1000 * MHZ,
    .priv = NULL,
};

static struct clock divcore_clk = {
    .id = DIV_CORE,
    DIV_CLK_OPS(DIVcore),
    .freq = 1000 * MHZ,
    .priv = NULL,
};

static struct clock arm_clk = {
    .id = DIV_CORE2,
    DIV_CLK_OPS(armclk),
    .freq = 1000 * MHZ,
    .priv = NULL,
};

static struct clock corem0_clk = {
    .id = CLK_ACLK_COREM0,
    DIV_CLK_OPS(aclk_corem0),
    .freq = 333 * MHZ,
    .priv = NULL,
};

static struct clock corem1_clk = {
    .id = CLK_ACLK_COREM1,
    DIV_CLK_OPS(aclk_corem1),
    .freq = 166 * MHZ,
    .priv = NULL,
};

static struct clock cores_clk = {
    .id = CLK_ACLK_CORES,
    DIV_CLK_OPS(aclk_cores),
    .freq = 250 * MHZ,
    .priv = NULL,
};

static struct clock periphclk_clk = {
    .id = CLK_PERIPHCLK,
    DIV_CLK_OPS(periphclk),
    .freq = 125 * MHZ,
    .priv = NULL,
};

static struct clock atclk_clk = {
    .id = CLK_ATCLK,
    DIV_CLK_OPS(atclk),
    .freq = 200 * MHZ,
    .priv = NULL,
};

static struct clock pclk_dbg_clk = {
    .id = CLK_PCLK_DBG,
    DIV_CLK_OPS(pclk_dbg),
    .freq = 100 * MHZ,
    .priv = NULL,
};

static struct clock sclkhpm_clk = {
    .id = CLK_SCLKHPM,
    DIV_CLK_OPS(sclk_hpm),
    .freq = 200 * MHZ,
    .priv = NULL
};

static struct clock divcopy_clk = {
    .id = DIV_COPY,
    DIV_CLK_OPS(DIVcopy),
    .freq = 200 * MHZ,
    .priv = NULL
};



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
        mux = _cmu_cpu1->clk_mux_stat_cpu >> 24;
        parent_id[0] = CLK_MASTER;
        parent_id[1] = CLK_SCLKMPLL;
        break;
    case MUX_HPM:
        mux = _cmu_cpu1->clk_mux_stat_cpu >> 20;
        parent_id[0] = CLK_MOUTAPLL;
        parent_id[1] = CLK_SCLKMPLL;
        break;
    case MUX_CORE:
        mux = _cmu_cpu1->clk_mux_stat_cpu >> 16;
        parent_id[0] = CLK_MOUTAPLL;
        /* Tree says SCLKMPLL_USERC, Table says SCLKMPLL... */
        parent_id[1] = CLK_SCLKMPLL_USERC;
//        parent_id[1] = CLK_SCLKMPLL;
        break;
    default:
        assert(!"Unknown clock id for mux");
        return NULL;
    }
    mux &= CLK_MUXSTAT_MASK;
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


static struct clock sclk_mpll_userc_clk = {
    .id = CLK_SCLKMPLL_USERC,
    MUX_CLK_OPS(sclk_mpll_userc),
    .freq = 800 * MHZ,
    .priv = NULL
};

static struct clock muxcore_clk = {
    .id = MUX_CORE,
    MUX_CLK_OPS(MUXcore),
    .freq = 1000 * MHZ,
    .priv = NULL
};

static struct clock muxhpm_clk = {
    .id = MUX_HPM,
    MUX_CLK_OPS(MUXhpm),
    .freq = 1000 * MHZ,
    .priv = NULL
};

