/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2022, Capgemini Engineering
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>
#include <platsupport/mach/serial.h>

#define UART1_PADDR  0x30860000
#define UART2_PADDR  0x30890000
#define UART3_PADDR  0x30880000
#define UART4_PADDR  0x30a60000

#define UART1_IRQ    58
#define UART2_IRQ    59
#define UART3_IRQ    60
#define UART4_IRQ    61

#define UART_REF_CLK 12096000

#if defined(CONFIG_PLAT_IMX8MM_EVK)
#define DEFAULT_SERIAL_PADDR        UART2_PADDR
#define DEFAULT_SERIAL_INTERRUPT    UART2_IRQ
#elif defined(CONFIG_PLAT_IMX8MQ_EVK) || defined(CONFIG_PLAT_MAAXBOARD)
#define DEFAULT_SERIAL_PADDR        UART1_PADDR
#define DEFAULT_SERIAL_INTERRUPT    UART1_IRQ
#endif
