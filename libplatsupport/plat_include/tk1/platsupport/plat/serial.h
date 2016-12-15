/*
 * Copyright 2016, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */
#pragma once
#include <autoconf.h>

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

/* Official device IDs, as recognized by ps_cdev_init().
 *
 * The first 4 IDs, TK1_UART[ABCD] are "real" devices, and the next 4 IDs,
 * TK1_UART[ABCD]_ASYNC are those same devices repeated.
 *
 * The difference is that if you request the first 4, you will get a handle to
 * a ps_chardevice_t that exports a polling, synchronous interface.
 *
 * If you request the other 4, you will get a handle to a ps_chardevice_t that
 * exports an irq-based, asynchronous interface (and requires you to handle
 * IRQs and use callback functions).
 */
enum chardev_id {
    /* Synchronous devices. */
    TK1_UARTA,
    TK1_UARTB,
    TK1_UARTC,
    TK1_UARTD,
    /* Asynchronous versions of those devices. */
    TK1_UARTA_ASYNC,
    TK1_UARTB_ASYNC,
    TK1_UARTC_ASYNC,
    TK1_UARTD_ASYNC,
    /* Aliases */
    PS_SERIAL0 = TK1_UARTA,
    PS_SERIAL1 = TK1_UARTB,
    PS_SERIAL2 = TK1_UARTC,
    PS_SERIAL3 = TK1_UARTD,
    /* defaults */
    PS_SERIAL_DEFAULT = TK1_UARTD
};
