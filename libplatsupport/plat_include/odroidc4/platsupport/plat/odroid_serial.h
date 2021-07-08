/*
 * Copyright 2021, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define UART0_PADDR     0xffd22000
#define UART1_PADDR     0xffd24000
#define UART2_PADDR     0xffd23000
#define UART0_AO_PADDR  0xff803000
#define UART2_AO_PADDR  0xff804000

#define UART0_OFFSET    0x0
#define UART1_OFFSET    0x0
#define UART2_OFFSET    0x0
#define UART0_AO_OFFSET 0x0
#define UART2_AO_OFFSET 0x0

#define UART0_IRQ       58
#define UART1_IRQ       107
#define UART2_IRQ       125
#define UART0_AO_IRQ    225
#define UART2_AO_IRQ    229
