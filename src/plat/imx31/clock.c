/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include "../../clock.h"
#include "../../services.h"
#include <assert.h>
#include <string.h>
#include <utils/util.h>


static volatile struct clock_regs {
    int dummy;
} clk_regs;

static struct clock master_clk;

static clk_t* clks[] = {
    [CLK_MASTER]   = &master_clk,
};

static const freq_t freq_default[] = {
    [CLK_MASTER]   = 24 * MHZ,
};

static clk_t*
imx31_get_clock(clock_sys_t* sys, enum clk_id id)
{
    clk_t* clk = clks[id];
    assert(clk);
    clk->clk_sys = sys;
    return clk_init(clk);
}

static int
imx31_gate_enable(clock_sys_t* clock_sys, enum clock_gate gate, enum clock_gate_mode mode)
{
    return -1;
}

int
clock_sys_init(ps_io_ops_t* o, clock_sys_t* clock_sys){
    clock_sys->priv = (void*)&clk_regs;
    clock_sys->get_clock = &imx31_get_clock;
    clock_sys->gate_enable = &imx31_gate_enable;
    return 0;
}

void
clk_print_clock_tree(clock_sys_t* sys)
{
    clk_t *clk = clk_get_clock(sys, CLK_MASTER);
    clk_print_tree(clk, "");
    assert(!"Not implemented");
}


/* MASTER_CLK */
static freq_t
_master_get_freq(clk_t* clk)
{
    return clk->freq;
}

static freq_t
_master_set_freq(clk_t* clk, freq_t hz)
{
    /* Master clock frequency is fixed */
    (void)hz;
    return clk_get_freq(clk);
}

static void
_master_recal(clk_t* clk UNUSED)
{
    assert(0);
}

static clk_t*
_master_init(clk_t* clk)
{
    if (clk->priv == NULL) {
        clk->priv = (void*)&clk_regs;
    }
    clk->freq = freq_default[clk->id];
    return clk;
}

static struct clock master_clk = {
    .id = CLK_MASTER,
    CLK_OPS(master),
    .freq = 0,
    .priv = NULL,
};
