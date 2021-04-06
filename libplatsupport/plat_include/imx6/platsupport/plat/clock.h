/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2020, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

enum clk_id {
    CLK_MASTER,
    CLK_PLL2,
    CLK_MMDC_CH0,
    CLK_AHB,
    CLK_IPG,
    CLK_ARM,
    CLK_ENET,
    CLK_USB1,
    CLK_USB2,
    CLK_CLKO1,
    CLK_CLKO2,
    /* ----- */
    NCLOCKS,
    /* Custom clock */
    CLK_CUSTOM,
    /* Aliases */
    CLK_PLL1 = CLK_ARM,
//  CLK_PLL2 = CLK_SYS
    CLK_PLL3 = CLK_USB1,
//  CLK_PLL4 = CLK_AUDIO,
//  CLK_PLL5 = CLK_VIDEO,
    CLK_PLL6 = CLK_ENET,
    CLK_PLL7 = CLK_USB2,
//  CLK_PLL8 = CLK_MLB,
    CLK_PERCLK = CLK_IPG,
};

#define CLK_GATE(reg, index)  (((reg) << 4) + (index))
enum clock_gate {
    /* -- CCGR0 -- */
    /* -- CCGR1 -- */
#if defined(CONFIG_PLAT_IMX6DQ)
    enet_clock   = CLK_GATE(1, 5),
#endif
    /* -- CCGR2 -- */
    ocotp_ctrl   = CLK_GATE(2, 6),
    i2c3_serial  = CLK_GATE(2, 5),
    i2c2_serial  = CLK_GATE(2, 4),
    i2c1_serial  = CLK_GATE(2, 3),
    /* -- CCGR3 -- */
#if defined(CONFIG_PLAT_IMX6DQ)
    ipu1_ipu     = CLK_GATE(3, 0),
    ipu1_ipu_di0 = CLK_GATE(3, 1),
    ipu1_ipu_di1 = CLK_GATE(3, 2),
    ipu2_ipu     = CLK_GATE(3, 3),
    ipu2_ipu_di0 = CLK_GATE(3, 4),
    ipu2_ipu_di1 = CLK_GATE(3, 5),
#elif defined(CONFIG_PLAT_IMX6SX)
    enet_clock   = CLK_GATE(3, 2),
#else
#error "unknown i.MX6 SOC"
#endif
    /* -- CCGR4 -- */
    /* -- CCGR5 -- */

    /* -- CCGR6 -- */
    usboh3       = CLK_GATE(6, 0),
    usdhc1       = CLK_GATE(6, 1),
    usdhc2       = CLK_GATE(6, 2),
    usdhc3       = CLK_GATE(6, 3),
    usdhc4       = CLK_GATE(6, 4),
    eim_slow     = CLK_GATE(6, 5),
#ifdef CONFIG_PLAT_IMX6Q
    vdoaxiclk    = CLK_GATE(6, 6),
    vpu          = CLK_GATE(6, 7),
#endif
    NCLKGATES
};

