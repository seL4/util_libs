/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define UART0_PADDR  0x9000000

#define UART0_IRQ    33

enum chardev_id {
    PL001_UART0,
    /* Aliases */
    PS_SERIAL0 = PL001_UART0,
    /* defaults */
    PS_SERIAL_DEFAULT = PL001_UART0
};

#define DEFAULT_SERIAL_PADDR        UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT    UART0_IRQ
