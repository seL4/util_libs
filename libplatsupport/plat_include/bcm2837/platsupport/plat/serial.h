/*
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 * Copyright 2022, Technology Innovation Institute
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>
#include <platsupport/gen_config.h>

/*
 * On the Raspberry Pi platforms two processing units are at play: the
 * VideoCore GPU and the ARM CPU. Both have different views on the address
 * space, meaning different base addresses(VC: 0x7e000000, ARM: 0x3f000000).
 * In TRMs and DTS files, generally the VideoCore view is used, resulting in
 * peripherals being listed with their respective VideoCore base address.
 * Since user code is running on the ARM cores, we are more interested in
 * the ARM peripheral addresses.
 *
 * BCM2837 UARTs live under "/soc/" bus.
 * mini-UART is under AUX peripheral block.
 * Access needs to be page aligned.
 *
 * UART0 -> /soc/serial@7e201000
 * UART1 -> /soc/aux@7e215000
 *          -> /soc/serial@7e215040
 */

#define PL011_UART_BASE  0x3f201000
#define MINI_UART_BASE   0x3f215000

#define UART0_OFFSET     0x0     // UART0: PL011
#define UART1_OFFSET     0x40    // UART1: Mini UART

#define UART0_PADDR      (PL011_UART_BASE)   // 0x3f201000
#define UART1_PADDR      (MINI_UART_BASE)    // 0x3f215040


/* BCM2835/2837 TRM
 * Section 7.5
 * The mini-UART from the AUX peripheral triggers IRQ 29 ("Aux int")
 * on the VideoCore, which then signals pending GPU interrupts
 * to the ARM core.
 *
 * The PL011 triggers IRQ 57 ("uart_int").
 *
 * The seL4 kernel's interrupt
 * controller driver creates a linear mapping of all interrupts in the system
 * (see include/drivers/bcm2836-armctrl-ic.h) with 32 ARM PPIs, 32 basic
 * interrupts and 64 GPU interrupts.
 *
 * Thus, the actual interrupts to be used in userland are: 93 (=32+32+29).
 * ->
 *   mini-UART 32+32+29 = 93
 *   PL011 UART 32+32+57 = 121
 */
#define UART0_IRQ  121
#define UART1_IRQ  93

#define DEFAULT_SERIAL_PADDR      UART1_PADDR
#define DEFAULT_SERIAL_INTERRUPT  UART1_IRQ


enum chardev_id {

    BCM2xxx_UART0,
    BCM2xxx_UART1,

    NUM_CHARDEV,

    /* Aliases */
    PS_SERIAL0 = BCM2xxx_UART0,
    PS_SERIAL1 = BCM2xxx_UART1,

    /* Defaults */
    PS_SERIAL_DEFAULT = BCM2xxx_UART1
};