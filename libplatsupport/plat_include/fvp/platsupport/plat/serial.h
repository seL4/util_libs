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

#define UART0_PADDR  0x1c090000 
#define UART1_PADDR  0x1c0a0000 
#define UART2_PADDR  0x1c0b0000 
#define UART3_PADDR  0x1c0c0000 

#define UART0_IRQ    37 
#define UART1_IRQ    38 
#define UART2_IRQ    39 
#define UART3_IRQ    40 

enum chardev_id {
    PL001_UART0,
    PL001_UART1,
    PL001_UART2,
    PL001_UART3,
    /* Aliases */
    PS_SERIAL0 = PL001_UART0,
    PS_SERIAL1 = PL001_UART1,
    PS_SERIAL2 = PL001_UART2,
    PS_SERIAL3 = PL001_UART3,
    /* defaults */
    PS_SERIAL_DEFAULT = PL001_UART0
};

#define DEFAULT_SERIAL_PADDR        UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT    UART0_IRQ

