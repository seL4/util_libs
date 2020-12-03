/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <platsupport/i2c.h>

#include "../../arch/arm/clock.h"
#include "../../services.h"
#include <assert.h>
#include <string.h>
#include <utils/util.h>

static struct clock master_clk = { CLK_OPS_DEFAULT(MASTER) };
static struct clock sp804_clk = { CLK_OPS_DEFAULT(SP804) };

int clock_sys_init(ps_io_ops_t *o, clock_sys_t *clock_sys)
{
    clock_sys->priv = (void *)0xdeadbeef;
    clock_sys->get_clock = &ps_get_clock;
    clock_sys->gate_enable = NULL;
    return 0;
}

void clk_print_clock_tree(clock_sys_t *sys)
{
    clk_t *clk = clk_get_clock(sys, CLK_MASTER);
    clk_print_tree(clk, "");
}

clk_t *ps_clocks[] = {
    [CLK_MASTER]   = &master_clk,
    [CLK_SP804]    = &sp804_clk,
};

/* These frequencies are NOT the recommended frequencies. They are to be used
 * when we need to make assumptions about what u-boot has left us with.
 * These values originate from the bcm2837 platform port. According to the
 * BCM2837 TRM in section 14.2. the SP804 clock depends on the apb_clock. The
 * memory bus is controlled by the frequency of the GPU processor core
 * (core_freq). According to
 * https://www.raspberrypi.org/documentation/configuration/config-txt/overclocking.md
 * the bcm2837 has a default core_freq of 400MHz and the bcm2711 has a default
 * core_freq of 500MHz. But according to
 * https://www.raspberrypi.org/forums/viewtopic.php?t=227221#p1393826 the
 * frequency needs to be locked at 250MHz for the bcm2837. In the BCM2711 TRM in
 * section 2.1.1., it is also mentioned that the system clock is 250MHz. Thus,
 * we leave the SP804 Clock at 250MHz for now.
 */
freq_t ps_freq_default[] = {
    [CLK_MASTER]   =  0 * MHZ,
    [CLK_SP804]    = 250 * MHZ,
};
