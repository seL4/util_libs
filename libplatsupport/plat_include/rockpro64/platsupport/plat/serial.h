/*
 * Copyright 2019, Data61
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

#define UART0_PADDR  0xFF180000
#define UART1_PADDR  0xFF190000
#define UART2_PADDR  0xFF1A0000
#define UART3_PADDR  0xFF1B0000
#define UART4_PADDR  0xFF370000

#define UART0_IRQ    131
#define UART1_IRQ    130
#define UART2_IRQ    132
#define UART3_IRQ    133
#define UART4_IRQ    134

enum chardev_id {
    RP_UART0,
    RP_UART1,
    RP_UART2,
    RP_UART3,
    RP_UART4,
    /* Aliases */
    PS_SERIAL0 = RP_UART0,
    PS_SERIAL1 = RP_UART1,
    PS_SERIAL2 = RP_UART2,
    PS_SERIAL3 = RP_UART3,
    PS_SERIAL4 = RP_UART4,
    /* defaults */
    PS_SERIAL_DEFAULT = RP_UART2
};

#define DEFAULT_SERIAL_PADDR UART2_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART2_IRQ
