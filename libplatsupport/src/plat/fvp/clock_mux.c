/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <platsupport/mux.h>
#include <utils/attribute.h>
#include <platsupport/clock.h>

int
clock_sys_init(ps_io_ops_t* io_ops, clock_sys_t* clk_sys)
{
    return 0;
}

int
mux_sys_init(ps_io_ops_t* io_ops, UNUSED void *dependencies, mux_sys_t* mux)
{
    return 0;
}
