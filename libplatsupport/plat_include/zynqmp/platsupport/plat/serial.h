/*
 * Copyright 2017, DornerWorks
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*
 * This data was produced by DornerWorks, Ltd. of Grand Rapids, MI, USA under
 * a DARPA SBIR, Contract Number D16PC00107.
 *
 * Approved for Public Release, Distribution Unlimited.
 */

#pragma once

#define UART0_PADDR  0xFF000000
#define UART1_PADDR  0xFF010000

#define UART0_IRQ    53
#define UART1_IRQ    54

enum chardev_id {
    ZYNQ_UART0,
    ZYNQ_UART1,
    /* Aliases */
    PS_SERIAL0 = ZYNQ_UART0,
    PS_SERIAL1 = ZYNQ_UART1,
    /* defaults */
#if defined(CONFIG_PLAT_ZYNQMP_ULTRA96) || defined(CONFIG_PLAT_ZYNQMP_ULTRA96V2)
    PS_SERIAL_DEFAULT = ZYNQ_UART1
#else
    PS_SERIAL_DEFAULT = ZYNQ_UART0
#endif
};

#if defined(CONFIG_PLAT_ZYNQMP_ULTRA96) || defined(CONFIG_PLAT_ZYNQMP_ULTRA96V2)
#define DEFAULT_SERIAL_PADDR UART1_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART1_IRQ
#else
#define DEFAULT_SERIAL_PADDR UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART0_IRQ
#endif
