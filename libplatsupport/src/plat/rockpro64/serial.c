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

#include <string.h>
#include <stdlib.h>
#include <platsupport/serial.h>
#include "../../chardev.h"

#define RHR         0x00
#define THR         0x00
#define IER         0x04
#define LSR         0x14
#define RHR_MASK    MASK(8)
#define IER_RHRIT   BIT(0)
#define LSR_TXFIFOE BIT(5)
#define LSR_RXFIFOE BIT(0)

#define REG_PTR(base, off)     ((volatile uint32_t *)((base) + (off)))

int uart_getchar(ps_chardevice_t *d)
{
    int ch = EOF;

    if (*REG_PTR(d->vaddr, LSR) & LSR_RXFIFOE) {
        ch = *REG_PTR(d->vaddr, RHR) & RHR_MASK;
    }
    return ch;
}

static void internal_uart_putchar(void* vaddr, int c)
{
    while (!(*REG_PTR(vaddr, LSR) & LSR_TXFIFOE)) {
        continue;
    }
    *REG_PTR(vaddr, THR) = c;
}

int uart_putchar(ps_chardevice_t* d, int c)
{
    void* vaddr = d->vaddr;
    if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
        internal_uart_putchar(vaddr, '\r');
    }
    internal_uart_putchar(vaddr, c);

    return c;
}

static void
uart_handle_irq(ps_chardevice_t* d UNUSED)
{
    /* nothing to do */
}

int uart_init(const struct dev_defn* defn,
              const ps_io_ops_t* ops,
              ps_chardevice_t* dev)
{
    memset(dev, 0, sizeof(*dev));
    void* vaddr = chardev_map(defn, ops);
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

    *REG_PTR(dev->vaddr, IER) = IER_RHRIT;
    return 0;
}
