/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "../../chardev.h"
#include <string.h>
#include <stdlib.h>

#define IMXUART_DLL             0x000
#define IMXUART_RHR             0x000
#define IMXUART_THR             0x000

#define IMXUART_LSR             0x014
#define IMXUART_LSR_RXFIFIOE    (1<<0)
#define IMXUART_LSR_RXOE        (1<<1)
#define IMXUART_LSR_RXPE        (1<<2)
#define IMXUART_LSR_RXFE        (1<<3)
#define IMXUART_LSR_RXBI        (1<<4)
#define IMXUART_LSR_TXFIFOE     (1<<5)
#define IMXUART_LSR_TXSRE       (1<<6)
#define IMXUART_LSR_RXFIFOSTS   (1<<7)

#define REG_PTR(base, offset)  ((volatile uint32_t *)((char*)(base) + (offset)))

int uart_getchar(ps_chardevice_t* d)
{
    if (*REG_PTR(d->vaddr, IMXUART_LSR) & IMXUART_LSR_RXFIFIOE) {
        return *REG_PTR(d->vaddr, IMXUART_RHR);
    } else {
        return -1;
    }
}

int uart_putchar(ps_chardevice_t* d, int c)
{
    if (*REG_PTR(d->vaddr, IMXUART_LSR) & IMXUART_LSR_TXFIFOE) {
        *REG_PTR(d->vaddr, IMXUART_THR) = c;
        return c;
    } else {
        return -1;
    }
}

static void uart_handle_irq(ps_chardevice_t* d)
{
    /* TODO */
}

int
uart_init(const struct dev_defn* defn,
          const ps_io_ops_t* ops,
          ps_chardevice_t* dev)
{
    void* vaddr = chardev_map(defn, ops);
    memset(dev, 0, sizeof(*dev));
    if (vaddr == NULL) {
        return -1;
    }
    dev->id         = defn->id;
    dev->vaddr      = vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;

    return 0;
}
