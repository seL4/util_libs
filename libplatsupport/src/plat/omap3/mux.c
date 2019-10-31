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

struct omap3_mux_regs {
    int dummy;
};

static struct omap3_mux {
    volatile struct omap3_mux_regs*    mux;
} _mux;

static inline struct omap3_mux* get_mux_priv(mux_sys_t* mux) {
    return (struct omap3_mux*)mux->priv;
}

static inline void set_mux_priv(mux_sys_t* mux, struct omap3_mux* omap3_mux)
{
    assert(mux != NULL);
    assert(omap3_mux != NULL);
    mux->priv = omap3_mux;
}

static int
omap3_mux_feature_enable(mux_sys_t* mux, enum mux_feature mux_feature,
                         enum mux_gpio_dir mgd UNUSED)
{
    if (mux == NULL || mux->priv == NULL) {
        return -1;
    }

    struct omap3_mux* m UNUSED = get_mux_priv(mux);

    switch (mux_feature) {
    default:
        return -1;
    }
}

static int
omap3_mux_init_common(mux_sys_t* mux)
{
    set_mux_priv(mux, &_mux);
    mux->feature_enable = &omap3_mux_feature_enable;
    return 0;
}

int
omap3_mux_init(void* bank1 UNUSED,
               mux_sys_t* mux)
{
    return omap3_mux_init_common(mux);
}

int
mux_sys_init(ps_io_ops_t* io_ops UNUSED, void *dependencies UNUSED,
             mux_sys_t* mux)
{
    return omap3_mux_init_common(mux);
}
