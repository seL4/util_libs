/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdlib.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include "serial.h"
#include <string.h>

#define REG_PTR(base, off)     ((volatile uint32_t *)((base) + (off)))

/* When DLAB=1, MU_IO is a baud rate register.
 * Otherwise, write to TX, read to RX */
#define MU_IO       0x40
/* When DLAB=1, MU_IIR is a baud rate register.
 * Otherwise IRQ enable */
#define MU_IIR      0x44
#define MU_IER      0x48
#define MU_LCR      0x4C
#define MU_MCR      0x50
#define MU_LSR      0x54
#define MU_MSR      0x58
#define MU_SCRATCH  0x5C
#define MU_CNTL     0x60

/* This bit is set if the transmit FIFO can accept at least one byte.*/
#define MU_LSR_TXEMPTY   BIT(5)
/* This bit is set if the transmit FIFO is empty and the
 * transmitter is idle. (Finished shifting out the last bit). */
#define MU_LSR_TXIDLE    BIT(6)
#define MU_LSR_RXOVERRUN BIT(1)
#define MU_LSR_DATAREADY BIT(0)

#define MU_LCR_DLAB      BIT(7)
#define MU_LCR_BREAK     BIT(6)
#define MU_LCR_DATASIZE  BIT(0)

static void uart_handle_irq(ps_chardevice_t *d UNUSED)
{
}

int uart_putchar(ps_chardevice_t *d, int c)
{
    while (!(*REG_PTR(d->vaddr, MU_LSR) & MU_LSR_TXIDLE));
    *REG_PTR(d->vaddr, MU_IO) = (c & 0xff);

    return 0;
}

int uart_getchar(ps_chardevice_t *d UNUSED)
{
    while (!(*REG_PTR(d->vaddr, MU_LSR) & MU_LSR_DATAREADY));
    return *REG_PTR(d->vaddr, MU_IO);
}

int uart_init(const struct dev_defn *defn,
              const ps_io_ops_t *ops,
              ps_chardevice_t *dev)
{
    /* Attempt to map the virtual address, assure this works */
    void *vaddr = chardev_map(defn, ops);
    memset(dev, 0, sizeof(*dev));
    if (vaddr == NULL) {
        return -1;
    }

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void *)vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    return 0;
}
