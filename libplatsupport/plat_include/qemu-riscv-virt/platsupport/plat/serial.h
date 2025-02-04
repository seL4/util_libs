/*
 * Copyright 2022, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *
 * QEMU RISC-V virt emulates a 16550 compatible UART.
 *
 */

#pragma once

/* This information is taken from the device tree. */
#define UART0_PADDR     0x10000000
#define UART0_IRQ       10

enum chardev_id {
    UART0,
    /* Aliases */
    PS_SERIAL0 = UART0,
    /* defaults */
    PS_SERIAL_DEFAULT = UART0
};

#define DEFAULT_SERIAL_PADDR        UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT    UART0_IRQ
