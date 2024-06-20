/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platsupport/mux.h>
#include <utils/attribute.h>
#include <platsupport/clock.h>

int clock_sys_init(ps_io_ops_t *io_ops, clock_sys_t *clk_sys)
{
    return 0;
}

int mux_sys_init(ps_io_ops_t *io_ops, UNUSED void *dependencies, mux_sys_t *mux)
{
    return 0;
}
