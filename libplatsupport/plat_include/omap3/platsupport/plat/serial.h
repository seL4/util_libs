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

#define UART1_PADDR  0x4806a000
#define UART2_PADDR  0x4806c000
#define UART3_PADDR  0x49020000
#define UART4_PADDR  0x49042000

#define UART1_IRQ    72
#define UART2_IRQ    73
#define UART3_IRQ    74
#define UART4_IRQ    80

/* official device names */
enum chardev_id {
    OMAP3_UART1,
    OMAP3_UART2,
    OMAP3_UART3,
    OMAP3_UART4,
    /* Aliases */
    PS_SERIAL0 = OMAP3_UART1,
    PS_SERIAL1 = OMAP3_UART2,
    PS_SERIAL2 = OMAP3_UART3,
    PS_SERIAL3 = OMAP3_UART4,
    /* defaults */
    PS_SERIAL_DEFAULT = OMAP3_UART3
};

#define DEFAULT_SERIAL_PADDR UART3_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART3_IRQ
