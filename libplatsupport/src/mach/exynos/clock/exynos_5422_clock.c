/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
#include "../clock.h"

uint32_t exynos_pll_get_freq(const clk_t* clk, int clkid, uint32_t pll_idx)
{
    uint32_t v, p, m, s;
    clk_regs_io_t** clk_regs;
    int c, r, o;

    clkid_decode(clkid, &c, &r, &o);
    clk_regs = clk_get_clk_regs(clk);

    v = clk_regs[c]->pll_con[pll_idx];
    exynos_mpll_get_pms(v, &p, &m, &s);

    return exynos_pll_calc_freq((uint64_t)clk_get_freq(clk->parent), p, m, s);
}
