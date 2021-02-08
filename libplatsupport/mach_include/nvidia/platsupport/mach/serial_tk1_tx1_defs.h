/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* the UARTs are in one page, so we only map the first one */
#define UARTA_PADDR  0x70006000
#define UARTB_PADDR  0x70006000
#define UARTC_PADDR  0x70006000
#define UARTD_PADDR  0x70006000

#define UARTA_OFFSET 0x0
#define UARTB_OFFSET 0x40
#define UARTC_OFFSET 0x200
#define UARTD_OFFSET 0x300

#define UARTA_IRQ    68
#define UARTB_IRQ    69
#define UARTC_IRQ    78
#define UARTD_IRQ    122
