/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */



#include "clock.h"

#define DIV_SEP_BITS 4
#define DIV_VAL_BITS 4

/* CON 0 */
#define PLL_PMS_MASK    PLL_PMS(0x3f, 0x1ff, 0x3)
#define PLL_ENABLE      BIT(31)
#define PLL_LOCKED      BIT(29)
/* CON1 */
#define PLL_BYPASS      BIT(22)



/***********
 *** DIV ***
 ***********/

freq_t
_div_get_freq(clk_t* clk)
{
    clk_regs_io_t** clk_regs;
    uint32_t div;
    uint32_t fin;
    int clkid;
    clkid = exynos_clk_get_priv_id(clk);
    clk_regs = clk_get_clk_regs(clk);
    fin = clk_get_freq(clk->parent);
    div = exynos_cmu_get_div(clk_regs, clkid, 1);
    return fin / (div + 1);
}

freq_t
_div_set_freq(clk_t* clk, freq_t hz)
{
    clk_regs_io_t** clk_regs;
    uint32_t div;
    uint32_t fin;
    int clkid;
    clkid = exynos_clk_get_priv_id(clk);
    clk_regs = clk_get_clk_regs(clk);
    fin = clk_get_freq(clk->parent);
    if (fin / 1 < hz){
        /* Parent frequency too low */
        fin = clk_set_freq(clk->parent, hz * (MASK(DIV_VAL_BITS) / 2 + 1));
    }
    if (fin / (MASK(DIV_VAL_BITS) + 1) > hz) {
        /* Parent frequency too high */
        fin = clk_set_freq(clk->parent, hz * (MASK(DIV_VAL_BITS) / 2 + 1));
    }
    div = fin / hz;
    if (div > MASK(DIV_VAL_BITS)) {
        /* This should have been caught by now, but best to protect against overflow */
        div = MASK(CLK_DIV_BITS);
    }

    exynos_cmu_set_div(clk_regs, clkid, div, 1);
    return clk_get_freq(clk);
}

void
_div_recal(clk_t* clk)
{
    assert(0);
}

/***********
 *** PLL ***
 ***********/

freq_t
_pll_get_freq(clk_t* clk)
{
    clk_regs_io_t** clk_regs;
    volatile struct pll_regs* pll_regs;
    const struct pll_priv* pll_priv;
    int clkid, c, r, o, pll_idx;
    uint32_t muxstat;
    clk_regs = clk_get_clk_regs(clk);
    pll_priv = exynos_clk_get_priv_pll(clk);
    clkid = pll_priv->clkid;
    pll_idx = pll_priv->pll_offset;
    clkid_decode(clkid, &c, &r, &o);

    pll_regs = (volatile struct pll_regs*)&clk_regs[c]->pll_lock[pll_idx];
    muxstat = exynos_cmu_get_srcstat(clk_regs, clkid);

    if ((pll_regs->con1) & PLL_BYPASS || (muxstat & 0x1)) {
        /* Muxed or bypassed to FINPLL */
        return clk_get_freq(clk->parent);
    } else {
        uint32_t v, p, m, s;
        v = pll_regs->con0 & PLL_PMS_MASK;
        m = (v >> 16) & 0x1ff;
        p = (v >>  8) &  0x3f;
        s = (v >>  0) &   0x3;
        return ((uint64_t)clk_get_freq(clk->parent) * m / p) >> s;
    }
}

freq_t
_pll_set_freq(clk_t* clk, freq_t hz)
{
    volatile struct pll_regs* pll_regs;
    const struct pll_priv* pll_priv;
    clk_regs_io_t** clk_regs;
    struct pms_tbl *tbl;
    int tbl_size;
    int mhz = hz / (1 * MHZ);
    uint32_t pms;
    int i;
    int clkid, c, r, o;
    clk_regs = clk_get_clk_regs(clk);
    pll_priv = exynos_clk_get_priv_pll(clk);
    clkid = pll_priv->clkid;
    clkid_decode(clkid, &c, &r, &o);

    tbl = pll_priv->tbl;
    tbl_size = pll_priv->pll_tbl_size;
    pll_regs = (volatile struct pll_regs*)&clk_regs[c]->pll_lock[r];

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

void
_pll_recal(clk_t* clk)
{
    assert(0);
}
