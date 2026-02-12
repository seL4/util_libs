/*
 * Copyright 2026, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define UART0_PADDR  0x400E0000
#define UART1_PADDR  0x400F0000

#define DEFAULT_SERIAL_PADDR     UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART0_IRQ
#define UART_REF_CLK             64000000

/* official device names */
enum chardev_id {
    UART0,
    UART1,
    /* defaults */
    PS_SERIAL_DEFAULT = UART0
};
