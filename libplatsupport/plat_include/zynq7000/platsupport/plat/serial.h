/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define UART0_PADDR  0xE0000000
#define UART1_PADDR  0xE0001000

#define UART0_IRQ    59
#define UART1_IRQ    82

enum chardev_id {
    ZYNQ_UART0,
    ZYNQ_UART1,
    /* Aliases */
    PS_SERIAL0 = ZYNQ_UART0,
    PS_SERIAL1 = ZYNQ_UART1,
    /* defaults */
    PS_SERIAL_DEFAULT = ZYNQ_UART1
};

#define DEFAULT_SERIAL_PADDR UART1_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART1_IRQ
