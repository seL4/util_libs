/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright (c) 2024, Capabilities Ltd <heshamalmatary@capabilitieslimited.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// AP UARTs
#define UART0_PADDR  0x2a400000
#define UART1_PADDR  0x2a410000 // Secure mode

#define UART0_IRQ    95
#define UART1_IRQ    96

enum chardev_id {
    PL001_UART0,
    PL001_UART1,

    /* Aliases */
    PS_SERIAL0 = PL001_UART0,
    PS_SERIAL1 = PL001_UART1,

    /* defaults */
    PS_SERIAL_DEFAULT = PL001_UART0
};

#define DEFAULT_SERIAL_PADDR        UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT    UART0_IRQ
