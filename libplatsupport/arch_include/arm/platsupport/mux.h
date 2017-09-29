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

#pragma once

typedef struct mux_sys mux_sys_t;

#include <platsupport/plat/mux.h>

struct mux_sys {
    int (*feature_enable)(mux_sys_t* mux, enum mux_feature);
    void *priv;
};

#include <platsupport/io.h>

static inline int mux_sys_valid(const mux_sys_t* mux_sys)
{
    return mux_sys && mux_sys->priv;
}

/**
 * Initialise (IO)MUX sub systems
 * @param[in]  io_ops  collection of IO operations for the subsystem to use.
 * @param[out] mux     On success, this will be filled with the appropriate
 *                     subsystem data.
 * @return             0 on success.
 */
int mux_sys_init(ps_io_ops_t* io_ops, mux_sys_t* mux);

/**
 * Enable a SoC feature via the IO MUX
 * @param[in] mux         A handle to the mux system
 * @param[in] mux_feature A SoC specific feature to enable.
 * @return                0 on success
 */
static inline int mux_feature_enable(mux_sys_t* mux, enum mux_feature mux_feature)
{
    if (mux->feature_enable) {
        return mux->feature_enable(mux, mux_feature);
    } else {
        return -1;
    }
}

