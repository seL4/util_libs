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

#include <stdint.h>
#include <utils/attribute.h>
#include <platsupport/mux.h>
#include "../../services.h"

/* Zynq MUX is on the SLCR, along with just about everything else. */
struct slcr;

static struct zynq_mux {
    struct slcr *slcr;
} _mux;

static inline void set_mux_priv(
    mux_sys_t *mux,
    struct zynq_mux *zynq_mux)
{
    assert(mux != NULL);
    assert(zynq_mux != NULL);
    mux->priv = zynq_mux;
}

static int zynq_mux_feature_enable(
    const mux_sys_t *mux,
    mux_feature_t mux_feature,
    UNUSED enum mux_gpio_dir mgd)
{
    return 0;
}

static int zynq_mux_init_common(
    mux_sys_t *mux)
{
    set_mux_priv(mux, &_mux);
    mux->feature_enable = &zynq_mux_feature_enable;
    return 0;
}

int zynq_mux_init(
    mux_sys_t *mux)
{
    return zynq_mux_init_common(mux);
}

int mux_sys_init(
    ps_io_ops_t *io_ops,
    UNUSED void *dependencies,
    mux_sys_t *mux)
{
    return zynq_mux_init_common(mux);
}
