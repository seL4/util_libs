/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

enum clk_id {
    CLK_MASTER, /* The input clock PS_CLK */
    /* PLLs */
    CLK_ARM_PLL,
    CLK_DDR_PLL,
    CLK_IO_PLL,
    /* CPU Clocks */
    CLK_CPU_6OR4X,
    CLK_CPU_3OR2X,
    CLK_CPU_2X,
    CLK_CPU_1X,
    /* DDR Clocks */
    CLK_DDR_2X,
    CLK_DDR_3X,
    /* ----- */
    CLK_DCI,
    /* I/O Peripheral Clocks */
    CLK_GEM0,
    CLK_GEM1,
    CLK_SDIO0,
    CLK_SDIO1,
    CLK_SPI0,
    CLK_SPI1,
    CLK_CAN0,
    CLK_CAN1,
    CLK_UART0,
    CLK_UART1,
    CLK_LQSPI,
    CLK_SMC,
    /* FPGA Programmable Logic Clocks */
    CLK_FPGA_PL0,
    CLK_FPGA_PL1,
    CLK_FPGA_PL2,
    CLK_FPGA_PL3,
    /* System Debug Clocks */
    CLK_DBG,
    CLK_PCAP,
    /* ----- */
    NCLOCKS,
    /* Aliases */
    PS_CLK = CLK_MASTER,
};

enum clock_gate {
    /* The value represents a clock's control bit in the APER_CLK_CTRL register */
    DMAC_CLKGATE    =  0,
    USB0_CLKGATE    =  2,
    USB1_CLKGATE    =  3,
    GEM0_CLKGATE    =  6,
    GEM1_CLKGATE    =  7,
    SDIO0_CLKGATE   = 10,
    SDIO1_CLKGATE   = 11,
    SPI0_CLKGATE    = 14,
    SPI1_CLKGATE    = 15,
    CAN0_CLKGATE    = 16,
    CAN1_CLKGATE    = 17,
    I2C0_CLKGATE    = 18,
    I2C1_CLKGATE    = 19,
    UART0_CLKGATE   = 20,
    UART1_CLKGATE   = 21,
    GPIO_CLKGATE    = 22,
    LQSPI_CLKGATE   = 23,
    SMC_CLKGATE     = 24,
    NCLKGATES
};

/**
 * Select 621 CPU clock ratio
 * @param[in] cpu_clk  One of the CPU clocks
 * @return             0 on success
 */
int clk_cpu_clk_select_621(clk_t *cpu_clk);

/**
 * Select 421 CPU clock ratio
 * @param[in] cpu_clk  One of the CPU clocks
 * @return             0 on success
 */
int clk_cpu_clk_select_421(clk_t *cpu_clk);

