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

#include <stdlib.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include <string.h>

#include "../../chardev.h"

#define USR       0x0008 /* Status register */
#define UCR       0x0010 /* Control register */
#define UTF       0x0070 /* TX fifo */
#define UNTX      0x0040 /* Number of bytes to send */

#define USR_TXRDY             (1U << 2)
#define USR_TXEMP             (1U << 3)

#define CMD_TXRDY_RESET       (3U << 8)

#define UART_REG(base, x)    ((volatile uint32_t *)((uintptr_t)(base) + (x)))

static void
uart_handle_irq(ps_chardevice_t* d UNUSED)
{
}

int uart_putchar(ps_chardevice_t* d, int c)
{
    while (!(*UART_REG(d->vaddr, USR) & USR_TXEMP));

    *UART_REG(d->vaddr, UNTX) = 1;
    *UART_REG(d->vaddr, UTF) = c & 0xff;
    if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
        uart_putchar(d, '\r');
    }
    return 0;
}

int uart_getchar(ps_chardevice_t* d UNUSED)
{
    return EOF;
}

int uart_init(const struct dev_defn* defn,
              const ps_io_ops_t* ops,
              ps_chardevice_t* dev)
{
    /* Attempt to map the virtual address, assure this works */
    void* vaddr = chardev_map(defn, ops);
    memset(dev, 0, sizeof(*dev));
    if (vaddr == NULL) {
        return -1;
    }

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void*)vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    return 0;
}
