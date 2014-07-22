/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include "uart.h"
#include <stdlib.h>
#include <platsupport/plat/uart.h>

#define REG_PTR(base, offset)  ((volatile uint32_t *)((char*)(base) + (offset)))

static int uart_getchar(struct ps_chardevice* d)
{
    if (*REG_PTR(d->vaddr, IMXUART_LSR) & IMXUART_LSR_RXFIFIOE) {
        return *REG_PTR(d->vaddr, IMXUART_RHR);
    } else {
        return -1;
    }
}

static int uart_putchar(struct ps_chardevice* d, int c)
{
    if (*REG_PTR(d->vaddr, IMXUART_LSR) & IMXUART_LSR_TXFIFOE) {
        *REG_PTR(d->vaddr, IMXUART_THR) = c;
        return c;
    } else {
        return -1;
    }
}

static int uart_ioctl(struct ps_chardevice* d, int param, long arg)
{
    /* TODO */
    return 0;
}

static void uart_handle_irq(struct ps_chardevice* d, int irq)
{
    /* TODO */
}


struct ps_chardevice*
uart_init(const struct dev_defn* defn,
          const ps_io_ops_t* ops,
          struct ps_chardevice* dev) {

    void* vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        return NULL;
    }
    dev->id         = defn->id;
    dev->vaddr      = vaddr;
    dev->getchar    = &uart_getchar;
    dev->putchar    = &uart_putchar;
    dev->ioctl      = &uart_ioctl;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->rxirqcb    = NULL;
    dev->txirqcb    = NULL;
    dev->ioops      = *ops;

    /* TODO */
    dev->clk        = NULL;

    return dev;
}
