/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdint.h>
#include <platsupport/mux.h>
#include <utils/attribute.h>
#include "../../services.h"

struct apq8064_mux_regs {
    int dummy;
};

static struct apq8064_mux {
    volatile struct apq8064_mux_regs    *mux;
} _mux;

static inline struct apq8064_mux *get_mux_priv(mux_sys_t *mux)
{
    return (struct apq8064_mux *)mux->priv;
}

static inline void set_mux_priv(mux_sys_t *mux, struct apq8064_mux *apq8064_mux)
{
    assert(mux != NULL);
    assert(apq8064_mux != NULL);
    mux->priv = apq8064_mux;
}

static int apq8064_mux_feature_enable(mux_sys_t *mux, mux_feature_t mux_feature, UNUSED enum mux_gpio_dir mgd)
{
    struct apq8064_mux *m;
    if (mux == NULL || mux->priv == NULL) {
        return -1;
    }
    m = get_mux_priv(mux);

    switch (mux_feature) {
    default:
        (void)m;
        return -1;
    }
}

static int apq8064_mux_init_common(mux_sys_t *mux)
{
    set_mux_priv(mux, &_mux);
    mux->feature_enable = &apq8064_mux_feature_enable;
    return 0;
}

int apq8064_mux_init(void *bank1,
                     mux_sys_t *mux)
{
    (void)bank1;
    return apq8064_mux_init_common(mux);
}

int mux_sys_init(ps_io_ops_t *io_ops, UNUSED void *dependencies, mux_sys_t *mux)
{
    (void)io_ops;
    return apq8064_mux_init_common(mux);
}
