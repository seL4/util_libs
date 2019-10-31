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
#include <platsupport/mux.h>
#include <utils/attribute.h>
#include "../../services.h"

struct apq8064_mux_regs {
    int dummy;
};

static struct apq8064_mux {
    volatile struct apq8064_mux_regs*    mux;
} _mux;

static inline struct apq8064_mux* get_mux_priv(const mux_sys_t* mux) {
    return (struct apq8064_mux*)mux->priv;
}

static inline void set_mux_priv(mux_sys_t* mux, struct apq8064_mux* apq8064_mux)
{
    assert(mux != NULL);
    assert(apq8064_mux != NULL);
    mux->priv = apq8064_mux;
}

static int
apq8064_mux_feature_enable(const mux_sys_t* mux, enum mux_feature mux_feature,
                           enum mux_gpio_dir mgd UNUSED)
{
    if (mux == NULL || mux->priv == NULL) {
        return -1;
    }

    struct apq8064_mux* m UNUSED = get_mux_priv(mux);

    switch (mux_feature) {
    default:
        return -1;
    }
}

static int
apq8064_mux_init_common(mux_sys_t* mux)
{
    set_mux_priv(mux, &_mux);
    mux->feature_enable = &apq8064_mux_feature_enable;
    return 0;
}

int
apq8064_mux_init(UNUSED void* bank1,
                 mux_sys_t* mux)
{
    return apq8064_mux_init_common(mux);
}

int
mux_sys_init(UNUSED const ps_io_ops_t* io_ops, void *dependencies UNUSED,
             mux_sys_t* mux)
{
    return apq8064_mux_init_common(mux);
}
