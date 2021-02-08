/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once
#include <autoconf.h>

/* official device names */
enum chardev_id {
    UART0,
    UART1,
    PS_SERIAL_DEFAULT = UART0
};


#define UART0_PADDR 0x10010000
#define UART1_PADDR 0x10011000
#define UART0_IRQ 4
#define UART1_IRQ 5

#define DEFAULT_SERIAL_PADDR UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART0_IRQ
