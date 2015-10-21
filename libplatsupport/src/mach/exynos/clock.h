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

#define PLL_PRIV(pll, _type, _tbl) {            \
            .tbl = &_tbl[0],                    \
            .type = PLLTYPE_##_type,            \
            .pll_tbl_size = ARRAY_SIZE(_tbl),   \
            .pll_offset = OFFSET_##pll,         \
            .clkid = CLKID_##pll                \
        }


#define CLK_SRC_BITS         4
#define CLK_SRCSTAT_BITS     4
#define CLK_SRCMASK_BIT      1
#define CLK_SRCMASK_BITS     4
#define CLK_SRCMASK_ENABLE   1
#define CLK_SRCMASK_DISABLE  0
#define CLK_DIV_BITS         4
#define CLK_DIVSTAT_BIT      1
#define CLK_DIVSTAT_BITS     4
#define CLK_GATE_BITS        4

#define CLK_DIVSTAT_STABLE   0x0
#define CLK_DIVSTAT_UNSTABLE 0x1

#define CLK_GATE_SKIP        0x0
#define CLK_GATE_PASS        0x1


#define PLL_MPS(m,p,s)       (((m) << 16) | ((p) << 8) | ((s) << 0))

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
typedef volatile struct clk_regs clk_regs_io_t;

struct mpsk_tbl {
    uint32_t mhz;
    uint32_t mps;
    uint32_t k;
};

enum pll_tbl_type {
    PLLTYPE_MPS,
    PLLTYPE_MPSK
};

struct pll_priv {
    struct mpsk_tbl* tbl;
    enum pll_tbl_type type;
    int pll_tbl_size;
    int pll_offset;
    int clkid;
};


static inline clk_regs_io_t**
clk_sys_get_clk_regs(clock_sys_t* clock_sys)
{
    clk_regs_io_t** clk_regs_ptr = (clk_regs_io_t**)clock_sys->priv;
    return clk_regs_ptr;
};

static inline clk_regs_io_t**
clk_get_clk_regs(clk_t* clk)
{
    return clk_sys_get_clk_regs(clk->clk_sys);
};


static inline const struct pll_priv*
exynos_clk_get_priv_pll(clk_t* clk) {
    return (const struct pll_priv*)clk->priv;
}

static inline int
exynos_clk_get_priv_id(clk_t* clk)
{
    return (int)clk->priv;
}

/* Generic exynos devider */
freq_t _div_get_freq(clk_t* clk);
freq_t _div_set_freq(clk_t* clk, freq_t hz);
void   _div_recal(clk_t* clk);
/* Generic exynos PLL */
freq_t _pll_get_freq(clk_t* clk);
freq_t _pll_set_freq(clk_t* clk, freq_t hz);
void   _pll_recal(clk_t* clk);
clk_t* _pll_init(clk_t* clk);


/**** helpers ****/
static inline void
clkid_decode(int clkid, int* cmu, int* reg, int* off)
{
    *cmu = CLKID_GET_CMU(clkid);
    *reg = CLKID_GET_IDX(clkid);
    *off = CLKID_GET_OFFSET(clkid);
}

static inline int
clkid_change_reg(int clkid, int change)
{
    int r;
    /* extract reg value and apply change */
    r = CLKID_GET_IDX(clkid) + (change);
    /* erase old reg value out of clkid */
    clkid &= ~(0x3f << 3);
    /* return clkid with new reg offset value */
    return clkid | ((r & 0x3f) << 3);
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
exynos_cmu_get_src(clk_regs_io_t** regs, int clkid)
{
    int c, r, o;
    clkid_decode(clkid, &c, &r, &o);
    return clkbf_get(&regs[c]->src[r], o * CLK_SRC_BITS, CLK_SRC_BITS);
}

static inline int
exynos_cmu_get_srcstat(clk_regs_io_t** regs, int clkid)
{
    int c, r, o;
    clkid_decode(clkid, &c, &r, &o);
    return clkbf_get(&regs[c]->srcstat[r], o * CLK_SRC_BITS, CLK_SRC_BITS);
}


static inline void
exynos_cmu_set_src(clk_regs_io_t** regs, int clkid, int src)
{
    int c, r, o;
    clkid_decode(clkid, &c, &r, &o);
    /* Configure source */
    clkbf_set(&regs[c]->src[r], o * CLK_SRC_BITS, CLK_SRC_BITS, src);
}

static inline void
exynos_cmu_set_src_mask(clk_regs_io_t** regs, int clkid, int val)
{
    int c, r, o;
    clkid_decode(clkid, &c, &r, &o);
    /* Mask / unmask the clock source */
    clkbf_set(&regs[c]->srcmask[r], o * CLK_SRCMASK_BITS, CLK_SRCMASK_BIT, val);
}

static inline int
exynos_cmu_get_div(clk_regs_io_t** regs, int clkid, int span)
{
    int c, r, o;
    clkid_decode(clkid, &c, &r, &o);
    return clkbf_get(&regs[c]->div[r], o * CLK_DIV_BITS, CLK_DIV_BITS * span);
}

static inline void
exynos_cmu_set_div(clk_regs_io_t** regs, int clkid, int span, int div)
{
    int c, r, o;
    clkid_decode(clkid, &c, &r, &o);
    clkbf_set(&regs[c]->div[r], o * CLK_DIV_BITS, CLK_DIV_BITS * span, --div);
    /* Wait for changes to take affect */
    while (clkbf_get(&regs[c]->divstat[r], o * CLK_DIVSTAT_BITS, CLK_DIVSTAT_BIT));
}


static inline int
exynos_cmu_get_gate(clk_regs_io_t** regs, int clkid)
{
    int c, r, o;
    clkid_decode(clkid, &c, &r, &o);
    return clkbf_get(&regs[c]->gate[r], o * CLK_GATE_BITS, CLK_GATE_BITS);
}

static inline void
exynos_cmu_set_gate(clk_regs_io_t** regs, int clkid, int v)
{
    int c, r, o;
    clkid_decode(clkid, &c, &r, &o);
    clkbf_set(&regs[c]->gate[r], o * CLK_GATE_BITS, CLK_GATE_BITS, v);
}

#endif /* MACH_CLOCK_H */
