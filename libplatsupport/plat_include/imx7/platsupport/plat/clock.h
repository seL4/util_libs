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

enum clk_id {
    CLK_MASTER,
    CLK_ARM,        /* 800MHz - 1.2GHz             */
    CKL_SYS,        /* 480 MHz, with multiple PFDs */
    CLK_ENET,       /* 630-1300MHz, set to 1000MHz */
    CLK_AUDIO,      /* 650-1300MHz                 */
    CLK_VIDEO,      /* 650-1300MHz                 */
    CLK_DRAM,       /* 800-1066MHz                 */
    NCLOCKS,
};

enum clock_gate {
    NCLKGATES
};
