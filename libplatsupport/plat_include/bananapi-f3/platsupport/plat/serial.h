/*
 * Copyright 2025, 10xEngineers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once
#include <autoconf.h>

/* The K1 SoC provides 10 UARTs (UART0–UART9), all are 16550A and 167502 compatible.
 * Below UART refers to UART0, which is used as the serial console.
 */

enum chardev_id {
    UART0,
    PS_SERIAL_DEFAULT = UART0
};

#define UART0_PADDR 0xd4017000
#define UART0_IRQ 42

#define DEFAULT_SERIAL_PADDR UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART0_IRQ
