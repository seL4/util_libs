/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>
#include <platsupport/gen_config.h>

/* Corresponds to the Device Tree node '/soc@107c000000/serial@7d001000' */
#define PL011_UART_BASE  0x107d001000

#define UART0_OFFSET     0x0     // UART0: PL011

#define UART0_PADDR      (PL011_UART_BASE)

#define UART0_IRQ  153

#define DEFAULT_SERIAL_PADDR      UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT  UART0_IRQ

enum chardev_id {
    BCM2xxx_UART0,

    NUM_CHARDEV,

    /* Aliases */
    PS_SERIAL0 = BCM2xxx_UART0,

    /* Defaults */
    PS_SERIAL_DEFAULT = BCM2xxx_UART0
};
