/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once
#include <autoconf.h>

/* The StarFive JH7110 SoC contains five 8250 compatible UARTs. */

enum chardev_id {
    UART0,
    UART1,
    UART2,
    UART3,
    UART4,
    UART5,
    PS_SERIAL_DEFAULT = UART0
};

#define UART0_PADDR 0x10000000
#define UART1_PADDR 0x10010000
#define UART2_PADDR 0x10020000
#define UART3_PADDR 0x12000000
#define UART4_PADDR 0x12010000
#define UART5_PADDR 0x12020000

#define UART0_IRQ 32
#define UART1_IRQ 33
#define UART2_IRQ 34
#define UART3_IRQ 45
#define UART4_IRQ 46
#define UART5_IRQ 47

/* The default serial device corresponds to the UART available via the GPIO
 * pins of the Star64 Model-A. */
#define DEFAULT_SERIAL_PADDR UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART0_IRQ
