/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _EXYNOS5_CLOCK_H
#define _EXYNOS5_CLOCK_H

enum clk_id {
    CLK_MASTER,
    CLK_SPI0,
    CLK_SPI1,
    CLK_SPI2,
    CLK_SPI0_ISP,
    CLK_SPI1_ISP,
    CLK_UART0,
    CLK_UART1,
    CLK_UART2,
    CLK_UART3,
    CLK_PWM,
    CLK_SCLKMPLL,
    CLK_SCLKBPLL,
    CLK_SCLKCPLL,
    CLK_SCLKGPLL,
    CLK_SCLKEPLL,
    CLK_SCLKVPLL,
    NCLOCKS,
};

enum clock_gate {
    /* These numbers currently have no meaning */
    CLKGATE_I2C0 = 261,
    CLKGATE_I2C1 = 262,
    CLKGATE_I2C2 = 263,
    CLKGATE_I2C3 = 264,
    CLKGATE_I2C4 = 265,
    CLKGATE_I2C5 = 266,
    CLKGATE_I2C6 = 267,
    CLKGATE_I2C7 = 268,
    CLKGATE_SPI0,
    CLKGATE_SPI1,
    CLKGATE_SPI2,
    CLKGATE_UART0,
    CLKGATE_UART1,
    CLKGATE_UART2,
    CLKGATE_UART3,
    NCLKGATES,
};

int exynos5_clock_sys_init(void* cpu, void* core, void* acp, void* isp, void* top,
                           void* lex, void* r0x,  void* r1x, void* cdrex,
                           clock_sys_t* clock_sys);

#endif /* _EXYNOS5_CLOCK_H */



