/*
 * Copyright 2017, DornerWorks
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DORNERWORKS_BSD)
 */
/*
 * This data was produced by DornerWorks, Ltd. of Grand Rapids, MI, USA under
 * a DARPA SBIR, Contract Number D16PC00107.
 *
 * Approved for Public Release, Distribution Unlimited.
 */

#include "../../arch/arm/clock.h"
#include "../../services.h"
#include <assert.h>
#include <string.h>
#include <utils/util.h>

static struct clock cpu_1x_clk = { CLK_OPS_DEFAULT(CPU_1X) };

void clk_print_clock_tree(const clock_sys_t* sys)
{
   clk_t *clk = clk_get_clock(sys, CLK_CPU_1X);
   clk_print_tree(clk, "");
}

clk_t* ps_clocks[] =
{
   [CLK_CPU_1X]    = &cpu_1x_clk,
};

freq_t ps_freq_default[] =
{
   [CLK_CPU_1X]    =  1100 * MHZ,
};
