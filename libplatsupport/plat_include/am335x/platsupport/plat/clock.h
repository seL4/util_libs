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

#ifndef _PLATSUPPORT_PLAT_CLOCK_H_
#define _PLATSUPPORT_PLAT_CLOCK_H_

enum clk_id {
    CLK_MASTER,
    /* ----- */
    NCLOCKS,
    /* Custom clock */
    CLK_CUSTOM,
};

enum clock_gate {
    NCLKGATES
};

#endif /* _PLATSUPPORT_PLAT_CLOCK_H_ */
