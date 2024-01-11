/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Mostly copy/paste from the HiKey plat.
 * Should be moved to a common driver file for PL011 */

#include <string.h>
#include <stdlib.h>
#include <platsupport/serial.h>
#include "../../chardev.h"

#define RHR_MASK                MASK(8)
#define UARTDR                  0x000
#define UARTFR                  0x018
#define UARTIMSC                0x038
#define UARTICR                 0x044
#define PL011_UARTFR_TXFF       BIT(5)
#define PL011_UARTFR_RXFE       BIT(4)

#define REG_PTR(base, off)     ((volatile uint32_t *)((base) + (off)))

int uart_getchar(ps_chardevice_t *d)
{
    int ch = EOF;

    if ((*REG_PTR(d->vaddr, UARTFR) & PL011_UARTFR_RXFE) == 0) {
        ch = *REG_PTR(d->vaddr, UARTDR) & RHR_MASK;
    }
    return ch;
}

int uart_putchar(ps_chardevice_t *d, int c)
{
    while ((*REG_PTR(d->vaddr, UARTFR) & PL011_UARTFR_TXFF) != 0);

    *REG_PTR(d->vaddr, UARTDR) = c;
    if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
        uart_putchar(d, '\r');
    }

    return c;
}

static void uart_handle_irq(ps_chardevice_t *dev)
{
    *REG_PTR(dev->vaddr, UARTICR) = 0x7f0;
}

int uart_init(const struct dev_defn *defn,
              const ps_io_ops_t *ops,
              ps_chardevice_t *dev)
{
    memset(dev, 0, sizeof(*dev));
    void *vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        return -1;
    }

    /* Set up all the device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void *)vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    *REG_PTR(dev->vaddr, UARTIMSC) = 0x50;
    return 0;
}
