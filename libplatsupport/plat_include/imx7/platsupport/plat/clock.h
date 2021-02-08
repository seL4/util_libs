/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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
