/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <assert.h>
#include <platsupport/clock.h>


#define CLK_OPS(clk) \
        .get_freq = _##clk##_get_freq, \
        .set_freq = _##clk##_set_freq, \
        .recal    = _##clk##_recal,    \
        .init     = _##clk##_init,     \
        .parent   = NULL,              \
        .sibling  = NULL,              \
        .child    = NULL,              \
        .name     = #clk


void clk_recal(clk_t* clk);
void clk_print_tree(clk_t* clk, char* prefix);

static inline clk_t* clk_init(clk_t* clk)
{
    assert(clk);
    assert(clk->init);
    return clk->init(clk);
}

static inline clock_sys_t* clk_get_clock_sys(clk_t* clk){
    assert(clk);
    return clk->clk_sys;
}
