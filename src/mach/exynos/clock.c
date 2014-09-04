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
#define BITS_PER_DIV 4

static inline volatile struct clk_regs*
clk_sys_get_clk_regs(clock_sys_t* clock_sys, int idx) {
    struct clk_regs** clk_regs_ptr = (struct clk_regs**)clock_sys->priv;
    return clk_regs_ptr[idx];
};

#if 0
/******** Clock dividers *********/
static inline int
clk_div_get_priv(clk_t* clk)
{
    return (int)clk->priv;
}

freq_t
_div_clk_get_freq(clk_t* clk)
{
    struct div_priv* div_data;
    volatile struct clk_regs* clk_regs;
    freq_t fin;
    uint32_t v;
    int c, r, o;
    int div;
    div_data = clk_div_get_priv(clk);
    fin = clk_get_freq(clk->parent);

    c = DIVID_GET_CMU(div_data->div_id);
    r = DIVID_GET_IDX(div_data->div_id);
    o = DIVID_GET_OFFSET(div_data->div_id);

    clk_regs = clk_sys_get_clk_regs(div_data->clock_sys, c);
    v = clk_regs->div[r];
    v >>= (o * BITS_PER_DIV);
    div = (v & MASK(BITS_PER_DIV)) + 1;
    return fin / div;
}

freq_t
_div_clk_set_freq(clk_t* clk, freq_t hz)
{
    (void)hz;
    return clk_get_freq(clk);
}

void
_div_clk_recal(clk_t* clk)
{
    assert(!"Not implemented");
}
#endif
