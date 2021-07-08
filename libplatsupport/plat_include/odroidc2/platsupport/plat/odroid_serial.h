/*
 * Copyright 2021, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define UART0_PADDR     0xc1108000
#define UART1_PADDR     0xc1108000
#define UART2_PADDR     0xc1108000
#define UART0_AO_PADDR  0xc8100000
#define UART2_AO_PADDR  0xc8104000

#define UART0_OFFSET    0x4c0
#define UART1_OFFSET    0x4dc
#define UART2_OFFSET    0x700
#define UART0_AO_OFFSET 0x4c0
#define UART2_AO_OFFSET 0x4e0

#define UART0_IRQ       54
#define UART1_IRQ       105
#define UART2_IRQ       123
#define UART0_AO_IRQ    225
#define UART2_AO_IRQ    229

