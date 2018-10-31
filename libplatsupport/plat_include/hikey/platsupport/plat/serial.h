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

#define UART0_PADDR  0xF8015000
#define UART1_PADDR  0xF7111000
#define UART2_PADDR  0xF7112000
#define UART3_PADDR  0xF7113000
#define UART4_PADDR  0xF7114000

#define UART0_IRQ    68
#define UART1_IRQ    69
#define UART2_IRQ    70
#define UART3_IRQ    71
#define UART4_IRQ    72

enum chardev_id {
    DM_UART0,
    DM_UART1,
    DM_UART2,
    DM_UART3,
    DM_UART4,
    /* Aliases */
    PS_SERIAL0 = DM_UART0,
    PS_SERIAL1 = DM_UART1,
    PS_SERIAL2 = DM_UART2,
    PS_SERIAL3 = DM_UART3,
    PS_SERIAL4 = DM_UART4,
    /* defaults */
    PS_SERIAL_DEFAULT = DM_UART0
};

#define DEFAULT_SERIAL_PADDR UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART0_IRQ
