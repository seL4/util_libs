/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once
#include <platsupport/mach/serial.h>

/* the UARTs are in one page, so we only map the first one */
#define UARTA_PADDR  0x03100000
#define UARTB_PADDR  0x03110000
#define UARTC_PADDR  0x0c280000
#define UARTD_PADDR  0x03130000

#define UARTA_OFFSET 0x0
#define UARTB_OFFSET 0x0
#define UARTC_OFFSET 0x0
#define UARTD_OFFSET 0x0

#define UARTA_IRQ    (112 + 32)
#define UARTB_IRQ    (113 + 32)
#define UARTC_IRQ    (114 + 32)
#define UARTD_IRQ    (115 + 32)


#define PS_SERIAL_DEFAULT   NV_UARTA
#define DEFAULT_SERIAL_PADDR UARTA_PADDR
#define DEFAULT_SERIAL_INTERRUPT UARTA_IRQ
