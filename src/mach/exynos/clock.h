/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef MACH_CLOCK_H
#define MACH_CLOCK_H

#include "../../arch/arm/clock.h"

/* CLKID is used to decode the register bank and offset of a particular generic clock */
#define CLKID(cmu, reg, offset)  ((CLKREGS_##cmu) << 10 | (reg) << 3 | (offset) << 0)
#define CLKID_GET_OFFSET(x)      ((x) & 0x7)
#define CLKID_GET_IDX(x)         (((x) >> 3) & 0x3f)
#define CLKID_GET_CMU(x)         (((x) >> 10) & 0xf)

#define CLK_SRC_BITS         4
#define CLK_SRCSTAT_BITS     4
#define CLK_SRCMASK_BITS     4
#define CLK_DIV_BITS         4
#define CLK_DIVSTAT_BITS     4
#define CLK_GATE_BITS        4

/* SRC STAT */
#define CLK_SRCSTAT_SHIFT(x) ((x)*CLK_MASK_BITS)
#define CLK_SRCSTAT_MASK     ((1 << CLK_MASK_SHIFT(1)) - 1)
#define CLK_SRCSTAT_CHANGING (0x4)


/* CLK MASK */
#define CLK_MASK_SHIFT(x) ((x)*CLK_MASK_BITS)
#define CLK_MASK_MASK     ((1 << CLK_MASK_SHIFT(1)) - 1)
#define CLK_MASK_SET      (0x00)
#define CLK_MASK_CLEAR    (0x01)

/* CLK DIV */
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

struct clk_regs {
    volatile uint32_t pll_lock[64];   /* 0x000 */
    volatile uint32_t pll_con[64];    /* 0x100 */
    volatile uint32_t src[64];        /* 0x200 */
    volatile uint32_t srcmask[64];    /* 0x300 */
    volatile uint32_t srcstat[64];    /* 0x400 */
    volatile uint32_t div[64];        /* 0x500 */
    volatile uint32_t divstat[128];   /* 0x600 */
    volatile uint32_t gate[64];       /* 0x800 */
    volatile uint32_t clkout;         /* 0xA00 */
    volatile uint32_t clkout_divstat; /* 0xA04 */
};

/* 0x10034000 */
struct cmu_leftbus_regs {
    uint32_t res0[128];
    uint32_t clk_src_leftbus;              /* 0x200 */
    uint32_t res1[127];
    uint32_t clk_src_stat_leftbus;         /* 0x400 */
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
    uint32_t clk_src_stat_rightbus;        /* 0x400 */
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
    uint32_t clk_src_stat_top0;       /* 0x410 */
    uint32_t clk_src_stat_top1;       /* 0x414 */
    uint32_t res12[4];
    uint32_t clk_src_stat_mfc;        /* 0x428 */
    uint32_t clk_src_stat_g3d;        /* 0x42c */
    uint32_t res13[10];
    uint32_t clk_src_stat_cam1;       /* 0x458 */
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
    uint32_t clk_src_stat_dmc;        /* 0x400 */
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
    uint32_t clk_src_stat_cpu;        /* 0x400 */
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

static inline int
exynos_clk_get_priv_id(clk_t* clk)
{
    return (int)clk->priv;
}

/* Generic exynos deviders */
freq_t _div_clk_get_freq(clk_t* clk);
freq_t _div_clk_set_freq(clk_t* clk, freq_t hz);
void   _div_clk_recal(clk_t* clk);
clk_t* _div_clk_init(clk_t* clk);


/**** helpers ****/
static inline void
clkid_decode(int clkid, int* cmu, int* reg, int* off)
{
    *cmu = CLKID_GET_CMU(clkid);
    *reg = CLKID_GET_IDX(clkid);
    *off = CLKID_GET_OFFSET(clkid);
}

static inline int
clkbf_get(volatile uint32_t* reg, int start_bit, int nbits)
{
    uint32_t v;
    v = *reg;
    return (v >> start_bit) & MASK(nbits);
}

static inline void
clkbf_set(volatile uint32_t* reg, int start_bit, int nbits, int v)
{
    uint32_t o;
    v <<= start_bit;
    o = *reg & ~(MASK(nbits) << start_bit);
    *reg = o | v;
}


static inline int
exynos_cmu_get_src(struct clk_regs** regs, int clkid)
{
    int c, r, o;
    clkid_decode(clkid, &c, &r, &o);
    return clkbf_get(&regs[c]->src[r], o * CLK_SRC_BITS, CLK_SRC_BITS);
}

static inline void
exynos_cmu_set_src(struct clk_regs** regs, int clkid, int src)
{
    int c, r, o;
    clkid_decode(clkid, &c, &r, &o);
    /* Configure source */
    clkbf_set(&regs[c]->src[r], o * CLK_SRC_BITS, CLK_SRC_BITS, src);
    /* Unmask the source */
    clkbf_set(&regs[c]->srcmask[r], o * CLK_SRCMASK_BITS, CLK_SRCMASK_BITS, 1);
}

static inline int
exynos_cmu_get_div(struct clk_regs** regs, int clkid, int span)
{
    int c, r, o;
    clkid_decode(clkid, &c, &r, &o);
    return clkbf_get(&regs[c]->src[r], o * CLK_DIV_BITS, CLK_DIV_BITS * span);
}

static inline void
exynos_cmu_set_div(struct clk_regs** regs, int clkid, int div, int span)
{
    int c, r, o;
    clkid_decode(clkid, &c, &r, &o);
    clkbf_set(&regs[c]->div[r], o * CLK_DIV_BITS, CLK_DIV_BITS * span, div);
    /* Wait for changes to take affect */
    while (clkbf_get(&regs[c]->divstat[r], o * CLK_DIV_BITS, CLK_DIV_BITS));
}


static inline int
exynos_cmu_get_gate(struct clk_regs** regs, int clkid)
{
    int c, r, o;
    clkid_decode(clkid, &c, &r, &o);
    return clkbf_get(&regs[c]->src[r], o * CLK_GATE_BITS, CLK_GATE_BITS);
}

static inline void
exynos_cmu_set_gate(struct clk_regs** regs, int clkid, int v)
{
    int c, r, o;
    clkid_decode(clkid, &c, &r, &o);
    clkbf_set(&regs[c]->div[r], o * CLK_GATE_BITS, CLK_GATE_BITS, v);
}

#endif /* MACH_CLOCK_H */
