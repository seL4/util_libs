/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <elfloader.h>
#include <sys_fputc.h>

#define MPCORE_PRIV               0xF8F00000

/* SCU */
#define SCU_BASE                  (MPCORE_PRIV + 0x0)
#define SCU_CTRL_OFFSET           0x000
#define SCU_FILTADDR_START_OFFSET 0x040
#define SCU_FILTADDR_END_OFFSET   0x044

#define SCU_CTRL_EN               BIT(0)
#define SCU_CTRL_ADDRFILT_EN      BIT(1)

/* SLCR */
#define SLCR_BASE                 0xF8000000
#define SLCR_LOCK_OFFSET          0x004
#define SLCR_UNLOCK_OFFSET        0x008
#define SLCR_UART_CLK_CTRL_OFFSET 0x154
#define SLCR_OCM_CFG_OFFSET       0x910

#define SLCR_LOCK_KEY             0x767B
#define SLCR_UNLOCK_KEY           0xDF0D

/* SLCR register UART_CLK_CTRL
 *
 *  bits  | name     | reset val | description
 * -------+----------+-------------------------------------------------------
 *  31:14 | reserved | 0x0       |
 *  13:8  | DIVISOR  | 0x3F      | Divisor for UART Controller source clock
 *  7:6   | reserved | 0x0       |
 *  5:4   | SRCSEL   | 0x0       | PLL source to generate the clock
 *        |          |           |   b0x: IO PLL (default)
 *        |          |           |   b10: ARM PLL
 *        |          |           |   b11: DDR PLL
 *  3:2   | reserved | 0x0       |
 *  1     | CLKACT1  | 0x1       | UART 1 reference clock
 *        |          |           |   b0: disabled
 *        |          |           |   b1: enabled (default)
 *  0     | CLKACT0  | 0x1       | UART 0 Reference clock
 *        |          |           |   b0: disable
 *                   |           |   b1: enable (default)
 */
#define SLCR_UART_CLK_CTRL_DEFAULT  0x3f03


#define SLCR_OCM_CFG_RAMHI(x)     BIT(x)
#define SLCR_OCM_CFG_RAMHI_ALL    ( SLCR_OCM_CFG_RAMHI(0) \
                                  | SLCR_OCM_CFG_RAMHI(1) \
                                  | SLCR_OCM_CFG_RAMHI(2) \
                                  | SLCR_OCM_CFG_RAMHI(3) )

#define REG(a) *(volatile uint32_t*)(a)

#define SCU(o)  REG(SCU_BASE + SCU_##o##_OFFSET)
#define SLCR(o) REG(SLCR_BASE + SLCR_##o##_OFFSET)

/* Remaps the OCM and ensures DDR is accessible at 0x00000000 */
void remap_ram(void)
{
    /*** 29.4.1 Changing Address Mapping ***/
    /* 1: Complete outstanding transactions */
    asm volatile("dsb");
    asm volatile("isb");

    /* 2-4: prime the icache with this function
     *      skipped because icache is disabled and our remapping does not
     *      affect .text section */

    /* 5-7: unlock SLCR, Modify OCM_CFG, lock SLCR */
    SLCR(UNLOCK) = SLCR_UNLOCK_KEY;
    SLCR(OCM_CFG) |= SLCR_OCM_CFG_RAMHI_ALL;
    SLCR(LOCK) = SLCR_LOCK_KEY;

    /* 8-9: Modify address filtering */
    SCU(FILTADDR_START) = 0x00000000;
    SCU(FILTADDR_END) = 0xFFE00000;

    /* 10: Enable filtering */
    SCU(CTRL) |= (SCU_CTRL_EN | SCU_CTRL_ADDRFILT_EN);

    /* Ensure completion */
    asm volatile("dmb");
}


/* Re-write the reset value into the UART clock control register to trigger
 * proper clock initialization. This is necessary as some QEMU versions (e.g.
 * 5.1) set the initial register values correctly but fail to set up clocks
 * properly in the course of bringing up the system. Rewriting the values
 * outside a reset condition succeeds in setting up the clocks correctly. It
 * does not do any harm on real hardware, basically we switch from implicit
 * to explicit initialization.
 */
void reinitialize_uart_clk(void)
{
    SLCR(UNLOCK) = SLCR_UNLOCK_KEY;
    SLCR(UART_CLK_CTRL) = SLCR_UART_CLK_CTRL_DEFAULT;
    SLCR(LOCK) = SLCR_LOCK_KEY;
}

void platform_init(void)
{
    remap_ram();
    reinitialize_uart_clk();
}
