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
 * space, meaning different base addresses(VC: 0x7e000000, ARM: 0xfe000000).
 * In TRMs and DTS files, generally the VideoCore view is used, resulting in
 * peripherals being listed with their respective VideoCore base address.
 * Since user code is running on the ARM cores, we are more interested in
 * the ARM peripheral addresses.
 * 
 * BCM2711 UARTs live under "/soc/" bus.
 * mini-UART is under AUX peripheral block.
 * Access needs to be page aligned.
 * 
 * UART0 -> /soc/serial@7e201000
 * UART1 -> /soc/aux@7e215000
 *          -> /soc/serial@7e215040
 * UART2 -> /soc/serial@7e201400
 * UART3 -> /soc/serial@7e201600
 * UART4 -> /soc/serial@7e201800
 * UART5 -> /soc/serial@7e201a00
 */

#define PL011_UART_BASE  0xfe201000
#define MINI_UART_BASE   0xfe215000

#define UART0_OFFSET     0x0     // UART0: PL011
#define UART1_OFFSET     0x40    // UART1: Mini UART
#define UART2_OFFSET     0x400   // UART2: PL011
#define UART3_OFFSET     0x600   // UART3: PL011
#define UART4_OFFSET     0x800   // UART4: PL011
#define UART5_OFFSET     0xa00   // UART5: PL011

#define UART0_PADDR      (PL011_UART_BASE)   // 0xfe201000
#define UART1_PADDR      (MINI_UART_BASE)    // 0xfe215040
#define UART2_PADDR      (PL011_UART_BASE)   // 0xfe201400
#define UART3_PADDR      (PL011_UART_BASE)   // 0xfe201600
#define UART4_PADDR      (PL011_UART_BASE)   // 0xfe201800
#define UART5_PADDR      (PL011_UART_BASE)   // 0xfe201a00

/*
 * BCM2711 TRM
 * section 6.2.4. VideoCore interrupts
 * section 6.3.   GIC-400 - VC peripheral IRQs
 * 
 * The mini UART (UART1) is part of AUX
 * -> 29
 * 
 * PL011 UARTs have common IRQ
 * -> 57
 * 
 * GIC-400 VC IRQS start from 96
 * ->
 *   mini-UART IRQ:  96 + 29 = 125
 *   PL011 UART IRQ: 96 + 57 = 153
 */
#define UART0_IRQ  153
#define UART1_IRQ  125
#define UART2_IRQ  153
#define UART3_IRQ  153
#define UART4_IRQ  153
#define UART5_IRQ  153

#define DEFAULT_SERIAL_PADDR      UART1_PADDR
#define DEFAULT_SERIAL_INTERRUPT  UART1_IRQ


enum chardev_id {

    BCM2xxx_UART0,
    BCM2xxx_UART1,
    BCM2xxx_UART2,
    BCM2xxx_UART3,
    BCM2xxx_UART4,
    BCM2xxx_UART5,

    NUM_CHARDEV,

    /* Aliases */
    PS_SERIAL0 = BCM2xxx_UART0,
    PS_SERIAL1 = BCM2xxx_UART1,
    PS_SERIAL2 = BCM2xxx_UART2,
    PS_SERIAL3 = BCM2xxx_UART3,
    PS_SERIAL4 = BCM2xxx_UART4,
    PS_SERIAL5 = BCM2xxx_UART5,

    /* Defaults */
    PS_SERIAL_DEFAULT = BCM2xxx_UART1
};