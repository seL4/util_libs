/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include "../../arch/arm/clock.h"
#include "../../services.h"
#include <assert.h>
#include <string.h>
#include <utils/util.h>


static volatile struct clock_regs {
    int dummy;
} clk_regs;


static struct clock master_clk = { CLK_OPS_DEFAULT(MASTER) };

static int
omap3_gate_enable(clock_sys_t* clock_sys, enum clock_gate gate, enum clock_gate_mode mode)
{
    return -1;
}

int
clock_sys_init(ps_io_ops_t* o, clock_sys_t* clock_sys)
{
    clock_sys->priv = (void*)&clk_regs;
    clock_sys->get_clock = &ps_get_clock;
    clock_sys->gate_enable = &omap3_gate_enable;
    return 0;
}

void
clk_print_clock_tree(clock_sys_t* sys)
{
    clk_t *clk = clk_get_clock(sys, CLK_MASTER);
    clk_print_tree(clk, "");
}

clk_t* ps_clocks[] = {
    [CLK_MASTER]   = &master_clk,
};

freq_t ps_freq_default[] = {
    [CLK_MASTER]   = 24 * MHZ,
};


