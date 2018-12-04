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

#include "clock.h"

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
    if (fin / 1 < hz) {
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

    exynos_cmu_set_div(clk_regs, clkid, 1, div);
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
    uint32_t v, p, m, s;
    uint32_t muxstat;
    clk_regs = clk_get_clk_regs(clk);
    pll_priv = exynos_clk_get_priv_pll(clk);
    clkid = pll_priv->clkid;
    pll_idx = pll_priv->pll_offset;
    clkid_decode(clkid, &c, &r, &o);

    if(config_set(CONFIG_PLAT_EXYNOS5422)) {
        v = clk_regs[c]->pll_con[pll_idx];
    } else {
        pll_regs = (volatile struct pll_regs*)&clk_regs[c]->pll_lock[pll_idx];
        muxstat = exynos_cmu_get_srcstat(clk_regs, clkid);
        if (muxstat & 0x1) {
            /* Muxed or bypassed to FINPLL */
            return clk_get_freq(clk->parent);
        } else {
            v = pll_regs->con0 & PLL_MPS_MASK;
        }
    }

    m = (v >> 16) & 0x1ff;
    p = (v >>  8) &  0x3f;
    s = (v >>  0) &   0x7;

    return ((uint64_t)clk_get_freq(clk->parent) * m / p) >> s;
}

freq_t
_pll_set_freq(clk_t* clk, freq_t hz)
{
    volatile struct pll_regs* pll_regs;
    const struct pll_priv* pll_priv;
    clk_regs_io_t** clk_regs;
    struct mpsk_tbl *tbl;
    int tbl_size;
    int mhz = hz / (1 * MHZ);
    uint32_t mps, k;
    uint32_t con0, con1;
    int i;
    int clkid, c, r, o, pll_idx;

    /* get clk regs address and clkid */
    clk_regs = clk_get_clk_regs(clk);
    pll_priv = exynos_clk_get_priv_pll(clk);
    clkid = pll_priv->clkid;
    pll_idx = pll_priv->pll_offset;
    clkid_decode(clkid, &c, &r, &o);
    /* prepare searching the correct frequency parameter */
    tbl = pll_priv->tbl;
    tbl_size = pll_priv->pll_tbl_size;
    pll_regs = (volatile struct pll_regs*)&clk_regs[c]->pll_lock[pll_idx];
    /* Search the table for an appropriate frequency value and get parameters */
    mps = tbl[tbl_size - 1].mps;
    k   = tbl[tbl_size - 1].k;
    for (i = 0; i < tbl_size; i++) {
        if (tbl[i].mhz >= mhz) {
            mps = tbl[i].mps;
            k = tbl[i].k;
            break;
        }
    }
    /* set PLL_FOUT to XXTI and bypass PLL */
    exynos_cmu_set_src(clk_regs, clkid, 0x0);
    /* updating involved bits in con0 and con1 regs */
    con0 = pll_regs->con0 & ~PLL_MPS_MASK;
    con1 = pll_regs->con1 & ~PLL_K_MASK;
    if (pll_priv->type == PLLTYPE_MPSK) {
        pll_regs->con1 = (con1 | k);
    }
    pll_regs->con0 = (con0 | mps | PLL_ENABLE);
    while (!(pll_regs->con0 & PLL_LOCKED));
    /* PLL is configured, set PLL_FOUT to PLL */
    exynos_cmu_set_src(clk_regs, clkid, 0x1);

    return clk_get_freq(clk);
}

void
_pll_recal(clk_t* clk)
{
    assert(0);
}

clk_t*
_pll_init(clk_t* clk)
{
    clk_t* parent = clk_get_clock(clk_get_clock_sys(clk), CLK_MASTER);
    clk_init(parent);
    clk_register_child(parent, clk);
    return clk;
}
