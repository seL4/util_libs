/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _PLATSUPPORT_PLAT_CLOCK_H_
#define _PLATSUPPORT_PLAT_CLOCK_H_


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

#endif /* _PLATSUPPORT_PLAT_CLOCK_H_ */
