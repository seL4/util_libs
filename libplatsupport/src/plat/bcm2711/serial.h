/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "../../chardev.h"
#include <autoconf.h>
#include <platsupport/gen_config.h>

#define UART0_IRQ       153
#define UART1_IRQ       125
#define UART2_IRQ       153
#define UART3_IRQ       153
#define UART4_IRQ       153
#define UART5_IRQ       153

#define PL011_UART_BASE 0xfe201000
#define MINI_UART_BASE  0xfe215000

#define UART0_OFFSET    0x0     // UART0: PL011
#define UART1_OFFSET    0x40    // UART1: Mini UART
#define UART2_OFFSET    0x400   // UART2: PL011
#define UART3_OFFSET    0x600   // UART3: PL011
#define UART4_OFFSET    0x800   // UART4: PL011
#define UART5_OFFSET    0xa00   // UART5: PL011

#define UART0_PADDR  (PL011_UART_BASE + UART0_OFFSET)   // 0xfe201000
#define UART1_PADDR  (MINI_UART_BASE  + UART1_OFFSET)   // 0xfe215040
#define UART2_PADDR  (PL011_UART_BASE + UART2_OFFSET)   // 0xfe201400
#define UART3_PADDR  (PL011_UART_BASE + UART3_OFFSET)   // 0xfe201600
#define UART4_PADDR  (PL011_UART_BASE + UART4_OFFSET)   // 0xfe201800
#define UART5_PADDR  (PL011_UART_BASE + UART5_OFFSET)   // 0xfe201a00

int uart_init(const struct dev_defn *defn, const ps_io_ops_t *ops, ps_chardevice_t *dev);
