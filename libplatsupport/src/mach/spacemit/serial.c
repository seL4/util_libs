/*
 * Copyright 2025, 10xEngineers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include "../../chardev.h"

#define UART_THR 0x00         /* UART Transmit Holding Register */
#define UART_LSR 0x14         /* UART Line Status Register */

#define UART_LSR_TDRQ  BIT(5) /* Transmit Data Request */

#define REG_PTR(base, off)     ((volatile uint32_t *)((base) + (off)))

int uart_getchar(ps_chardevice_t *d)
{
    while ((*REG_PTR(d->vaddr, UART_LSR) & BIT(0)));
    return *REG_PTR(d->vaddr, UART_THR);
}

int uart_putchar(ps_chardevice_t *d, int c)
{
    if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
        uart_putchar(d, '\r');
    }

    while ((*REG_PTR(d->vaddr, UART_LSR) & UART_LSR_TDRQ) == 0);

    /* Add character to the buffer. */
    *REG_PTR(d->vaddr, UART_THR) = c;

    return c;
}

static void uart_handle_irq(ps_chardevice_t *dev)
{
    /*
     * This is currently only called when received data is available on the device.
     * The interrupt will be acked to the device on the next read to the device.
     * There's nothing else we need to do here.
     */
}

int uart_init(const struct dev_defn *defn,
              const ps_io_ops_t *ops,
              ps_chardevice_t *dev)
{
    memset(dev, 0, sizeof(*dev));
    void *vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        ZF_LOGE("Unable to map chardev");
        return -1;
    }

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    return 0;
}
