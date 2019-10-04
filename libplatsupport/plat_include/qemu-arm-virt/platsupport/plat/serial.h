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

#define UART0_PADDR  0x9000000

#define UART0_IRQ    1

enum chardev_id {
    PL001_UART0,
    /* Aliases */
    PS_SERIAL0 = PL001_UART0,
    /* defaults */
    PS_SERIAL_DEFAULT = PL001_UART0
};

#define DEFAULT_SERIAL_PADDR        UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT    UART0_IRQ

