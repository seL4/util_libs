/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platsupport/io.h>
#include "devcfg.h"

#define DEVCFG_PADDR   0xF8007000
#define DEVCFG_SIZE    0x1000

static devcfg_regs_t* devcfg_regs = NULL;

int devcfg_init(ps_io_ops_t* ops) {
    if (devcfg_regs == NULL) {
        devcfg_regs = ps_io_map(&ops->io_mapper, DEVCFG_PADDR, DEVCFG_SIZE, false /* cached */, PS_MEM_NORMAL);
    }

    return 0;
}

devcfg_regs_t* devcfg_get_regs(void) {
    return devcfg_regs;
}
