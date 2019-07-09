/*
 * Copyright 2019, Data61
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
#include <autoconf.h>

/* official device names */
enum chardev_id {
    UART0,
    UART1,
    PS_SERIAL_DEFAULT = UART0
};


#define UART0_PADDR 0x10010000
#define UART1_PADDR 0x10011000
#define UART0_IRQ 4
#define UART1_IRQ 5

#define DEFAULT_SERIAL_PADDR UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART0_IRQ
