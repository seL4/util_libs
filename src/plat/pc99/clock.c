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

static clk_t*
imx6_get_clock(clock_sys_t* sys, enum clk_id id)
{
    (void)sys;
    (void)id;
    return NULL;
}

static int
imx6_gate_enable(clock_sys_t* clock_sys, enum clock_gate gate, enum clock_gate_mode mode)
{
    (void)clock_sys;
    (void)gate;
    (void)mode;
    return 0;
}

int
clock_sys_init(ps_io_ops_t* o, clock_sys_t* clock_sys){
    clock_sys->priv = NULL;
    clock_sys->get_clock = &imx6_get_clock;
    clock_sys->gate_enable = &imx6_gate_enable;
    return 0;
}

void
clk_print_clock_tree(clock_sys_t* sys)
{
    (void)sys;
}

