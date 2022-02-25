/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define BUS_ADDR_OFFSET             0x7E000000
#define PADDDR_OFFSET               0x3F000000

#define UART_BUSADDR                0x7E215000

#define UART_PADDR_0                (UART_BUSADDR-BUS_ADDR_OFFSET+PADDDR_OFFSET)
/* Broadcom 2835 Peripheral Manual, section 7.5,
 * table "ARM Peripherals interrupts table"
 */
#define UART_IRQ_0                  (57)

enum chardev_id {
    BCM2837_UART0,

    NUM_CHARDEV,
    /* Aliases */
    PS_SERIAL0 = BCM2837_UART0,
    /* defaults */
    PS_SERIAL_DEFAULT = BCM2837_UART0
};

#define DEFAULT_SERIAL_PADDR UART_PADDR_0
#define DEFAULT_SERIAL_INTERRUPT UART_IRQ_0
