/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#pragma once

/* this is the misc pinmux */
#define MUX_PADDR_BASE 0x70000000
/* the auxliary pinmux */
#define MUX_AUX_PADDR_BASE 0x70006000

#define GMACFG_ADDR_OFFSET 0x0900

typedef struct mux_sys mux_sys_t;

enum mux_feature {
    NMUX_FEATURES
};

int
tegra_mux_init(volatile void* pinmux_misc,volatile void* pinmux_aux, mux_sys_t* mux);
