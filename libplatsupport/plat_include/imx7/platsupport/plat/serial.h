/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/mach/serial.h>

#define UART1_PADDR  0x30860000
#define UART2_PADDR  0x30890000
#define UART3_PADDR  0x30880000
#define UART4_PADDR  0x30a60000
#define UART5_PADDR  0x30a70000
#define UART6_PADDR  0x30a80000
#define UART7_PADDR  0x30a90000

#define UART1_IRQ    58
#define UART2_IRQ    59
#define UART3_IRQ    60
#define UART4_IRQ    61
#define UART5_IRQ    62
#define UART6_IRQ    48
#define UART7_IRQ    158

#define UART_REF_CLK 12096000

#define DEFAULT_SERIAL_PADDR        UART1_PADDR
#define DEFAULT_SERIAL_INTERRUPT    UART1_IRQ
