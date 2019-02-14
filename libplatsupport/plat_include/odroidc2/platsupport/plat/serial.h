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

#define UART0_PADDR     0xc1108000
#define UART1_PADDR     0xc1108000
#define UART2_PADDR     0xc1108000
#define UART0_AO_PADDR  0xc8100000
#define UART2_AO_PADDR  0xc8104000

#define UART0_OFFSET    0x4c0
#define UART1_OFFSET    0x4dc
#define UART2_OFFSET    0x700
#define UART0_AO_OFFSET 0x4c0
#define UART2_AO_OFFSET 0x4e0

#define UART0_IRQ       54
#define UART1_IRQ       105
#define UART2_IRQ       123
#define UART0_AO_IRQ    225
#define UART2_AO_IRQ    229

enum chardev_id {
    UART0,
    UART1,
    UART2,
    UART0_AO,
    UART2_AO,
    /* Aliases */
    PS_SERIAL0 = UART0,
    PS_SERIAL1 = UART1,
    PS_SERIAL2 = UART2,
    PS_SERIAL3 = UART0_AO,
    PS_SERIAL4 = UART2_AO,
    /* defaults */
    PS_SERIAL_DEFAULT = PS_SERIAL3
};

#define DEFAULT_SERIAL_PADDR UART0_AO_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART0_AO_IRQ
