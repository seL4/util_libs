/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once
#include <autoconf.h>

/* The ESWIN EIC7700X SoC contains five 8250 compatible UARTs. */

enum chardev_id {
    UART0,
    UART1,
    UART2,
    UART3,
    UART4,
    PS_SERIAL_DEFAULT = UART0
};

#define UART0_PADDR 0x50900000
#define UART1_PADDR 0x50910000
#define UART2_PADDR 0x50920000
#define UART3_PADDR 0x50930000
#define UART4_PADDR 0x50940000

#define UART0_IRQ 100
#define UART1_IRQ 101
#define UART2_IRQ 102
#define UART3_IRQ 103
#define UART4_IRQ 104

#define DEFAULT_SERIAL_PADDR UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART0_IRQ
