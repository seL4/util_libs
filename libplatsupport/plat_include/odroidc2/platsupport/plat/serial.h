/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/plat/odroid_serial.h>

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
