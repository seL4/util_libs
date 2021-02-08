/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

enum clk_id {
    CLK_MASTER,
    /* PLLS */
    CLK_SCLKMPLL,
    CLK_SCLKEPLL,
    CLK_SCLKVPLL,
    CLK_MOUTAPLL,
    CLK_SCLKAPLL,
    CLK_SCLKMPLL_USERC,
    CLK_ACLK_COREM0,
    CLK_ACLK_CORES,
    CLK_ACLK_COREM1,
    CLK_PERIPHCLK,
    CLK_SCLKHPM,
    CLK_ATCLK,
    CLK_PCLK_DBG,
    /* Dividers */
    CLK_DIVCORE,
    CLK_DIVCORE2,
    CLK_DIVCOPY,
    /* Muxes */
    CLK_MUXCORE,
    CLK_MUXHPM,
    /* Peripherals */
    CLK_UART0,
    CLK_UART1,
    CLK_UART2,
    CLK_UART3,

    CLK_SPI0,
    CLK_SPI1,
    CLK_SPI2,
    CLK_SPI0_ISP,
    CLK_SPI1_ISP,
    /* ----- */
    NCLOCKS,
    /* Aliases */
    DIV_HPM         = CLK_SCLKHPM,
    CLK_ARMCLK      = CLK_DIVCORE2,
    CLK_TRACECLK    = CLK_ATCLK,
    CLK_CTMCLK      = CLK_ATCLK,
    CLK_HCLK_CSSYS  = CLK_ATCLK,
    CLK_TSVAL_UECKL = CLK_PCLK_DBG,
    CLK_FINPLL      = CLK_MASTER,
};

enum clock_gate {
    NCLKGATES,
};

