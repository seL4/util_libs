/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include <platsupport/mux.h>

static int
pc99_mux_feature_enable(mux_sys_t* mux, enum mux_feature mux_feature)
{
    return 0;
}

int
mux_sys_init(ps_io_ops_t* io_ops, mux_sys_t* mux)
{
    (void)io_ops;
    mux->feature_enable = &pc99_mux_feature_enable;
    return 0;
}


