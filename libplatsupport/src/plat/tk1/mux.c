/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <utils/util.h>

#include <platsupport/mux.h>
#include <platsupport/gpio.h>

typedef struct tegra_mux_state {
    volatile void *pinmux_misc;
    volatile void *pinmux_aux;
} tegra_mux_state_t;

static int tegra_mux_init_common(mux_sys_t* mux)
{
    return 0;
}

int
tegra_mux_init(volatile void* pinmux_misc,volatile void* pinmux_aux, mux_sys_t* mux)
{
    tegra_mux_state_t *state = malloc(sizeof(*state));
    if (!state) {
        ZF_LOGE("Failed to malloc");
        return 1;
    }
    state->pinmux_misc = pinmux_misc;
    state->pinmux_aux = pinmux_aux;
    mux->priv = state;

    return tegra_mux_init_common(mux);
}

int
mux_sys_init(ps_io_ops_t *io_ops, mux_sys_t *mux)
{
    tegra_mux_state_t *state = malloc(sizeof(*state));
    if (!state) {
        ZF_LOGE("Failed to malloc");
        return 1;
    }
    state->pinmux_misc = (volatile void*)ps_io_map(&io_ops->io_mapper, MUX_PADDR_BASE, PAGE_SIZE_4K, 0, PS_MEM_NORMAL);
    if (!state->pinmux_misc) {
        ZF_LOGE("Failed to map pinmux page");
        free(state);
        return 1;
    }
    state->pinmux_aux = (volatile void*)ps_io_map(&io_ops->io_mapper, MUX_AUX_PADDR_BASE, PAGE_SIZE_4K, 0, PS_MEM_NORMAL);
    if (!state->pinmux_aux) {
        ZF_LOGE("Failed to map auxialiary pinmux page");
        ps_io_unmap(&io_ops->io_mapper, (void*)state->pinmux_misc, PAGE_SIZE_4K);
        free(state);
        return 1;
    }
    mux->priv = state;
    return tegra_mux_init_common(mux);
}
