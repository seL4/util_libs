/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <string.h>
#include <stdlib.h>
#include "serial.h"

#define RHR         0x00
#define THR         0x00
#define IER         0x04
#define LSR         0x14
#define RHR_MASK    MASK(8)
#define IER_RHRIT   BIT(0)
#define LSR_TXFIFOE BIT(5)
#define LSR_RXFIFOE BIT(0)

#define REG_PTR(base, off)     ((volatile uint32_t *)((base) + (off)))

static int uart_getchar(ps_chardevice_t *d)
{
    int ch = EOF;

    if (*REG_PTR(d->vaddr, LSR) & LSR_RXFIFOE) {
        ch = *REG_PTR(d->vaddr, RHR) & RHR_MASK;
    }
    return ch;
}

static int uart_putchar(ps_chardevice_t* d, int c)
{
    while (!(*REG_PTR(d->vaddr, LSR) & LSR_TXFIFOE))
        continue;
    *REG_PTR(d->vaddr, THR) = c;
    if (c == '\n') {
        uart_putchar(d, '\r');
    }

    return c;
}

static void
uart_handle_irq(ps_chardevice_t* d UNUSED)
{
    /* nothing to do */
}


static ssize_t
uart_write(ps_chardevice_t* d, const void* vdata, size_t count, chardev_callback_t rcb UNUSED, void* token UNUSED)
{
    const char* data = (const char*)vdata;
    int i;
    for (i = 0; i < count; i++) {
        if (uart_putchar(d, *data++) < 0) {
            return i;
        }
    }
    return count;
}

static ssize_t
uart_read(ps_chardevice_t* d, void* vdata, size_t count, chardev_callback_t rcb UNUSED, void* token UNUSED)
{
    char* data;
    int ret;
    int i;
    data = (char*)vdata;
    for (i = 0; i < count; i++) {
        ret = uart_getchar(d);
        if (ret != EOF) {
            *data++ = ret;
        } else {
            return i;
        }
    }
    return count;
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

    *REG_PTR(dev->vaddr, IER) = IER_RHRIT;
    return 0;
}

