/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define UART0_PADDR  0x44E09000
#define UART1_PADDR  0x48022000
#define UART2_PADDR  0x48024000

#define UART0_IRQ    72
#define UART1_IRQ    73
#define UART2_IRQ    74

enum chardev_id {
    DM_UART0,
    DM_UART1,
    DM_UART2,
    /* Aliases */
    PS_SERIAL0 = DM_UART0,
    PS_SERIAL1 = DM_UART1,
    PS_SERIAL2 = DM_UART2,
    /* defaults */
    PS_SERIAL_DEFAULT = DM_UART0
};

#define DEFAULT_SERIAL_PADDR UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART0_IRQ
