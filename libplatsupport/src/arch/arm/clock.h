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

#ifndef ARCH_CLOCK_H
#define ARCH_CLOCK_H


#include <assert.h>
#include <platsupport/clock.h>

#define _CLK_OPS(_id, _name, ops, data) \
        .id       = _id,                \
        .name     = _name,              \
        .get_freq = _##ops##_get_freq,  \
        .set_freq = _##ops##_set_freq,  \
        .recal    = _##ops##_recal,     \
        .init     = _##ops##_init,      \
        .parent   = NULL,               \
        .sibling  = NULL,               \
        .child    = NULL,               \
        .req_freq = 0,                  \
        .priv     = (void*)data

#define CLK_OPS(name, ops, data)        _CLK_OPS(CLK_##name, #name, ops, data)
#define CLK_OPS_CUSTOM(name, ops, data) _CLK_OPS(CLK_CUSTOM, name, ops, data)
#define CLK_OPS_DEFAULT(clk_id)         CLK_OPS(clk_id, default_clk, NULL)


/* Array of default frequencies */
extern freq_t ps_freq_default[];

/* Array of clocks */
extern clk_t* ps_clocks[];

/** Helpers **/
static inline clock_sys_t*
clk_get_clock_sys(clk_t* clk)
{
    assert(clk);
    return clk->clk_sys;
}

static inline clk_t*
clk_init(clk_t* clk)
{
    assert(clk);
    assert(clk->init);
    return clk->init(clk);
}

static inline void
clk_recal(clk_t* clk)
{
    assert(clk);
    assert(clk->recal);
    clk->recal(clk);
}


/**
 * Prints a clock tree.
 * The prefix should be based on the depth of the current root. When calling this
 * funtion for the first time, pass "" for prefix. The prefix will be ammended
 * for the current depth of traversal.
 * @param[in] clk     The root of the tree
 * @param[in] prefix  A string prefix to print before each line
 */
void clk_print_tree(clk_t* clk, const char* prefix);


/* Default clocks - Frequency must be defined in freq_default */
freq_t _default_clk_get_freq(clk_t* clk);
freq_t _default_clk_set_freq(clk_t* clk, freq_t hz);
void   _default_clk_recal(clk_t* clk);
clk_t* _default_clk_init(clk_t* clk);

/* Generic clock acquisition for all platforms */
clk_t* ps_get_clock(clock_sys_t* sys, enum clk_id id);

#endif /* ARCH_CLOCK_H */
