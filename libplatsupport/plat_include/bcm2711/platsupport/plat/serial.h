/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#define BUS_ADDR_OFFSET             0x7E000000
#define PADDDR_OFFSET               0xFE000000

/*
 * The mini UART (UART1) is one of the 3 auxiliary
 * peripherals on the bcm2711 (section 2.1.)
 *      find in seL4/tools/dts/rpi4.dts under
 *          /soc/aux@7e215000
 *          /soc/serial@7e215040
 */
#define UART_BUSADDR                0x7E215000

#define UART_PADDR_0                (UART_BUSADDR-BUS_ADDR_OFFSET+PADDDR_OFFSET)
/*
 * BCM2711 TRM
 * The mini UART (UART1) is part of AUX
 * section 6.2.4. VideoCore interrupts:             29
 * section 6.3.   GIC-400 - VC peripheral IRQs:     96
 * => mini UART IRQ: 96 + 29 = 125
 */
#define UART_IRQ_0                  (125)

enum chardev_id {
    BCM2711_UART0,

    NUM_CHARDEV,
    /* Aliases */
    PS_SERIAL0 = BCM2711_UART0,
    /* defaults */
    PS_SERIAL_DEFAULT = BCM2711_UART0
};

#define DEFAULT_SERIAL_PADDR UART_PADDR_0
#define DEFAULT_SERIAL_INTERRUPT UART_IRQ_0
