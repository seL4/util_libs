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
/* The mini-UART from the Auxiliary peripheral triggers interrupt 29 ("Aux int")
 * on the VideoCore, which then signals pending GPU interrupts to the ARM core.
 * Further details can be found in the Broadcom BCM2835/BCM2837 Manual, section
 * 7.5, table "ARM peripherals interrupts table". The seL4 kernel's interrupt
 * controller driver creates a linear mapping of all interrupts in the system
 * (see include/drivers/bcm2836-armctrl-ic.h) with 32 ARM PPIs, 32 basic
 * interrupts and 64 GPU interrupts. Thus the actual interrupt to be used in
 * userland is 93 (=32+32+29).
 */
#define UART_IRQ_0                  (93)

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
