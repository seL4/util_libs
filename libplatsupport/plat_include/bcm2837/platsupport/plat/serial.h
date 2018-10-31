/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
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
